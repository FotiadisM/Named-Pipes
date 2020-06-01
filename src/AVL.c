#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../include/AVL.h"

AVLTreePtr AVLTree_Init()
{
    AVLTreePtr tree = NULL;

    if ((tree = malloc(sizeof(AVLTree))) == NULL)
    {
        perror("malloc failed");
        return NULL;
    }

    tree->elements = 0;
    tree->root = NULL;

    return tree;
}

void AVLTree_Close(AVLTreePtr tree)
{
    AVLNode_Close(tree->root);

    free(tree);
}

void AVLNode_Close(AVLNodePtr node)
{
    if (node == NULL)
    {
        return;
    }

    AVLNode_Close(node->left);
    AVLNode_Close(node->right);

    free(node);
}

int max(const int a, const int b)
{
    return (a > b) ? a : b;
}

int AVLNode_getHeight(const AVLNodePtr node)
{
    if (node == NULL)
    {
        return 0;
    }

    return node->height;
}

int AVLNode_isBalanced(const AVLNodePtr node)
{
    if (node == NULL)
    {
        return 0;
    }

    return AVLNode_getHeight(node->left) - AVLNode_getHeight(node->right);
}

AVLNodePtr AVLTree_RotateLeft(AVLNodePtr node)
{
    AVLNodePtr pr1 = node->right;
    AVLNodePtr pr2 = pr1->left;

    pr1->left = node;
    node->right = pr2;

    node->height = max(AVLNode_getHeight(node->left), AVLNode_getHeight(node->right)) + 1;
    pr1->height = max(AVLNode_getHeight(pr1->left), AVLNode_getHeight(pr1->right)) + 1;

    return pr1;
}

AVLNodePtr AVLTree_RotateRight(AVLNodePtr node)
{
    AVLNodePtr pr1 = node->left;
    AVLNodePtr pr2 = pr1->right;

    pr1->right = node;
    node->left = pr2;

    node->height = max(AVLNode_getHeight(node->left), AVLNode_getHeight(node->right)) + 1;
    pr1->height = max(AVLNode_getHeight(pr1->left), AVLNode_getHeight(pr1->right)) + 1;

    return pr1;
}

int AVLTree_Insert(AVLTreePtr tree, const DatePtr key, const ListNodePtr listNode)
{
    if ((tree->root = AVLNode_Insert(tree->root, key, listNode)) == NULL)
    {
        return -1;
    }

    tree->elements++;

    return 0;
}

AVLNodePtr AVLNode_Insert(AVLNodePtr node, const DatePtr key, const ListNodePtr listNode)
{
    int isBalanced = 0;

    if (node == NULL)
    {
        AVLNodePtr newNode = NULL;

        if ((newNode = malloc(sizeof(AVLNode))) == NULL)
        {
            perror("malloc failed");
            return NULL;
        }

        newNode->height = 1;
        newNode->key = key;
        newNode->value = listNode;
        newNode->left = NULL;
        newNode->right = NULL;

        return newNode;
    }

    switch (Date_Compare(key, node->key))
    {
    case -1:
        if ((node->left = AVLNode_Insert(node->left, key, listNode)) == NULL)
        {
            return NULL;
        }
        break;

    case 1:
        if ((node->right = AVLNode_Insert(node->right, key, listNode)) == NULL)
        {
            return NULL;
        }
        break;

    default:
        return node;
    }

    node->height = max(AVLNode_getHeight(node->left), AVLNode_getHeight(node->right)) + 1;

    isBalanced = AVLNode_isBalanced(node);

    if (isBalanced > 1)
    {
        switch (Date_Compare(key, node->left->key))
        {
        case -1:
            return AVLTree_RotateRight(node);

        case 1:
            node->left = AVLTree_RotateLeft(node->left);
            return AVLTree_RotateRight(node);
        }
    }
    else if (isBalanced < -1)
    {
        switch (Date_Compare(key, node->right->key))
        {
        case -1:
            node->right = AVLTree_RotateRight(node->right);
            return AVLTree_RotateLeft(node);

        case 1:
            return AVLTree_RotateLeft(node);
        }
    }

    return node;
}

int AVLNode_countPatients(const AVLNodePtr node, const char *disease, const char *country, const DatePtr d1, const DatePtr d2)
{
    if (node == NULL)
    {
        return 0;
    }

    if (Date_Compare(node->key, d2) == 1)
    {
        return AVLNode_countPatients(node->left, disease, country, d1, d2);
    }
    else if (Date_Compare(node->key, d1) == -1)
    {
        return AVLNode_countPatients(node->right, disease, country, d1, d2);
    }
    else
    {
        int count = 0;
        ListNodePtr listNode = node->value;

        if (country == NULL)
        {
            while (listNode != NULL)
            {
                if (Date_Compare(listNode->patient->entryDate, node->key))
                {
                    break;
                }
                if (!strcmp(listNode->patient->diseaseID, disease))
                {
                    count++;
                }
                listNode = listNode->next;
            }
        }
        else
        {
            while (listNode != NULL)
            {
                if (Date_Compare(listNode->patient->entryDate, node->key))
                {
                    break;
                }
                if (!strcmp(listNode->patient->country, country))
                {
                    if (!strcmp(listNode->patient->diseaseID, disease))
                    {
                        count++;
                    }
                }
                listNode = listNode->next;
            }
        }

        return (AVLNode_countPatients(node->left, disease, country, d1, d2) + AVLNode_countPatients(node->right, disease, country, d1, d2) + count);
    }
}

int AVLNode_countNullPatients(AVLNodePtr node, const char *disease)
{
    int count = 0;
    ListNodePtr ptr = NULL;

    if (node == NULL)
    {
        return 0;
    }

    ptr = node->value;
    while (ptr != NULL)
    {
        if (Date_Compare(ptr->patient->entryDate, node->key))
        {
            break;
        }

        if (!strcmp(ptr->patient->diseaseID, disease))
        {
            if (ptr->patient->exitDate == NULL)
            {
                count++;
            }
        }
        ptr = ptr->next;
    }

    return (AVLNode_countNullPatients(node->left, disease) + AVLNode_countNullPatients(node->right, disease) + count);
}
