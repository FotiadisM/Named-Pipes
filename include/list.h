#pragma once

#include "./patient.h"

#define F_PATIENT 1
#define DF_PATIENT 0

typedef struct string_node
{
    char *str;
    struct string_node *next;
} string_node;

typedef string_node *string_nodePtr;

typedef struct ListNode
{
    PatientPtr patient;
    struct ListNode *next;
} ListNode;

typedef ListNode *ListNodePtr;

typedef struct List
{
    int len;
    ListNodePtr head;
} List;

typedef List *ListPtr;

string_nodePtr add_stringNode(string_nodePtr node, const char *str);
void clear_stringNode(string_nodePtr node);

ListPtr List_Init();
void List_Close(ListPtr l, int bool);
ListNodePtr List_InsertSorted(ListPtr list, const PatientPtr patient);

void List_Print(ListPtr list);
// int List_Insert(ListPtr list, const PatientPtr patient);
