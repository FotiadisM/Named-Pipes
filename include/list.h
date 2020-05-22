#pragma once

typedef struct string_node
{
    char *str;
    struct string_node *next;
} string_node;

typedef string_node *string_nodePtr;

string_nodePtr add_stringNode(string_nodePtr node, const char *str);

void clear_stringNode(string_nodePtr node);