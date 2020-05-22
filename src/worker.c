#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

#include "../include/worker.h"
#include "../include/pipes.h"

int Worker(const size_t bufferSize, const char *input_dir)
{
    int r_fd = 0, w_fd = 0;
    char *buffer = NULL;
    toDoPtr toDos = NULL;
    struct sigaction *act = NULL;

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

    if (Worker_GetCountries(&toDos, r_fd, w_fd, buffer, bufferSize) == -1)
    {
        if (close(r_fd) == -1 || close(w_fd) == -1)
        {
            perror("close");
        }

        free(buffer);

        return -1;
    }

    Worker_handleSignals(act);

    if (Worker_Run(toDos, buffer, bufferSize, input_dir) == -1)
    {
        printf("Worker_Run() failed\n");
    }

    if (close(r_fd) == -1 || close(w_fd) == -1)
    {
        perror("close");
    }

    free(act);
    free(buffer);
    clear_toDos(toDos);

    return 0;
}

int Worker_GetCountries(toDoPtr *toDos, const int r_fd, const int w_fd, char *buffer, const size_t bufferSize)
{
    while (1)
    {
        read(r_fd, buffer, bufferSize);

        if (strcmp(buffer, "OK") == 0)
        {
            break;
        }

        else if (strcmp(buffer, "/exit") == 0)
        {
            if (close(r_fd) == -1 || close(w_fd) == -1)
            {
                perror("close");
            }

            free(buffer);

            return -1;
        }

        else
        {
            if ((*toDos = add_toDo(*toDos, buffer)) == NULL)
            {
                write(w_fd, "ERR", strlen("ERR") + 1);
            }
            else
            {
                write(w_fd, "OK", strlen("OK") + 1);
            }
        }
    }

    return 0;
}

int hello(int signum)
{
    printf("%d_hello\n", signum);

    return 0;
}

void Worker_handleSignals(struct sigaction *act)
{
    act->sa_handler = (void *)hello;

    sigaction(SIGINT, act, NULL);
}

int Worker_Run(toDoPtr toDos, char *buffer, const size_t bufferSize, const char *input_dir)
{
    char *path;
    toDoPtr toDo = toDos;

    while (toDo != NULL)
    {
        int fd = 0;
        DIR *dirp = NULL;
        struct dirent *dir_info = NULL;

        if ((path = Worker_getPath(input_dir, toDo->country)) == NULL)
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
            char *file = NULL;

            if (!(strcmp(dir_info->d_name, ".") == 0 || strcmp(dir_info->d_name, "..") == 0))
            {
                if ((file = Worker_getPath(path, dir_info->d_name)) == NULL)
                {
                    perror("Worker_getPath()");
                    return -1;
                }

                if ((fd = open(file, O_RDONLY)) == -1)
                {
                    perror("open()");
                    return -1;
                }

                // read file etc

                close(fd);

                free(file);
            }
        }

        free(path);
        closedir(dirp);

        toDo->status = 1;
        toDo = toDo->next;
    }

    return 0;
}

char *Worker_getPath(const char *input_dir, const char *country)
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

toDoPtr add_toDo(toDoPtr toDos, char *country)
{
    toDoPtr newToDo = NULL;

    if ((newToDo = malloc(sizeof(toDo))) == NULL)
    {
        perror("malloc");
        return NULL;
    }

    if ((newToDo->country = malloc(strlen(country) + 1)) == NULL)
    {
        perror("malloc");
        return NULL;
    }

    strncpy(newToDo->country, country, strlen(country) + 1);
    newToDo->status = 0;

    newToDo->next = toDos;
    toDos = newToDo;

    return newToDo;
}

void clear_toDos(toDoPtr toDos)
{
    toDoPtr tmpToDo = NULL;

    while (toDos != NULL)
    {
        tmpToDo = toDos;
        toDos = toDos->next;

        free(tmpToDo->country);
        free(tmpToDo);
    }
}