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
#include "../include/patient.h"

static void handler(int signum);
static string_nodePtr Worker_GetCountries(const int r_fd, const int w_fd, char *buffer, const size_t bufferSize);
static void Worker_handleSignals(struct sigaction *act);
static int Worker_Run(string_nodePtr countries, const int r_fd, const int w_fd, char *buffer, const size_t bufferSize, const char *input_dir);
static char *Worker_getPath(const char *input_dir, const char *country);

volatile sig_atomic_t m_signal = 0;

int Worker(const size_t bufferSize, const char *input_dir)
{
    int r_fd = 0, w_fd = 0;
    char *buffer = NULL;
    struct sigaction *act = NULL;
    string_nodePtr countries = NULL;

    if ((w_fd = Pipe_Init("./pipes/r_", getpid(), O_WRONLY)) == -1)
    {
        printf("Pipe_Init() failed\n");
        return -1;
    }

    if ((r_fd = Pipe_Init("./pipes/w_", getpid(), O_RDONLY)) == -1)
    {
        printf("Pipe_Init() failed");
        return -1;
    }

    if ((buffer = malloc(bufferSize)) == NULL)
    {
        perror("malloc");
        return -1;
    }

    if ((act = malloc(sizeof(struct sigaction))) == NULL)
    {
        perror("malloc");
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

    if (Worker_Run(countries, r_fd, w_fd, buffer, bufferSize, input_dir) == -1)
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

static int Worker_Run(string_nodePtr countries, const int r_fd, const int w_fd, char *buffer, const size_t bufferSize, const char *input_dir)
{
    wordexp_t p;
    size_t len = 0;
    DIR *dirp = NULL;
    FILE *filePtr = NULL;
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
                        if ((patient = Patient_Init(p.we_wordv[0], p.we_wordv[2], p.we_wordv[3], p.we_wordv[5], p.we_wordv[4], country->str, file)) == NULL)
                        {
                            printf("Patient_Init() failed");
                            return -1;
                        }
                        patient->exit = NULL;

                        Patient_Close(patient);
                    }
                    else
                    {
                    }

                    wordfree(&p);
                }

                free(file);
                fclose(filePtr);
            }
        }

        free(path);
        closedir(dirp);

        country = country->next;
    }

    free(line);

    return 0;
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

static void handler(int signum)
{
    m_signal = signum;
}
