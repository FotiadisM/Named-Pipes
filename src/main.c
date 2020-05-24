#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <wait.h>
#include <string.h>

#include "../include/diseaseAggregator.h"
#include "../include/worker.h"
#include "../include/pipes.h"

int main(int argc, char *argv[])
{
    pid_t pid = 0;
    size_t bufferSize = 0;
    worker_infoPtr workers_array = NULL;
    int numWorkers = 0, opt = 0;
    char *input_dir = NULL;

    while ((opt = getopt(argc, argv, "w:b:i:")) != -1)
    {
        switch (opt)
        {
        case 'w':
            if ((numWorkers = atoi(optarg)) == 0)
            {
                printf("Invalid value: %s\n", optarg);
            }
            break;
        case 'b':
            if ((bufferSize = atoi(optarg)) == 0)
            {
                printf("Invalid value: %s\n", optarg);
            }
            break;
        case 'i':
            if ((input_dir = malloc(strlen(optarg) + 1)) == NULL)
            {
                perror("malloc");
            }
            strcpy(input_dir, optarg);
            break;
        case '?':
            fprintf(stderr, "Usage: ./diseaseAggregator –w <numWorkers> -b <bufferSize> -i <input_dir>\n");
            return -1;
        }
    }

    if (optind > argc)
    {
        fprintf(stderr, "Usage: ./diseaseAggregator –w <numWorkers> -b <bufferSize> -i <input_dir>\n");
        return -1;
    }

    if ((workers_array = malloc(numWorkers * sizeof(worker_info))) == NULL)
    {
        perror("malloc");
        return -1;
    }

    for (int i = 0; i < numWorkers; i++)
    {
        workers_array[i].pid = 0;
        workers_array[i].r_fd = 0;
        workers_array[i].w_fd = 0;
        workers_array[i].countries_list = NULL;
    }

    for (int i = 0; i < numWorkers; i++)
    {
        pid = fork();

        if (pid == -1)
        {
            perror("fork failed");
            return -1;
        }

        else if (pid == 0)
        {
            free(workers_array);

            if (Worker(bufferSize, input_dir) == -1)
            {
                printf("worker exiting\n");
                exit(EXIT_FAILURE);
            }

            free(input_dir);

            exit(EXIT_SUCCESS);
        }

        else
        {
            workers_array[i].pid = pid;

            if ((workers_array[i].r_fd = Pipe_Init("./pipes/r_", pid, O_RDONLY)) == -1)
            {
                printf("Pipe_Init() failed, exiting");
            }

            if ((workers_array[i].w_fd = Pipe_Init("./pipes/w_", pid, O_WRONLY)) == -1)
            {
                printf("Pipe_Init() failed, exiting");
            }
        }
    }

    if (DA_Run(workers_array, numWorkers, bufferSize, input_dir) == -1)
    {
        printf("DA_Run() failed, exiting");
    }

    for (int i = 0; i < numWorkers; i++)
    {
        wait(NULL);

        clear_stringNode(workers_array[i].countries_list);
    }

    free(workers_array);
    free(input_dir);

    return 0;
}