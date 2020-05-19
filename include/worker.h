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

int Worker_Run(const size_t bufferSize, const char *input_dir);

toDoPtr add_toDo(toDoPtr toDos, char *country);

void clear_toDos(toDoPtr toDos);

#endif