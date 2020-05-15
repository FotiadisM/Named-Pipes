#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

#include "../include/diseaseAggregator.h"

int Pipe_Init(int pid)
{
    char str[12] = {'\0'};
    char path[22] = "./pipes/r_";

    if (sprintf(str, "%d", pid) == -1)
    {
        perror("sprintf");
        return -1;
    }
    strcat(path, str);

    if (mkfifo(path, 0640) == -1)
    {
        perror("mkfifo");
        return -1;
    }

    path[8] = 'w';
    if (mkfifo(path, 0640) == -1)
    {
        perror("mkfifo");
        return -1;
    }

    return 0;
}

int *Pipes_Open(const int *pid_array, const int numWorkers, const char *path, const int flags)
{
    int fd = 0, *fd_array = NULL;

    if ((fd_array = malloc(numWorkers * sizeof(int))) == NULL)
    {
        perror("malloc");
        return NULL;
    }

    for (int i = 0; i < numWorkers; i++)
    {
        char pid_str[12] = {'\0'};
        char f_path[22] = {'\0'};

        if (sprintf(pid_str, "%d", pid_array[i]) == -1)
        {
            perror("sprintf");
            return NULL;
        }

        strcpy(f_path, path);

        strcat(f_path, pid_str);

        if ((fd = open(f_path, flags)) == -1)
        {
            perror("open");
            return NULL;
        }
        fd_array[i] = fd;
    }

    return fd_array;
}

int DA_Run(const int *pid_array, const int numWorkers, const char *input_dir, const size_t bufferSize)
{
    char *buffer = NULL;
    int *r_fds = NULL, *w_fds = NULL;

    if ((buffer = malloc(bufferSize)) == NULL)
    {
        perror("malloc");
    }

    if ((w_fds = Pipes_Open(pid_array, numWorkers, "./pipes/r_", O_WRONLY)) == NULL)
    {
        printf("Pipes_Open failed");
        return -1;
    }

    if ((r_fds = Pipes_Open(pid_array, numWorkers, "./pipes/w_", O_RDONLY)) == NULL)
    {
        printf("Pipes_Open failed");
        return -1;
    }

    for (int i = 0; i < numWorkers; i++)
    {
        write(w_fds[i], "hello", bufferSize);
    }

    for (int i = 0; i < numWorkers; i++)
    {
        if (close(r_fds[i]) == -1)
        {
            perror("close");
        }

        if (close(w_fds[i]) == -1)
        {
            perror("close");
        }
    }

    free(r_fds);
    free(w_fds);
    free(buffer);

    return 0;
}
