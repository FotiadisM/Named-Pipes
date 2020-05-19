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
        char buffer[24] = {'\0'};

        if (!(strcmp(dir_info->d_name, ".") == 0 || strcmp(dir_info->d_name, "..") == 0))
        {
            string_nodePtr node = NULL;

            write(workers_array[count].w_fd, dir_info->d_name, strlen(dir_info->d_name) + 1);

            if ((node = malloc(sizeof(string_node))) == NULL)
            {
                perror("malloc");
                return -1;
            }

            if ((node->str = malloc(strlen(dir_info->d_name) + 1)) == NULL)
            {
                perror("malloc");
                return -1;
            }

            strcpy(node->str, dir_info->d_name);
            node->next = workers_array[count].countries_list;
            workers_array[count].countries_list = node;

            read(workers_array[count].r_fd, buffer, bufferSize);
            // need to handle err

            if (++count == numWorkers)
            {
                flag = 0;
                count = 0;
            }
        }
    }

    for (int i = 0; i < numWorkers; i++)
    {
        // Need to handle DrisNumber < numWorkers case

        // if (flag && i >= count)
        // {
        //     write(workers_array[count].w_fd, "/exit", bufferSize);
        // }
        // else
        // {
        write(workers_array[i].w_fd, "OK", bufferSize);
        // }
    }

    if (closedir(dirp) == -1)
    {
        perror("closedirp");
    }

    return 0;
}