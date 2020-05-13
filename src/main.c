#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>

#include "../include/diseaseAggregator.h"

int main(int argc, char *argv[])
{
    pid_t pid = 0;
    size_t bufferSize = 0;
    char *input_dir = NULL;
    int numWorkers = 0, opt = 0;

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
            if (Worker_Run(input_dir, bufferSize) == -1)
            {
                printf("worker failed\n");
            }
            free(input_dir);
            exit(0);
        }

        else
        {
            Pipe_Init(pid);
        }
    }

    if (DA_Run(input_dir, bufferSize) == -1)
    {
        printf("Program failed, exiting");
    }

    free(input_dir);

    return 0;
}