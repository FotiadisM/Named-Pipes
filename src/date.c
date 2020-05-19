#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/date.h"

DatePtr Date_Init(const char *info)
{
    char *str = NULL;
    DatePtr date = NULL;

    if ((date = malloc(sizeof(Date))) == NULL)
    {
        perror("malloc failed");
        return NULL;
    }

    if ((str = malloc(strlen(info) + 1)) == NULL)
    {
        perror("malloc failed");
        free(date);
        return NULL;
    }

    strcpy(str, info);

    date->day = atoi(strtok(str, "-"));
    date->month = atoi(strtok(NULL, "-"));
    date->year = atoi(strtok(NULL, " "));

    free(str);

    return date;
}

int Date_Compare(const DatePtr d1, const DatePtr d2)
{
    if (d1->year < d2->year)
    {
        return -1;
    }
    else if (d1->year > d2->year)
    {
        return 1;
    }
    else
    {
        if (d1->month < d2->month)
        {
            return -1;
        }
        else if (d1->month > d2->month)
        {
            return 1;
        }
        else
        {
            if (d1->day < d2->day)
            {
                return -1;
            }
            else if (d1->day > d2->day)
            {
                return 1;
            }
            else
            {
                return 0;
            }
        }
    }
}

void Date_Print(const DatePtr date)
{
    printf("%d-%d-%d", date->day, date->month, date->year);
}