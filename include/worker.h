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

typedef struct ageInfo
{
    int ag1;
    int ag2;
    int ag3;
    int ag4;
} ageInfo;

typedef ageInfo *ageInfoPtr;

typedef struct stats
{
    char *disease;
    ageInfoPtr ag;
    struct stats *next;
} stats;

typedef stats *statsPtr;

int Worker(const size_t bufferSize, const char *input_dir);

#endif