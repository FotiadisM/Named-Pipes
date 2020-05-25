#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <stdio.h>

#include "AVL.h"
#include "patient.h"

#define HASH_SEED 5381

typedef struct HashEntry
{
    char *key;
    AVLTreePtr tree;
} HashEntry;

typedef HashEntry *HashEntryPtr;

typedef struct HashNode
{
    HashEntryPtr *entries;
    struct HashNode *next;
} HashNode;

typedef HashNode *HashNodePtr;

typedef struct HashTable
{
    int size;
    int elements;
    size_t bucketSize;
    HashNodePtr table;
} HashTable;

typedef HashTable *HashTablePtr;

unsigned long hash(const char *str);

HashEntryPtr HashEntry_Init(const char *str);

HashTablePtr HashTable_Init(const int size, const size_t bucketSize);

void HashTable_Close(HashTablePtr ht);

void HashNode_Close(HashNodePtr node, const int bucketSize);

AVLTreePtr HashTable_LocateKey(HashNodePtr ht, const char *key, const int bucketSize);

AVLTreePtr HashNode_Insert(HashNodePtr node, const char *key, const int entriesPerBucket);

int HashTable_Insert(HashTablePtr ht, const char *key, const ListNodePtr listNode);

#endif