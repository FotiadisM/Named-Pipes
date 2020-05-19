#ifndef DATE_H
#define DATE_H

typedef struct Date
{
    int day;
    int month;
    int year;
} Date;

typedef Date *DatePtr;

DatePtr Date_Init(const char *info);

int Date_Compare(const DatePtr d1, const DatePtr d2);

void Date_Print(const DatePtr date);

#endif