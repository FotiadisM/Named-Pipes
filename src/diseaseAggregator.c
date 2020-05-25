#include <sys/select.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

#include "../include/diseaseAggregator.h"
#include "../include/pipes.h"

static int DA_DevideWork(worker_infoPtr workers_array, const int numWorkers, const size_t bufferSize, const char *input_dir);
static int DA_main(worker_infoPtr workers_array, const int numWorkers, const size_t bufferSize);

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

    // if (DA_main(workers_array, numWorkers, bufferSize) == -1)
    // {
    //     printf("DA_main() failed");
    //     return -1;
    // }

    free(buffer);

    return 0;
}

static int DA_DevideWork(worker_infoPtr workers_array, const int numWorkers, const size_t bufferSize, const char *input_dir)
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
            encode(workers_array[count].w_fd, dir_info->d_name, bufferSize);

            if ((workers_array[count].countries_list = add_stringNode(workers_array[count].countries_list, dir_info->d_name)) == NULL)
            {
                printf("add_string_node failed");
                return -1;
            }

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
        if (flag && i >= count)
        {
            write(workers_array[count].w_fd, "/exit", bufferSize);
        }
        else
        {
            encode(workers_array[i].w_fd, "OK", bufferSize);
        }
    }

    if (closedir(dirp) == -1)
    {
        perror("closedirp");
    }

    return 0;
}

static int DA_main(worker_infoPtr workers_array, const int numWorkers, const size_t bufferSize)
{
    fd_set fds;
    int maxfd = 0;
    char *buffer = NULL, *str = NULL;

    if ((buffer = malloc(bufferSize)) == NULL)
    {
        perror("malloc");
        return -1;
    }

    while (1)
    {
        FD_ZERO(&fds);

        for (int i = 0; i < numWorkers; i++)
        {
            FD_SET(workers_array[i].r_fd, &fds);

            if (workers_array[i].r_fd > maxfd)
            {
                maxfd = workers_array[i].r_fd;
            }
        }

        if (pselect(maxfd + 1, &fds, NULL, NULL, NULL, NULL) == -1)

        {
            perror("select()");
        }

        for (int i = 0; i < numWorkers; i++)
        {
            if (FD_ISSET(workers_array[i].r_fd, &fds))
            {
                str = decode(workers_array[i].r_fd, buffer, bufferSize);
            }
        }

        free(str);
    }

    free(buffer);

    return 0;
}
