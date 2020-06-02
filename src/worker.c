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
#include "../include/stats.h"
#include "../include/pipes.h"
#include "../include/hashTable.h"

volatile sig_atomic_t m_signal = 0;

static int Worker_Init(int *w_fd, int *r_fd, char **buffer, const size_t bufferSize, ListPtr *list, HashTablePtr *h1, HashTablePtr *h2, struct sigaction **act);
static string_nodePtr Worker_GetCountries(const int r_fd, char *buffer, const size_t bufferSize);
static int Worker_Run(ListPtr list, HashTablePtr h1, HashTablePtr h2, const string_nodePtr countries, string_nodePtr *dates, const int w_fd, const size_t bufferSize, const char *input_dir);
static int Worker_wait_input(const int w_fd, const int r_fd, char *buffer, const size_t bufferSize, const ListPtr list, const HashTablePtr h1, const HashTablePtr h2, const string_nodePtr countries, string_nodePtr date, const char *input_dir);

static void Worker_handleSignals(struct sigaction *act);
static void handler(int signum);
static void handle_sigint(const string_nodePtr countries, const int count, const int err);
static int handle_sigusr1(ListPtr list, HashTablePtr h1, HashTablePtr h2, const string_nodePtr countries, string_nodePtr dates, const char *input_dir);

static int diseaseFrequency(const int w_fd, const size_t bufferSize, const char *str, const HashTablePtr ht);
static int topk_AgeRanges(const int w_fd, const size_t bufferSize, const char *str, const ListPtr list);
static int searchPatientRecord(wordexp_t *p, const int w_fd, const size_t bufferSize, const ListPtr list);
static int numFunctions(const int w_fd, const size_t bufferSize, const char *str, const HashTablePtr h1, const ListPtr list, const int flag);

static int validatePatient(const ListPtr list, const char *id);
static PatientPtr getPatient(const wordexp_t *p, const char *country, const ListPtr list);
static PatientPtr getPatientById(const char *id, const ListPtr list);
static char *Worker_getPath(const char *input_dir, const char *country);

int Worker(const size_t bufferSize, char *input_dir)
{
    int r_fd = 0, w_fd = 0;
    char *buffer = NULL;
    ListPtr list = NULL;
    string_nodePtr countries = NULL, dates = NULL;
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

    if ((countries = Worker_GetCountries(r_fd, buffer, bufferSize)) == NULL)
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

    if (Worker_Run(list, diseaseHT, countryHT, countries, &dates, w_fd, bufferSize, input_dir) == -1)
    {
        printf("Worker_Run() failed\n");
    }

    if (Worker_wait_input(w_fd, r_fd, buffer, bufferSize, list, diseaseHT, countryHT, countries, dates, input_dir) == -1)
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
    clear_stringNode(countries);
    clear_stringNode(dates);

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

static string_nodePtr Worker_GetCountries(const int r_fd, char *buffer, const size_t bufferSize)
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
    else if (signum == SIGUSR1)
    {
        m_signal = 2;
    }
}

static void handle_sigint(const string_nodePtr countries, const int count, const int err)
{
    string_nodePtr node = countries;
    FILE *filePtr = NULL;

    char path[100];

    sprintf(path, "./logs/log_file%d", getpid());

    if ((filePtr = fopen(path, "w+")) == NULL)
    {
        perror("open file");
    }

    while (node != NULL)
    {
        fprintf(filePtr, "%s\n", node->str);
        node = node->next;
    }

    fprintf(filePtr, "\nTotal: %d\nSuccessful: %d\nError: %d", count + err, count, err);

    fclose(filePtr);

    printf("worker: %d recieved siganl\n", getpid());
}

