#ifndef PERSON_H
#define PERSON_H

#include <stdio.h>

#include "./date.h"

#define ENTER 1
#define EXIT 0

typedef struct Patient
{
    char *id;
    char *fName;
    char *lName;
    char *age;
    char *disease;
    char *country;
    DatePtr entry;
    DatePtr exit;
} Patient;

typedef Patient *PatientPtr;

PatientPtr Patient_Init(
    const char *id,
    const char *fName,
    const char *lName,
    const char *age,
    const char *disease,
    const char *country,
    const char *date);

void Patient_Close(PatientPtr p);

PatientPtr Patient_getPatient(FILE *filePtr, const char *country, const char *entry);

int Patient_Compare(PatientPtr p1, PatientPtr p2);

#endif