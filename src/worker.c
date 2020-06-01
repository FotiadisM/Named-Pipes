#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <wordexp.h>

#include "../include/worker.h"
#include "../include/pipes.h"
#include "../include/hashTable.h"

volatile sig_atomic_t m_signal = 0;

static int Worker_Init(int *w_fd, int *r_fd, char **buffer, const size_t bufferSize, ListPtr *list, HashTablePtr *h1, HashTablePtr *h2, struct sigaction **act);

static string_nodePtr Worker_GetCountries(const int r_fd, const int w_fd, char *buffer, const size_t bufferSize);
static int Worker_Run(ListPtr list, HashTablePtr h1, HashTablePtr h2, const string_nodePtr countries, const int w_fd, const size_t bufferSize, const char *input_dir);
static int Worker_wait_input(const int w_fd, const int r_fd, char *buffer, const size_t bufferSize, const ListPtr list, const HashTablePtr h1, const HashTablePtr h2);

static void Worker_handleSignals(struct sigaction *act);
static void handler(int signum);

static int diseaseFrequency(const int w_fd, const size_t bufferSize, const char *str, const HashTablePtr ht);
static int numFunctions(const int w_fd, const size_t bufferSize, const char *str, const HashTablePtr h1, const ListPtr list, const int flag);

static int validatePatient(const ListPtr list, const char *id);
static PatientPtr getPatient(const wordexp_t *p, const char *country, const ListPtr list);
static PatientPtr getPatientById(const char *id, const ListPtr list);
static char *Worker_getPath(const char *input_dir, const char *country);

static int Worker_sendStatistics(const statsPtr st, const int w_fd, const size_t bufferSize, const char *country, const char *file);

static ageInfoPtr ageInfo_Init();
static void ageInfo_add(ageInfoPtr ag, const char *age_str);
static statsPtr stats_Init();
static statsPtr stats_add(statsPtr st, const char *disease, const char *age_str);
static void stats_close(statsPtr st);

int Worker(const size_t bufferSize, char *input_dir)
{
    int r_fd = 0, w_fd = 0;
    char *buffer = NULL;
    string_nodePtr countries = NULL;
    ListPtr list = NULL;
    HashTablePtr diseaseHT = NULL, countryHT = NULL;

    sigset_t blockset;
    struct sigaction *act = NULL;

    sigemptyset(&blockset);
    sigaddset(&blockset, SIGINT);
    sigaddset(&blockset, SIGQUIT);
    sigaddset(&blockset, SIGUSR1);
    sigprocmask(SIG_BLOCK, &blockset, NULL);

    if (Worker_Init(&w_fd, &r_fd, &buffer, bufferSize, &list, &diseaseHT, &countryHT, &act) == -1)
    {
        printf("Worker_Init() failed");
        return -1;
    }

    Worker_handleSignals(act);

    if ((countries = Worker_GetCountries(r_fd, w_fd, buffer, bufferSize)) == NULL)
    {
        if (close(r_fd) == -1 || close(w_fd) == -1)
        {
            perror("close");
        }

        free(act);
        free(buffer);
        free(input_dir);

        HashTable_Close(diseaseHT);
        HashTable_Close(countryHT);
        List_Close(list, F_PATIENT);

        return -1;
    }

    if (Worker_Run(list, diseaseHT, countryHT, countries, w_fd, bufferSize, input_dir) == -1)
    {
        printf("Worker_Run() failed\n");
    }

    clear_stringNode(countries);

    if (Worker_wait_input(w_fd, r_fd, buffer, bufferSize, list, diseaseHT, countryHT) == -1)
    {
        perror("Worker_wait_input() failed");
        return -1;
    }

    if (close(r_fd) == -1 || close(w_fd) == -1)
    {
        perror("close");
    }

    free(act);
    free(buffer);
    free(input_dir);

    HashTable_Close(diseaseHT);
    HashTable_Close(countryHT);
    List_Close(list, F_PATIENT);

    return 0;
}

