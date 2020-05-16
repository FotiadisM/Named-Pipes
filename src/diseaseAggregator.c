#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

#include "../include/diseaseAggregator.h"

int DA_Run(worker_infoPtr workers_array, const int numWorkers, const size_t bufferSize, const char *input_dir)
{
    char *buffer = NULL;

    if ((buffer = malloc(bufferSize)) == NULL)
    {
        perror("malloc");
        return -1;
    }

    if (DA_DevideWork(workers_array, numWorkers, bufferSize, input_dir) == -1)
    {
        printf("DA_DevideWork() failed\n");
        return -1;
    }

    free(buffer);

    return 0;
}

int DA_DevideWork(worker_infoPtr workers_array, const int numWorkers, const size_t bufferSize, const char *input_dir)
{
    int count = 0, flag = 1;
    DIR *dirp = NULL;
    struct dirent *dir_info = NULL;

    if ((dirp = opendir(input_dir)) == NULL)
    {
        perror("opendir()");
        return -1;
    }

    while ((dir_info = readdir(dirp)) != NULL)
    {
        if (!(strcmp(dir_info->d_name, ".") == 0 || strcmp(dir_info->d_name, "..") == 0))
        {
            write(workers_array[count].w_fd, dir_info->d_name, bufferSize);

            if (++count == numWorkers)
            {
                flag = 0;
                count = 0;
            }
        }
    }

    for (int i = 0; i < numWorkers; i++)
    {
        if (flag && i >= count)
        {
            write(workers_array[count].w_fd, "/exit", bufferSize);
        }
        else
        {
            write(workers_array[count].w_fd, "OK", bufferSize);
        }
    }

    return 0;
}