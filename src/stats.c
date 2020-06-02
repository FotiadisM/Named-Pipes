#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/stats.h"
#include "../include/pipes.h"
#include "../include/list.h"

int Worker_sendStatistics(const statsPtr st, const int w_fd, const size_t bufferSize, const char *country, const char *file)
{
    statsPtr node = st;
    size_t final_len = 0;
    string_nodePtr buffer_list = NULL, tmp = NULL;
    char *date = NULL, *buffer = NULL, *final_buffer = NULL;
    char ag1[12] = {'\0'}, ag2[12] = {'\0'}, ag3[12] = {'\0'}, ag4[12] = {'\0'};

    if ((date = strdup(file)) == NULL)
    {
        perror("strdup");
        return -1;
    }
    strtok(date, ".");

    while (node != NULL)
    {

        sprintf(ag1, "%d", node->ag->ag1);
        sprintf(ag2, "%d", node->ag->ag2);
        sprintf(ag3, "%d", node->ag->ag3);
        sprintf(ag4, "%d", node->ag->ag4);

        if ((buffer = malloc(strlen(node->disease) + strlen(ag1) + strlen(ag2) + strlen(ag3) + strlen(ag4) + 7)) == NULL)
        {
            perror("malloc");
            return -1;
        }

        strcpy(buffer, node->disease);
        strcat(buffer, "\n");
        strcat(buffer, ag1);
        strcat(buffer, "\n");
        strcat(buffer, ag2);
        strcat(buffer, "\n");
        strcat(buffer, ag3);
        strcat(buffer, "\n");
        strcat(buffer, ag4);
        strcat(buffer, "\n\n");

        final_len += strlen(buffer);
        buffer_list = add_stringNode(buffer_list, buffer);

        free(buffer);

        node = node->next;
    }

    if ((final_buffer = malloc(strlen(date) + strlen(country) + final_len + 3)) == NULL)
    {
        perror("malloc");
        return -1;
    }
    strcpy(final_buffer, date);
    strcat(final_buffer, "\n");
    strcat(final_buffer, country);
    strcat(final_buffer, "\n");

    tmp = buffer_list;
    while (tmp != NULL)
    {
        strcat(final_buffer, tmp->str);
        tmp = tmp->next;
    }

    encode(w_fd, final_buffer, bufferSize);

    free(date);
    free(final_buffer);
    clear_stringNode(buffer_list);

    return 0;
}

ageInfoPtr ageInfo_Init()
{
    ageInfoPtr ag = NULL;

    if ((ag = malloc(sizeof(ageInfo))) == NULL)
    {
        perror("malloc");
        return NULL;
    }

    ag->ag1 = ag->ag2 = ag->ag3 = ag->ag4 = 0;

    return ag;
}

void ageInfo_add(ageInfoPtr ag, const char *age_str)
{
    int age = (int)strtol(age_str, NULL, 10);

    if (age <= 20)
    {
        ag->ag1++;
    }
    else if (age <= 40)
    {
        ag->ag2++;
    }
    else if (age <= 60)
    {
        ag->ag3++;
    }
    else
    {
        ag->ag4++;
    }
}

statsPtr stats_Init(const char *disease)
{
    statsPtr st = NULL;

    if ((st = malloc(sizeof(stats))) == NULL)
    {
        perror("malloc");
        return NULL;
    }

    if ((st->ag = ageInfo_Init()) == NULL)
    {
        perror("ageInfo_Init() failed");
        return NULL;
    }

    if ((st->disease = strdup(disease)) == NULL)
    {
        perror("strdup");
        return NULL;
    }
    st->next = NULL;

    return st;
}

statsPtr stats_add(statsPtr st, const char *disease, const char *age_str)
{
    statsPtr tmp = st;

    if (st == NULL)
    {
        if ((st = stats_Init(disease)) == NULL)
        {
            perror("stats_Init");
            return NULL;
        }
        ageInfo_add(st->ag, age_str);

        return st;
    }

    while (tmp->next != NULL)
    {
        if (!strcmp(tmp->disease, disease))
        {
            break;
        }

        tmp = tmp->next;
    }

    if (!strcmp(tmp->disease, disease))
    {
        ageInfo_add(tmp->ag, age_str);
    }
    else
    {
        if ((tmp->next = stats_Init(disease)) == NULL)
        {
            perror("stats_Init()");
            return NULL;
        }

        ageInfo_add(tmp->next->ag, age_str);
    }

    return st;
}

void stats_close(statsPtr st)
{
    statsPtr tmp = st;

    while (st != NULL)
    {
        tmp = st;
        st = st->next;

        free(tmp->disease);
        free(tmp->ag);
        free(tmp);
    }
}