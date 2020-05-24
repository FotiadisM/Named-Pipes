#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
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

int encode(const int fd, const char *buffer, const size_t bufferSize)
{
    char str[12] = {0};
    size_t str_size = sizeof(str);
    size_t buffer_len = strlen(buffer) + 1;

    sprintf(str, "%zu", buffer_len);

    for (size_t i = 0; i < str_size / bufferSize; i++)
    {
        write(fd, str + (i * bufferSize), bufferSize);
    }

    if (str_size % bufferSize)
    {
        write(fd, str + (str_size - str_size % bufferSize), bufferSize);
    }

    for (size_t i = 0; i < buffer_len / bufferSize; i++)
    {
        write(fd, buffer + (i * bufferSize), bufferSize);
    }

    if (buffer_len % bufferSize)
    {
        write(fd, buffer + (buffer_len - buffer_len % bufferSize), bufferSize);
    }

    return 0;
}

char *decode(const int fd, char *buffer, const size_t bufferSize)
{
    char *r_buffer = NULL;
    char str[12] = {0};
    size_t str_size = sizeof(str);
    size_t r_buffer_size = 0;

    for (size_t i = 0; i < str_size / bufferSize; i++)
    {
        read(fd, buffer, bufferSize);
        memcpy(str + (i * bufferSize), buffer, bufferSize);
    }

    if (str_size % bufferSize)
    {
        read(fd, buffer, bufferSize);
        memcpy(str + (str_size / bufferSize) * bufferSize, buffer, str_size - (str_size / bufferSize) * bufferSize);
    }

    r_buffer_size = (size_t)strtol(str, NULL, 10);

    if ((r_buffer = malloc(r_buffer_size)) == NULL)
    {
        perror("malloc");
        return NULL;
    }

    for (size_t i = 0; i < r_buffer_size / bufferSize; i++)
    {
        read(fd, buffer, bufferSize);
        memcpy(r_buffer + (i * bufferSize), buffer, bufferSize);
    }

    if (r_buffer_size % bufferSize)
    {
        read(fd, buffer, bufferSize);
        memcpy(r_buffer + (r_buffer_size / bufferSize) * bufferSize, buffer, r_buffer_size - (r_buffer_size / bufferSize) * bufferSize);
    }

    return r_buffer;
}