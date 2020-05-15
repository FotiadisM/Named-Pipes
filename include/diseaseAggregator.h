#ifndef DISEASEAGGREGATOR_H
#define DISEASEAGGREGATOR_H

#include <stdio.h>

// int DA_Init(const int numWorkers, size_t bufferSize, const char *input_dir);

int Pipe_Init(int pid);

int *Pipes_Open(const int *pids, const int numWorkers, const char *path, const int flags);

int DA_Run(const int *pids, const int numWorkers, const char *input_dir, const size_t bufferSize);

#endif