static int handle_sigusr1(ListPtr list, HashTablePtr h1, HashTablePtr h2, const string_nodePtr countries, string_nodePtr dates, const char *input_dir)
{
    wordexp_t p;
    int flag = 1;
    size_t len = 0;
    DIR *dirp = NULL;
    FILE *filePtr = NULL;
    ListNodePtr node = NULL;
    PatientPtr patient = NULL;
    struct dirent *dir_info = NULL;
    string_nodePtr country = countries, tmp = NULL;
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
            if (!(strcmp(dir_info->d_name, ".") == 0 || strcmp(dir_info->d_name, "..") == 0))
            {
                if ((file = Worker_getPath(path, dir_info->d_name)) == NULL)
                {
                    perror("Worker_getPath()");
                    return -1;
                }

                flag = 1;
                tmp = dates;
                while (tmp != NULL)
                {
                    if (!strcmp(tmp->str, file))
                    {
                        flag = 0;
                    }
                    tmp = tmp->next;
                }

                if (flag)
                {
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

                    fclose(filePtr);
                    free(line);
                }

                free(file);
            }
        }

        free(path);
        closedir(dirp);

        country = country->next;
    }

    return 0;
}

static int Worker_Run(ListPtr list, HashTablePtr h1, HashTablePtr h2, const string_nodePtr countries, string_nodePtr *dates, const int w_fd, const size_t bufferSize, const char *input_dir)
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

                if ((*dates = add_stringNode(*dates, file)) == NULL)
                {
                    printf("add_stringNode() failed");
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

static int Worker_wait_input(const int w_fd, const int r_fd, char *buffer, const size_t bufferSize, const ListPtr list, const HashTablePtr h1, const HashTablePtr h2, const string_nodePtr countries, string_nodePtr dates, const char *input_dir)
{
    int count = 0, err = 0;
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
                    handle_sigint(countries, count, err);
                    break;
                }
                else if (m_signal == 2)
                {
                    handle_sigusr1(list, h1, h2, countries, dates, input_dir);
                }
            }
            else
            {
                perror("pselect");
            }
        }
        else
        {
            str = decode(r_fd, buffer, bufferSize);

            wordexp(str, &p, 0);

            if (!strcmp(p.we_wordv[0], "/exit"))
            {
                free(str);
                wordfree(&p);
                break;
            }
            else if (!strcmp(p.we_wordv[0], "/diseaseFrequency"))
            {
                if (diseaseFrequency(w_fd, bufferSize, str, h1) == -1)
                {
                    err++;
                }
                else
                {
                    count++;
                }
            }
            else if (!strcmp(p.we_wordv[0], "/topk-AgeRanges"))
            {
                if (topk_AgeRanges(w_fd, bufferSize, str, list) == -1)
                {
                    err++;
                }
                else
                {
                    count++;
                }
            }
            else if (!strcmp(p.we_wordv[0], "/searchPatientRecord"))
            {
                if (searchPatientRecord(&p, w_fd, bufferSize, list) == -1)
                {
                    err++;
                }
                else
                {
                    count++;
                }
            }
            else if (!strcmp(p.we_wordv[0], "/numPatientAdmissions"))
            {
                if (numFunctions(w_fd, bufferSize, str, h2, list, ENTER) == -1)
                {
                    err++;
                }
                else
                {
                    count++;
                }
            }
            else if (!strcmp(p.we_wordv[0], "/numPatientDischarges"))
            {
                if (numFunctions(w_fd, bufferSize, str, h2, list, EXIT) == -1)
                {
                    err++;
                }
                else
                {
                    count++;
                }
            }

            wordfree(&p);
            free(str);
        }
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
        return -1;
    }

    sprintf(answ, "%d", AVLNode_countPatients(tree->root, p.we_wordv[1], p.we_wordv[4], d1, d2));
    encode(w_fd, answ, bufferSize);

    free(d1);
    free(d2);
    wordfree(&p);

    return 0;
}

