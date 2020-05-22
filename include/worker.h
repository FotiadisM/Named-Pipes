#ifndef WORKER_H
#define WORKER_H

#include <stdio.h>

typedef struct toDo
{
    char *country;
    int status;
    struct toDo *next;
} toDo;

typedef toDo *toDoPtr;

int Worker(const size_t bufferSize, const char *input_dir);

int Worker_GetCountries(toDoPtr *toDos, const int r_fd, const int w_fd, char *buffer, const size_t bufferSize);

void Worker_handleSignals(struct sigaction *act);

int Worker_Run(toDoPtr toDos, char *buffer, const size_t bufferSize, const char *input_dir);

char *Worker_getPath(const char *input_dir, const char *country);

toDoPtr add_toDo(toDoPtr toDos, char *country);

void clear_toDos(toDoPtr toDos);

#endif