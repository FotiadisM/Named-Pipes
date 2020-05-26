#ifndef PERSON_H
#define PERSON_H

#include "./date.h"

#define ENTER 1
#define EXIT 0

typedef struct Patient
{
    char *id;
    char *fName;
    char *lName;
    char *age;
    char *diseaseID;
    char *country;
    DatePtr entryDate;
    DatePtr exitDate;
} Patient;

typedef Patient *PatientPtr;

PatientPtr Patient_Init(
    const char *id,
    const char *fName,
    const char *lName,
    const char *age,
    const char *diseaseID,
    const char *country,
    const char *date);

void Patient_Close(PatientPtr p);
int Patient_addExitDate(PatientPtr p, const char *file);
int Patient_Compare(PatientPtr p1, const char *id, const char *fName, const char *lName, const char *disease, const char *country, const char *age);
void Patient_Print(const PatientPtr patient);

#endif