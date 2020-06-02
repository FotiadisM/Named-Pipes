#ifndef WORKER_H
#define WORKER_H

#include <stdio.h>

#include "./list.h"

typedef struct toDo
{
    char *country;
    int status;
    struct toDo *next;
} toDo;

typedef toDo *toDoPtr;

int Worker(const size_t bufferSize, char *input_dir);

#endif