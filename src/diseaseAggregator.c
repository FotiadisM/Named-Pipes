#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
// #include <errno.h>

#include "../include/diseaseAggregator.h"

// int DA_Init(const int numWorkers, size_t bufferSize, const char *input_dir)
// {
//     // int *rpipes = NULL;
//     // int *wpipes = NULL;

//     if (Pipes_Init(numWorkers) == -1)
//     {
//         printf("Pipes_Init() failed\n");
//         return -1;
//     }

//     return 0;
// }

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

int Worker_Run(char *input_dir, size_t bufferSize)
{
    int rfd = 0, wfd = 0;
    char str[12] = {'\0'};
    char path[22] = "./pipes/r_";

    if (sprintf(str, "%d", getpid()) == -1)
    {
        perror("sprintf");
    }
    strcat(path, str);

    if ((rfd = open(path, O_WRONLY)) == -1)
    {
        perror("open");
        return -1;
    }
    printf("pipe oppened\n");

    return 0;
}