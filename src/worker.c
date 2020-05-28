#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
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

static void Worker_handleSignals(struct sigaction *act);
static void handler(int signum);

static int validatePatient(const wordexp_t *p, const char *country, const ListPtr list);
static PatientPtr getPatient(const wordexp_t *p, const char *country, const ListPtr list);
static char *Worker_getPath(const char *input_dir, const char *country);

static int Worker_sendStatistics(const statsPtr st, const int w_fd, const size_t bufferSize, const char *country, const char *file);

static ageInfoPtr ageInfo_Init();
static void ageInfo_add(ageInfoPtr ag, const char *age_str);
static statsPtr stats_Init();
static statsPtr stats_add(statsPtr st, const char *disease, const char *age_str);
static void stats_close(statsPtr st);

int Worker(const size_t bufferSize, const char *input_dir)
{
    int r_fd = 0, w_fd = 0;
    char *buffer = NULL;
    struct sigaction *act = NULL;
    string_nodePtr countries = NULL;
    ListPtr list = NULL;
    HashTablePtr diseaseHT = NULL, countryHT = NULL;

    if (Worker_Init(&w_fd, &r_fd, &buffer, bufferSize, &list, &diseaseHT, &countryHT, &act) == -1)
    {
        printf("Worker_Init() failed");
        return -1;
    }

    if ((countries = Worker_GetCountries(r_fd, w_fd, buffer, bufferSize)) == NULL)
    {
        if (close(r_fd) == -1 || close(w_fd) == -1)
        {
            perror("close");
        }

        free(buffer);

        return -1;
    }

    // Worker_handleSignals(act);

    if (Worker_Run(list, diseaseHT, countryHT, countries, w_fd, bufferSize, input_dir) == -1)
    {
        printf("Worker_Run() failed\n");
    }

    if (close(r_fd) == -1 || close(w_fd) == -1)
    {
        perror("close");
    }

    free(act);
    free(buffer);
    clear_stringNode(countries);

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
            if (close(r_fd) == -1 || close(w_fd) == -1)
            {
                perror("close");
            }

            free(buffer);
            free(str);

            exit(EXIT_SUCCESS);
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

    sigaction(SIGINT, act, NULL);
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
                        if (validatePatient(&p, country->str, list))
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

                free(file);
                fclose(filePtr);
            }

            stats_close(st);
        }

        free(path);
        closedir(dirp);

        country = country->next;
    }

    free(line);

    return 0;
}

static void handler(int signum)
{
    m_signal = signum;
}

static int validatePatient(const wordexp_t *p, const char *country, const ListPtr list)
{
    ListNodePtr node = list->head;

    while (node != NULL)
    {
        if (Patient_Compare(node->patient, p->we_wordv[0], p->we_wordv[2], p->we_wordv[3], p->we_wordv[4], country, p->we_wordv[5]))
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
    char *date = NULL;

    if ((date = strdup(file)) == NULL)
    {
        perror("strdup");
        return -1;
    }
    strtok(date, ".");

    statsPtr tmp = st;
    while (tmp != NULL)
    {
        printf("%s %s %d %d %d %d\n", date, st->disease, st->ag->ag1, st->ag->ag2, st->ag->ag3, st->ag->ag4);
        tmp = tmp->next;
    }

    free(date);

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