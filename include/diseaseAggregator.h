#ifndef DISEASEAGGREGATOR_H
#define DISEASEAGGREGATOR_H

#include <stdio.h>

typedef struct string_node
{
    char *str;
    struct string_node *next;
} string_node;

typedef string_node *string_nodePtr;

typedef struct worker_info
{
    pid_t pid;
    int r_fd;
    int w_fd;
    string_nodePtr countries_list;
} worker_info;

typedef worker_info *worker_infoPtr;

int DA_Run(worker_infoPtr workkers_array, const int numWorkers, const size_t bufferSize, const char *input_dir);

int DA_DevideWork(worker_infoPtr workers_array, const int numWorkers, const size_t bufferSize, const char *input_dir);

#endif