static int Worker_Init(int *w_fd, int *r_fd, char **buffer, const size_t bufferSize, ListPtr *list, HashTablePtr *h1, HashTablePtr *h2, struct sigaction **act)
{
    if ((*w_fd = Pipe_Init("./pipes/r_", getpid(), O_WRONLY)) == -1)
    {
        printf("Pipe_Init() failed\n");
        return -1;
    }

    if ((*r_fd = Pipe_Init("./pipes/w_", getpid(), O_RDONLY)) == -1)
    {
        printf("Pipe_Init() failed");
        return -1;
    }

    if ((*buffer = malloc(bufferSize)) == NULL)
    {
        perror("malloc");
        return -1;
    }

    if ((*list = List_Init()) == NULL)
    {
        return -1;
    }

    if ((*h1 = HashTable_Init(10, 24)) == NULL)
    {
        return -1;
    }
    if ((*h2 = HashTable_Init(10, 24)) == NULL)
    {
        return -1;
    }

    if ((*act = malloc(sizeof(struct sigaction))) == NULL)
    {
        perror("malloc");
        return -1;
    }

    return 0;
}

static string_nodePtr Worker_GetCountries(const int r_fd, const int w_fd, char *buffer, const size_t bufferSize)
{
    char *str = NULL;
    string_nodePtr node = NULL;

    while (1)
    {
        str = decode(r_fd, buffer, bufferSize);

        if (!strcmp(str, "OK"))
        {
            free(str);
            break;
        }

        else if (!strcmp(str, "/exit"))
        {
            free(str);

            return NULL;
        }

        else
        {
            if ((node = add_stringNode(node, str)) == NULL)
            {
                printf("add_stringNode() failed");
                return NULL;
            }
        }

        free(str);
    }

    return node;
}

static void Worker_handleSignals(struct sigaction *act)
{
    act->sa_handler = (void *)handler;
    sigemptyset(&act->sa_mask);
    act->sa_flags = 0;

    sigaction(SIGINT, act, NULL);
    sigaction(SIGQUIT, act, NULL);
    sigaction(SIGUSR1, act, NULL);
}

static void handler(int signum)
{
    if (signum == SIGINT || signum == SIGQUIT)
    {
        m_signal = 1;
    }
}

static void sig_int()
{
    FILE *filePtr = NULL;

    char path[100];

    sprintf(path, "./logs/log_file%d", getpid());

    if ((filePtr = fopen(path, "w+")) == NULL)
    {
        perror("open file");
    }

    fclose(filePtr);

    printf("worker: %d recieved siganl\n", getpid());
}

static int Worker_Run(ListPtr list, HashTablePtr h1, HashTablePtr h2, const string_nodePtr countries, const int w_fd, const size_t bufferSize, const char *input_dir)
{
    wordexp_t p;
    size_t len = 0;
    DIR *dirp = NULL;
    FILE *filePtr = NULL;
    ListNodePtr node = NULL;
    PatientPtr patient = NULL;
    struct dirent *dir_info = NULL;
    string_nodePtr country = countries;
    char *path = NULL, *file = NULL, *line = NULL;

    while (country != NULL)
    {
        if ((path = Worker_getPath(input_dir, country->str)) == NULL)
        {
            perror("Worker_getPath()");
            return -1;
        }

        if ((dirp = opendir(path)) == NULL)
        {
            perror("opendir()");
            return -1;
        }

        while ((dir_info = readdir(dirp)) != NULL)
        {
            statsPtr st = NULL;

            if (!(strcmp(dir_info->d_name, ".") == 0 || strcmp(dir_info->d_name, "..") == 0))
            {
                if ((file = Worker_getPath(path, dir_info->d_name)) == NULL)
                {
                    perror("Worker_getPath()");
                    return -1;
                }

                if ((filePtr = fopen(file, "r")) == NULL)
                {
                    perror("fopen()");
                    return -1;
                }

                while (getline(&line, &len, filePtr) != -1)
                {
                    strtok(line, "\n");
                    wordexp(line, &p, 0);

                    if (!strcmp(p.we_wordv[1], "ENTER"))
                    {
                        if (validatePatient(list, p.we_wordv[0]))
                        {
                            if ((st = stats_add(st, p.we_wordv[4], p.we_wordv[5])) == NULL)
                            {
                                printf("stats_add() failed");
                                return -1;
                            }

                            if ((patient = Patient_Init(p.we_wordv[0], p.we_wordv[2], p.we_wordv[3], p.we_wordv[5], p.we_wordv[4], country->str, dir_info->d_name)) == NULL)
                            {
                                printf("Patient_Init() failed");
                                return -1;
                            }
                            patient->exitDate = NULL;

                            if ((node = List_InsertSorted(list, patient)) == NULL)
                            {
                                return -1;
                            }

                            if (HashTable_Insert(h1, patient->diseaseID, node) == -1)
                            {
                                return -1;
                            }

                            if (HashTable_Insert(h2, patient->country, node) == -1)
                            {
                                return -1;
                            }
                        }
                    }
                    else
                    {
                        if ((patient = getPatient(&p, country->str, list)) == NULL)
                        {
                            // printf("Error: patient not registered\n");
                        }
                        else
                        {
                            if (Patient_addExitDate(patient, dir_info->d_name) == -1)
                            {
                                printf("Patient_addExitDate() failed");
                            }
                        }
                    }

                    wordfree(&p);
                }

                if (Worker_sendStatistics(st, w_fd, bufferSize, country->str, dir_info->d_name) == -1)
                {
                    perror("Worker_sendStatistics");
                    return -1;
                }

                stats_close(st);
                free(file);
                fclose(filePtr);
            }
        }

        free(path);
        closedir(dirp);

        country = country->next;
    }

    encode(w_fd, "OK", bufferSize);

    free(line);

    return 0;
}

