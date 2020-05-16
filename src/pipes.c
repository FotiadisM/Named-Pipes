#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

int Pipe_Init(const char *path, const int pid, const int flags)
{
    int fd = 0;
    char str[12] = {'\0'};
    char path_f[24] = {'\0'};

    if (sprintf(str, "%d", pid) == -1)
    {
        perror("sprintf");
        return -1;
    }

    if (strcpy(path_f, path) == NULL)
    {
        perror("strcpy");
        return -1;
    }

    strcat(path_f, str);

    if (mkfifo(path_f, 0640) == -1)
    {
        if (errno != EEXIST)
        {
            perror("mkfifo");
            return -1;
        }
    }

    if ((fd = open(path_f, flags)) == -1)
    {
        perror("open");
        return -1;
    }

    return fd;
}