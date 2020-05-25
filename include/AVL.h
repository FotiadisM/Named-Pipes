#ifndef AVL_H
#define AVL_H

#include "list.h"
#include "patient.h"

typedef struct AVLNode
{
    int height;
    DatePtr key;
    ListNodePtr value;
    struct AVLNode *left;
    struct AVLNode *right;
} AVLNode;

typedef AVLNode *AVLNodePtr;

typedef struct AVLTree
{
    int elements;
    AVLNodePtr root;
} AVLTree;

typedef AVLTree *AVLTreePtr;

AVLTreePtr AVLTree_Init();

void AVLNode_Close(AVLNodePtr node);

void AVLTree_Close(AVLTreePtr tree);

int max(const int a, const int b);

int AVLNode_getHeight(const AVLNodePtr node);

int AVLNode_isBalanced(const AVLNodePtr node);

AVLNodePtr AVLTree_RotateLeft(AVLNodePtr node);

AVLNodePtr AVLTree_RotateRight(AVLNodePtr node);

int AVLTree_Insert(AVLTreePtr tree, const DatePtr key, const ListNodePtr listNode);

AVLNodePtr AVLNode_Insert(AVLNodePtr node, const DatePtr key, const ListNodePtr listNode);

int AVLNode_countPatients(const AVLNodePtr node, const char *disease, const char *country, const DatePtr d1, const DatePtr d2);

int AVLNode_countNullPatients(AVLNodePtr node, const char *disease);

#endif