static int Worker_wait_input(const int w_fd, const int r_fd, char *buffer, const size_t bufferSize, const ListPtr list, const HashTablePtr h1, const HashTablePtr h2)
{
    fd_set rfds;
    wordexp_t p;
    char *str = NULL;

    while (1)
    {
        sigset_t emptyset;

        sigemptyset(&emptyset);

        FD_ZERO(&rfds);
        FD_SET(r_fd, &rfds);

        if (pselect(r_fd + 1, &rfds, NULL, NULL, NULL, &emptyset) == -1)
        {
            if (errno == EINTR)
            {
                if (m_signal == 1)
                {
                    sig_int();
                    break;
                }
            }
            else
            {
                perror("pselect");
            }
        }

        str = decode(r_fd, buffer, bufferSize);

        wordexp(str, &p, 0);

        if (m_signal)
        {
            if (m_signal == 1)
            {
                sig_int();
            }
        }

        if (!strcmp(p.we_wordv[0], "/exit"))
        {
            free(str);
            wordfree(&p);
            break;
        }
        else if (!strcmp(p.we_wordv[0], "/diseaseFrequency"))
        {
            diseaseFrequency(w_fd, bufferSize, str, h1);
        }
        else if (!strcmp(p.we_wordv[0], "/topk-AgeRanges"))
        {
            printf("str: %s\n", str);
        }
        else if (!strcmp(p.we_wordv[0], "/searchPatientRecord"))
        {
            PatientPtr patient = NULL;

            if ((patient = getPatientById(p.we_wordv[1], list)) != NULL)
            {
                Patient_Print(patient);
            }
        }
        else if (!strcmp(p.we_wordv[0], "/numPatientAdmissions"))
        {
            numFunctions(w_fd, bufferSize, str, h2, list, ENTER);
        }
        else if (!strcmp(p.we_wordv[0], "/numPatientDischarges"))
        {
            numFunctions(w_fd, bufferSize, str, h2, list, EXIT);
        }

        wordfree(&p);
        free(str);
    }

    return 0;
}

static int diseaseFrequency(const int w_fd, const size_t bufferSize, const char *str, const HashTablePtr ht)
{
    char answ[12];
    wordexp_t p;
    AVLTreePtr tree = NULL;
    DatePtr d1 = NULL, d2 = NULL;

    wordexp(str, &p, 0);

    if ((d1 = Date_Init(p.we_wordv[2])) == NULL || (d2 = Date_Init(p.we_wordv[3])) == NULL)
    {
        return -1;
    }

    if ((tree = HashTable_LocateKey(&(ht->table[hash(p.we_wordv[1]) % ht->size]), p.we_wordv[1], ht->bucketSize)) == NULL)
    {
        printf("Disease not found\n");
        return 0;
    }

    sprintf(answ, "%d", AVLNode_countPatients(tree->root, p.we_wordv[1], p.we_wordv[4], d1, d2));
    encode(w_fd, answ, bufferSize);

    free(d1);
    free(d2);
    wordfree(&p);

    return 0;
}

