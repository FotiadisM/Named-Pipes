#ifndef DISEASEAGGREGATOR_H
#define DISEASEAGGREGATOR_H

#include <stdio.h>
#include "../include/list.h"

typedef struct worker_info
{
    pid_t pid;
    int r_fd;
    int w_fd;
    string_nodePtr countries_list;
} worker_info;

typedef worker_info *worker_infoPtr;

int DA_Run(worker_infoPtr workkers_array, const int numWorkers, const size_t bufferSize, char *input_dir);

#endif