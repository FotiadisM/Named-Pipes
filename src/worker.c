#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "../include/worker.h"

int Worker_Run(char *input_dir, size_t bufferSize)
{
    int rfd = 0, wfd = 0;
    char buffer[bufferSize];
    char str[12] = {'\0'};
    char path[22] = "./pipes/r_";

    if (sprintf(str, "%d", getpid()) == -1)
    {
        perror("printf");
    }
    strcat(path, str);

    if ((rfd = open(path, O_RDONLY)) == -1)
    {
        perror("open");
        return -1;
    }

    path[8] = 'w';
    if ((wfd = open(path, O_WRONLY)) == -1)
    {
        perror("open");
        return -1;
    }

    printf("pipe oppened\n");

    read(rfd, buffer, bufferSize);
    printf("%s\n", buffer);

    if (close(rfd) == -1 || close(wfd) == -1)
    {
        perror("close");
    }

    return 0;
}