static int numFunctions(const int w_fd, const size_t bufferSize, const char *str, const HashTablePtr h1, const ListPtr list, const int flag)
{
    int count = 0;
    wordexp_t p;
    HashNodePtr node = NULL;
    ListNodePtr ptr = list->head;
    DatePtr d1 = NULL, d2 = NULL;

    wordexp(str, &p, 0);

    if ((d1 = Date_Init(p.we_wordv[2])) == NULL || (d2 = Date_Init(p.we_wordv[3])) == NULL)
    {
        return -1;
    }

    if (p.we_wordc == 4)
    {
        if (flag == ENTER)
        {
            for (int i = 0; i < h1->size; i++)
            {
                node = &(h1->table[i]);
                while (node != NULL)
                {
                    for (int j = 0; j < (int)h1->bucketSize; j++)
                    {
                        if (node->entries[j] != NULL)
                        {
                            ptr = list->head;
                            while (ptr != NULL)
                            {
                                if (!strcmp(ptr->patient->country, node->entries[j]->key))
                                {
                                    if (!strcmp(ptr->patient->diseaseID, p.we_wordv[1]))
                                    {
                                        if (Date_Compare(d1, ptr->patient->entryDate) <= 0 && Date_Compare(d2, ptr->patient->entryDate) >= 0)
                                        {
                                            count++;
                                        }
                                    }
                                }
                                ptr = ptr->next;
                            }
                            printf("%s %d\n", node->entries[j]->key, count);
                        }
                    }
                    node = node->next;
                }
            }
        }
        else
        {
            for (int i = 0; i < h1->size; i++)
            {
                node = &(h1->table[i]);
                while (node != NULL)
                {
                    for (int j = 0; j < (int)h1->bucketSize; j++)
                    {
                        if (node->entries[j] != NULL)
                        {
                            ptr = list->head;
                            while (ptr != NULL)
                            {
                                if (!strcmp(ptr->patient->country, node->entries[j]->key))
                                {
                                    if (!strcmp(ptr->patient->diseaseID, p.we_wordv[1]))
                                    {
                                        if (ptr->patient->exitDate != NULL)
                                        {
                                            if (Date_Compare(d1, ptr->patient->exitDate) <= 0 && Date_Compare(d2, ptr->patient->exitDate) >= 0)
                                            {
                                                count++;
                                            }
                                        }
                                    }
                                }
                                ptr = ptr->next;
                            }
                            printf("%s %d\n", node->entries[j]->key, count);
                        }
                    }
                    node = node->next;
                }
            }
        }
    }

    else
    {
        if (flag == ENTER)
        {
            while (ptr != NULL)
            {
                if (!strcmp(ptr->patient->country, p.we_wordv[4]))
                {
                    if (!strcmp(ptr->patient->diseaseID, p.we_wordv[1]))
                    {
                        if (Date_Compare(d1, ptr->patient->entryDate) <= 0 && Date_Compare(d2, ptr->patient->entryDate) >= 0)
                        {
                            count++;
                        }
                    }
                }
                ptr = ptr->next;
            }
        }
        else
        {
            while (ptr != NULL)
            {
                if (!strcmp(ptr->patient->country, p.we_wordv[4]))
                {
                    if (!strcmp(ptr->patient->diseaseID, p.we_wordv[1]))
                    {
                        if (ptr->patient->exitDate != NULL)
                        {
                            if (Date_Compare(d1, ptr->patient->exitDate) <= 0 && Date_Compare(d2, ptr->patient->exitDate) >= 0)
                            {
                                count++;
                            }
                        }
                    }
                }
                ptr = ptr->next;
            }
        }

        printf("%s %d\n", p.we_wordv[4], count);
    }

    free(d1);
    free(d2);
    wordfree(&p);

    return 0;
}

static int validatePatient(const ListPtr list, const char *id)
{
    ListNodePtr node = list->head;

    while (node != NULL)
    {
        if (!strcmp(node->patient->id, id))
        {
            return 0;
        }
        node = node->next;
    }

    return 1;
}

static PatientPtr getPatient(const wordexp_t *p, const char *country, const ListPtr list)
{
    ListNodePtr node = list->head;

    while (node != NULL)
    {
        if (!Patient_Compare(node->patient, p->we_wordv[0], p->we_wordv[2], p->we_wordv[3], p->we_wordv[4], country, p->we_wordv[5]))
        {
            return node->patient;
        }
        node = node->next;
    }

    return NULL;
}

static PatientPtr getPatientById(const char *id, const ListPtr list)
{
    ListNodePtr node = list->head;

    while (node != NULL)
    {
        if (!strcmp(node->patient->id, id))
        {
            return node->patient;
        }
        node = node->next;
    }

    return NULL;
}

static char *Worker_getPath(const char *input_dir, const char *country)
{
    char *path = NULL;

    if ((path = malloc(strlen(input_dir) + strlen(country) + 2)) == NULL)
    {
        perror("malloc");
        return NULL;
    }

    strcpy(path, input_dir);
    strcat(path, "/");
    strcat(path, country);

    return path;
}