static int topk_AgeRanges(const int w_fd, const size_t bufferSize, const char *str, const ListPtr list)
{
    int total = 0;
    ageInfoPtr ag = NULL;
    wordexp_t p;
    ListNodePtr ptr = list->head;
    DatePtr d1 = NULL, d2 = NULL;

    wordexp(str, &p, 0);

    if ((d1 = Date_Init(p.we_wordv[4])) == NULL || (d2 = Date_Init(p.we_wordv[5])) == NULL)
    {
        return -1;
    }

    ag = ageInfo_Init();

    while (ptr != NULL)
    {
        if (!strcmp(ptr->patient->country, p.we_wordv[2]))
        {
            if (!strcmp(ptr->patient->diseaseID, p.we_wordv[3]))
            {
                if (Date_Compare(d1, ptr->patient->entryDate) <= 0 && Date_Compare(d2, ptr->patient->entryDate) >= 0)
                {
                    ageInfo_add(ag, ptr->patient->age);
                }
            }
        }
        ptr = ptr->next;
    }

    total = ag->ag1 + ag->ag2 + ag->ag3 + ag->ag4;

    for (int i = 0; i < atoi(p.we_wordv[1]); i++)
    {
        char str[100] = {'\0'};

        if (ag->ag1 >= ag->ag2 && ag->ag1 >= ag->ag3 && ag->ag1 >= ag->ag4)
        {
            sprintf(str, "0-20: %f%%\n", ((float)ag->ag1 / total) * 100);
            ag->ag1 = 0;
        }
        else if (ag->ag2 >= ag->ag1 && ag->ag2 >= ag->ag3 && ag->ag2 >= ag->ag4)
        {
            sprintf(str, "21-40: %f%%\n", ((float)ag->ag2 / total) * 100);
            ag->ag2 = 0;
        }
        else if (ag->ag3 >= ag->ag1 && ag->ag3 >= ag->ag2 && ag->ag3 >= ag->ag4)
        {
            sprintf(str, "41-60: %f%%\n", ((float)ag->ag3 / total) * 100);
            ag->ag3 = 0;
        }
        else
        {
            sprintf(str, "61+: %f%%\n", ((float)ag->ag4 / total) * 100);
            ag->ag4 = 0;
        }

        encode(w_fd, str, bufferSize);
    }

    encode(w_fd, "OK", bufferSize);

    free(ag);
    free(d1);
    free(d2);
    wordfree(&p);

    return 0;
}

static int searchPatientRecord(wordexp_t *p, const int w_fd, const size_t bufferSize, const ListPtr list)
{
    PatientPtr patient = NULL;

    if ((patient = getPatientById(p->we_wordv[1], list)) != NULL)
    {
        char str[1000], d1[12], d2[12];

        sprintf(str, "%s %s %s %s %s %s ", patient->id, patient->fName, patient->lName, patient->country, patient->diseaseID, patient->age);
        sprintf(d1, "%d-%d-%d ", patient->entryDate->day, patient->entryDate->month, patient->entryDate->year);
        strcat(str, d1);

        if (patient->exitDate != NULL)
        {
            sprintf(d2, "%d-%d-%d ", patient->exitDate->day, patient->exitDate->month, patient->exitDate->year);
            strcat(str, d2);
        }
        else
        {
            strcat(str, "-");
        }

        encode(w_fd, str, bufferSize);
    }
    else
    {
        encode(w_fd, "OK", bufferSize);
    }

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
                            char send[100];
                            sprintf(send, "%s %d\n", node->entries[j]->key, count);
                            encode(w_fd, send, bufferSize);
                            // printf("%s %d\n", node->entries[j]->key, count);
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
                            char send[100];
                            sprintf(send, "%s %d\n", node->entries[j]->key, count);
                            encode(w_fd, send, bufferSize);
                            // printf("%s %d\n", node->entries[j]->key, count);
                        }
                    }
                    node = node->next;
                }
            }
        }

        encode(w_fd, "OK", bufferSize);
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

        // printf("%s %d\n", p.we_wordv[4], count);
        char send[100];
        sprintf(send, "%s %d\n", p.we_wordv[4], count);
        encode(w_fd, send, bufferSize);
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
