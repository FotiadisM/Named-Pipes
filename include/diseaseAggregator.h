#ifndef DISEASEAGGREGATOR_H
#define DISEASEAGGREGATOR_H

#include <stdio.h>

// int DA_Init(const int numWorkers, size_t bufferSize, const char *input_dir);

int Pipe_Init(int pid);

int Worker_Run(char *input_dir, size_t bufferSize);

int DA_Run(char *input_dir, size_t bufferSize);

#endif