static int Worker_sendStatistics(const statsPtr st, const int w_fd, const size_t bufferSize, const char *country, const char *file)
{
    statsPtr node = st;
    size_t final_len = 0;
    string_nodePtr buffer_list = NULL, tmp = NULL;
    char *date = NULL, *buffer = NULL, *final_buffer = NULL;
    char ag1[12] = {'\0'}, ag2[12] = {'\0'}, ag3[12] = {'\0'}, ag4[12] = {'\0'};

    if ((date = strdup(file)) == NULL)
    {
        perror("strdup");
        return -1;
    }
    strtok(date, ".");

    while (node != NULL)
    {

        sprintf(ag1, "%d", node->ag->ag1);
        sprintf(ag2, "%d", node->ag->ag2);
        sprintf(ag3, "%d", node->ag->ag3);
        sprintf(ag4, "%d", node->ag->ag4);

        if ((buffer = malloc(strlen(node->disease) + strlen(ag1) + strlen(ag2) + strlen(ag3) + strlen(ag4) + 7)) == NULL)
        {
            perror("malloc");
            return -1;
        }

        strcpy(buffer, node->disease);
        strcat(buffer, "\n");
        strcat(buffer, ag1);
        strcat(buffer, "\n");
        strcat(buffer, ag2);
        strcat(buffer, "\n");
        strcat(buffer, ag3);
        strcat(buffer, "\n");
        strcat(buffer, ag4);
        strcat(buffer, "\n\n");

        final_len += strlen(buffer);
        buffer_list = add_stringNode(buffer_list, buffer);

        free(buffer);

        node = node->next;
    }

    if ((final_buffer = malloc(strlen(date) + strlen(country) + final_len + 3)) == NULL)
    {
        perror("malloc");
        return -1;
    }
    strcpy(final_buffer, date);
    strcat(final_buffer, "\n");
    strcat(final_buffer, country);
    strcat(final_buffer, "\n");

    tmp = buffer_list;
    while (tmp != NULL)
    {
        strcat(final_buffer, tmp->str);
        tmp = tmp->next;
    }

    encode(w_fd, final_buffer, bufferSize);

    free(date);
    free(final_buffer);
    clear_stringNode(buffer_list);

    return 0;
}

static ageInfoPtr ageInfo_Init()
{
    ageInfoPtr ag = NULL;

    if ((ag = malloc(sizeof(ageInfo))) == NULL)
    {
        perror("malloc");
        return NULL;
    }

    ag->ag1 = ag->ag2 = ag->ag3 = ag->ag4 = 0;

    return ag;
}

static void ageInfo_add(ageInfoPtr ag, const char *age_str)
{
    int age = (int)strtol(age_str, NULL, 10);

    if (age <= 20)
    {
        ag->ag1++;
    }
    else if (age <= 40)
    {
        ag->ag2++;
    }
    else if (age <= 60)
    {
        ag->ag3++;
    }
    else
    {
        ag->ag4++;
    }
}

static statsPtr stats_Init(const char *disease)
{
    statsPtr st = NULL;

    if ((st = malloc(sizeof(stats))) == NULL)
    {
        perror("malloc");
        return NULL;
    }

    if ((st->ag = ageInfo_Init()) == NULL)
    {
        perror("ageInfo_Init() failed");
        return NULL;
    }

    if ((st->disease = strdup(disease)) == NULL)
    {
        perror("strdup");
        return NULL;
    }
    st->next = NULL;

    return st;
}

static statsPtr stats_add(statsPtr st, const char *disease, const char *age_str)
{
    statsPtr tmp = st;

    if (st == NULL)
    {
        if ((st = stats_Init(disease)) == NULL)
        {
            perror("stats_Init");
            return NULL;
        }
        ageInfo_add(st->ag, age_str);

        return st;
    }

    while (tmp->next != NULL)
    {
        if (!strcmp(tmp->disease, disease))
        {
            break;
        }

        tmp = tmp->next;
    }

    if (!strcmp(tmp->disease, disease))
    {
        ageInfo_add(tmp->ag, age_str);
    }
    else
    {
        if ((tmp->next = stats_Init(disease)) == NULL)
        {
            perror("stats_Init()");
            return NULL;
        }

        ageInfo_add(tmp->next->ag, age_str);
    }

    return st;
}

static void stats_close(statsPtr st)
{
    statsPtr tmp = st;

    while (st != NULL)
    {
        tmp = st;
        st = st->next;

        free(tmp->disease);
        free(tmp->ag);
        free(tmp);
    }
}