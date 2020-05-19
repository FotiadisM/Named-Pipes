#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/patient.h"

PatientPtr Patient_Init(
    const char *id,
    const char *fName,
    const char *lName,
    const char *age,
    const char *disease,
    const char *country,
    const char *date)
{
    PatientPtr p = NULL;

    if ((p = malloc(sizeof(Patient))) == NULL)
    {
        perror("malloc");
        return NULL;
    }

    else if ((p->id = malloc(strlen(id) + 1)) == NULL)
    {
        perror("malloc");
        free(p);
        return NULL;
    }

    else if ((p->fName = malloc(strlen(fName) + 1)) == NULL)
    {
        perror("malloc");
        Patient_Close(p);
        return NULL;
    }

    else if ((p->lName = malloc(strlen(lName) + 1)) == NULL)
    {
        perror("malloc");
        Patient_Close(p);
        return NULL;
    }

    else if ((p->age = malloc(strlen(age) + 1)) == NULL)
    {
        perror("malloc");
        Patient_Close(p);
        return NULL;
    }

    else if ((p->disease = malloc(strlen(disease) + 1)) == NULL)
    {
        perror("malloc");
        Patient_Close(p);
        return NULL;
    }

    else if ((p->country = malloc(strlen(country) + 1)) == NULL)
    {
        perror("malloc");
        Patient_Close(p);
        return NULL;
    }

    strcpy(p->id, id);
    strcpy(p->fName, fName);
    strcpy(p->lName, lName);
    strcpy(p->age, age);
    strcpy(p->disease, disease);
    strcpy(p->country, country);

    if ((p->entry = Date_Init(date)) == NULL)
    {
        Patient_Close(p);
        return NULL;
    }

    return p;
}

void Patient_Close(PatientPtr p)
{
    if (p->id != NULL)
    {
        free(p->id);
    }

    if (p->fName != NULL)
    {
        free(p->fName);
    }

    if (p->lName != NULL)
    {
        free(p->lName);
    }

    if (p->age != NULL)
    {
        free(p->age);
    }

    if (p->disease != NULL)
    {
        free(p->disease);
    }

    if (p->country != NULL)
    {
        free(p->country);
    }

    if (p->entry != NULL)
    {
        free(p->entry);
    }

    if (p->entry != NULL)
    {
        free(p->exit);
    }

    free(p);
}

int Patient_Compare(PatientPtr p1, PatientPtr p2)
{
    if (strcmp(p1->id, p2->id))
    {
        return -1;
    }

    else if (strcmp(p1->fName, p2->fName))
    {
        return -1;
    }

    else if (strcmp(p1->lName, p2->lName))
    {
        return -1;
    }

    else if (strcmp(p1->age, p2->age))
    {
        return -1;
    }

    return 0;
}