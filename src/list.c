#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/list.h"

string_nodePtr add_stringNode(string_nodePtr node, const char *str)
{
    string_nodePtr newNode = NULL;

    if ((newNode = malloc(sizeof(string_node))) == NULL)
    {
        perror("malloc");
        return NULL;
    }

    if ((newNode->str = malloc(strlen(str) + 1)) == NULL)
    {
        perror("malloc");
        return NULL;
    }

    strcpy(newNode->str, str);
    newNode->next = node;

    return newNode;
}

void clear_stringNode(string_nodePtr node)
{
    string_nodePtr tmp = NULL;

    while (node != NULL)
    {
        tmp = node;
        node = node->next;

        free(tmp->str);
        free(tmp);
    }
}