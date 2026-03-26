/* hash table */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../include/hash.h"
#include "../../include/iftop.h"

hash_status_enum hash_insert(hash_type *hash_table, void *key, void *rec) {
    hash_node_type *node, *prev;
    int bucket;

    /************************************************
     *  allocate node for data and insert in table  *
     ************************************************/


    /* insert node at beginning of list */
    bucket = hash_table->hash(key);
    node = xmalloc(sizeof *node);
    prev = hash_table->table[bucket];
    hash_table->table[bucket] = node;
    node->next = prev;
    node->key = hash_table->copy_key(key);
    node->record = rec;
    node->bucket = bucket;
    return HASH_STATUS_OK;
}

hash_status_enum hash_delete(hash_type *hash_table, void *key) {
    hash_node_type *prev, *node;
    int bucket;

    /********************************************
     *  delete node containing data from table  *
     ********************************************/

    /* find node */
    prev = 0;
    bucket = hash_table->hash(key);
    node = hash_table->table[bucket];
    while (node && !hash_table->compare(node->key, key)) {
        prev = node;
        node = node->next;
    }
    if (!node) {
        return HASH_STATUS_KEY_NOT_FOUND;
    }

    /* node designates node to delete, remove it from list */
    if (prev) {
        /* not first node, prev points to previous node */
        prev->next = node->next;
    } else {
        /* first node on chain */
        hash_table->table[bucket] = node->next;
    }

    hash_table->delete_key(node->key);
    free(node);
    return HASH_STATUS_OK;
}

hash_status_enum __attribute__((hot)) hash_find(hash_type *hash_table, void *key, void **rec) {
    hash_node_type *node;
    int bucket;

    bucket = hash_table->hash(key);
    node = hash_table->table[bucket];

    while (node && !hash_table->compare(node->key, key)) {
        node = node->next;
    }
    if (__builtin_expect(!node, 0)) {
        return HASH_STATUS_KEY_NOT_FOUND;
    }
    *rec = node->record;
    return HASH_STATUS_OK;
}

hash_status_enum hash_next_item(hash_type *hash_table, hash_node_type **ppnode) {
    int i;
    if (*ppnode != NULL) {
        if ((*ppnode)->next != NULL) {
            *ppnode = (*ppnode)->next;
            return HASH_STATUS_OK;
        }
        i = (*ppnode)->bucket + 1;
    } else {
        /* first node */
        i = 0;
    }
    while (i < hash_table->size && hash_table->table[i] == NULL) {
        i++;
    }
    if (i == hash_table->size) {
        *ppnode = NULL;
        return HASH_STATUS_KEY_NOT_FOUND;
    }
    *ppnode = hash_table->table[i];
    return HASH_STATUS_OK;
}

/*
 * Delete a node by pointer (avoids re-hashing). Bucket index is cached in node.
 */
hash_status_enum hash_delete_node(hash_type *hash_table, hash_node_type *target) {
    hash_node_type *prev, *node;
    int bucket = target->bucket;

    prev = NULL;
    node = hash_table->table[bucket];
    while (node && node != target) {
        prev = node;
        node = node->next;
    }
    if (!node) {
        return HASH_STATUS_KEY_NOT_FOUND;
    }

    if (prev) {
        prev->next = node->next;
    } else {
        hash_table->table[bucket] = node->next;
    }

    hash_table->delete_key(node->key);
    free(node);
    return HASH_STATUS_OK;
}

void hash_delete_all(hash_type *hash_table) {
    int i;
    hash_node_type *node, *next_node;
    for (i = 0; i < hash_table->size; i++) {
        node = hash_table->table[i];
        while (node != NULL) {
            next_node = node->next;
            hash_table->delete_key(node->key);
            free(node);
            node = next_node;
        }
    }
    memset(hash_table->table, 0, hash_table->size * sizeof *hash_table->table);
}

/*
 * Delete all nodes AND free their record pointers.
 */
void hash_delete_all_free(hash_type *hash_table) {
    int i;
    hash_node_type *node, *next_node;
    for (i = 0; i < hash_table->size; i++) {
        node = hash_table->table[i];
        while (node != NULL) {
            next_node = node->next;
            hash_table->delete_key(node->key);
            free(node->record);
            free(node);
            node = next_node;
        }
    }
    memset(hash_table->table, 0, hash_table->size * sizeof *hash_table->table);
}


/*
 * Allocate and return a hash
 */
hash_status_enum hash_initialise(hash_type *hash_table) {
    hash_table->table = xcalloc(hash_table->size, sizeof *hash_table->table);
    return HASH_STATUS_OK;
}

hash_status_enum hash_destroy(hash_type *hash_table) {
    free(hash_table->table);
    return HASH_STATUS_OK;
}
