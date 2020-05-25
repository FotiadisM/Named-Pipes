#include <stdlib.h>
#include <string.h>

#include "../include/defines.h"
#include "../include/hashTable.h"

unsigned long hash(const char *str)
{
    int c;
    unsigned long hash = HASH_SEED;

    while ((c = *str++))
    {
        hash = ((hash << 5) + hash) + c;
    }

    return hash;
}

HashEntryPtr HashEntry_Init(const char *key)
{
    HashEntryPtr entry = NULL;

    if ((entry = malloc(sizeof(HashEntry))) == NULL)
    {
        perror("malloc failed");
        return NULL;
    }

    if ((entry->key = malloc(strlen(key) + 1)) == NULL)
    {
        perror("malloc failed");
        free(entry);
        return NULL;
    }
    strcpy(entry->key, key);

    if ((entry->tree = AVLTree_Init(entry->tree)) == NULL)
    {
        free(entry->key);
        free(entry);
        return NULL;
    }

    return entry;
}

HashTablePtr HashTable_Init(const int size, const size_t bucketSize)
{
    HashTablePtr ht = NULL;

    if ((ht = malloc(sizeof(HashTable))) == NULL)
    {
        perror("malloc failed");
        return NULL;
    }

    ht->size = size;
    ht->elements = 0;
    ht->bucketSize = (bucketSize - sizeof(HashNodePtr)) / sizeof(HashEntryPtr);

    if ((ht->table = malloc(size * sizeof(HashNode))) == NULL)
    {
        perror("malloc failed");
        return NULL;
    }

    for (int i = 0; i < size; i++)
    {
        if ((ht->table[i].entries = malloc(ht->bucketSize * sizeof(HashEntryPtr))) == NULL)
        {
            perror("malloc failed");
            return NULL;
        }

        for (int j = 0; j < (int)ht->bucketSize; j++)
        {
            ht->table[i].entries[j] = NULL;
        }

        ht->table[i].next = NULL;
    }

    return ht;
}

void HashTable_Close(HashTablePtr ht)
{
    HashNodePtr tmpHashNode = NULL;

    for (int i = 0; i < ht->size; i++)
    {
        while (ht->table[i].next != NULL)
        {
            tmpHashNode = ht->table[i].next;
            ht->table[i].next = tmpHashNode->next;
            HashNode_Close(tmpHashNode, ht->bucketSize);
            free(tmpHashNode);
        }
        HashNode_Close(&(ht->table[i]), ht->bucketSize);
    }

    free(ht->table);
    free(ht);
}

void HashNode_Close(HashNodePtr node, const int bucketSize)
{
    for (int i = 0; i < bucketSize; i++)
    {
        if (node->entries[i] != NULL)
        {
            free(node->entries[i]->key);
            AVLTree_Close(node->entries[i]->tree);
            free(node->entries[i]);
        }
    }

    free(node->entries);
}

AVLTreePtr HashTable_LocateKey(HashNodePtr node, const char *key, const int bucketSize)
{
    HashNodePtr tmpHashNode = node;

    while (tmpHashNode != NULL)
    {
        for (int i = 0; i < bucketSize; i++)
        {
            if (tmpHashNode->entries[i] != NULL)
            {
                if (!strcmp(tmpHashNode->entries[i]->key, key))
                {
                    return tmpHashNode->entries[i]->tree;
                }
            }
        }
        tmpHashNode = tmpHashNode->next;
    }

    return NULL;
}

AVLTreePtr HashNode_Insert(HashNodePtr node, const char *key, const int bucketSize)
{
    HashNodePtr tmpHashNode = node;

    while (tmpHashNode->next != NULL)
    {
        tmpHashNode = tmpHashNode->next;
    }

    for (int i = 0; i < bucketSize; i++)
    {
        if (tmpHashNode->entries[i] == NULL)
        {
            if ((tmpHashNode->entries[i] = HashEntry_Init(key)) == NULL)
            {
                return NULL;
            }

            return tmpHashNode->entries[i]->tree;
        }
    }

    if ((tmpHashNode->next = malloc(sizeof(HashNode))) == NULL)
    {
        perror("mallon failed");
        return NULL;
    }

    tmpHashNode = tmpHashNode->next;
    tmpHashNode->next = NULL;

    if ((tmpHashNode->entries = malloc(bucketSize * sizeof(HashEntryPtr))) == NULL)
    {
        perror("mallon failed");
        return NULL;
    }

    for (int i = 0; i < bucketSize; i++)
    {
        tmpHashNode->entries[i] = NULL;
    }

    if ((tmpHashNode->entries[0] = HashEntry_Init(key)) == NULL)
    {
        return NULL;
    }

    return tmpHashNode->entries[0]->tree;
}

int HashTable_Insert(HashTablePtr ht, const char *key, const ListNodePtr listNode)
{
    int index = hash(key) % ht->size;
    AVLTreePtr tree = NULL;

    if ((tree = HashTable_LocateKey(&(ht->table[index]), key, ht->bucketSize)) == NULL)
    {
        if ((tree = HashNode_Insert(&(ht->table[index]), key, ht->bucketSize)) == NULL)
        {
            return -1;
        }
        ht->elements++;
    }

    return AVLTree_Insert(tree, listNode->patient->entryDate, listNode);
}