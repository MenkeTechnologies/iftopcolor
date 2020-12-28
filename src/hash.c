/* hash table */

#include <stdio.h>
#include <stdlib.h>
#include "hash.h"
#include "iftop.h"

int MAX_SIZE = 1024;

hash_status_enum hash_insert(hash_type *hash_table, void *key, void *rec) {
    hash_node_type *p, *p0;
    int bucket;

    /************************************************
     *  allocate node for data and insert in table  *
     ************************************************/


    /* insert node at beginning of list */
    bucket = hash_table->hash(hash_table, key);
    p = xmalloc(sizeof *p);
    p0 = hash_table->table[bucket];
    hash_table->table[bucket] = p;
    p->next = p0;
    p->key = hash_table->copy_key(hash_table, key);
    p->rec = rec;
    hash_table->numItems++;
    return HASH_STATUS_OK;
}

void debugLogAll(hash_type *mapy) {
    mapy->numItems--;
    if (DEBUG) {
        sprintf(DEBUG_BUF, "%s map got delete size %ld\n", mapy->type, mapy->numItems);
        debugLog(DEBUG_BUF);
    }

}

hash_status_enum hash_delete(hash_type *hash_table, void *key) {
    hash_node_type *p0, *p;
    int bucket;

    /********************************************
     *  delete node containing data from table  *
     ********************************************/

    /* find node */
    p0 = 0;
    bucket = hash_table->hash(hash_table, key);
    p = hash_table->table[bucket];
    while (p && !hash_table->compare(hash_table, p->key, key)) {
        p0 = p;
        p = p->next;
    }
    if (!p) return HASH_STATUS_KEY_NOT_FOUND;

    /* p designates node to delete, remove it from list */
    if (p0) {
        /* not first node, p0 points to previous node */
        p0->next = p->next;
    } else {
        /* first node on chain */
        hash_table->table[bucket] = p->next;
    }

    hash_table->delete_key(hash_table, p->key);
    free(p);
    return HASH_STATUS_OK;
}

hash_status_enum hash_find(hash_type *hash_table, void *key, void **rec) {
    hash_node_type *p;

    /*******************************
     *  find node containing data  *
     *******************************/
    p = hash_table->table[hash_table->hash(hash_table, key)];

    while (p && !hash_table->compare(hash_table, p->key, key)) {
        p = p->next;
    }
    if (!p) return HASH_STATUS_KEY_NOT_FOUND;
    *rec = p->rec;
    return HASH_STATUS_OK;
}

hash_status_enum hash_next_item(hash_type *hash_table, hash_node_type **ppnode) {
    int i;
    if (*ppnode != NULL) {
        if ((*ppnode)->next != NULL) {
            *ppnode = (*ppnode)->next;
            return HASH_STATUS_OK;
        }
        i = hash_table->hash(hash_table, (*ppnode)->key) + 1;
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

void hash_delete_all(hash_type *hash_table) {
    int i;
    hash_node_type *n, *nn;
    if (DEBUG) {
        sprintf(DEBUG_BUF, "BEFORE delete all hash size %d vs numItems %ld\n", hash_table->size, hash_table->numItems);
        debugLog(DEBUG_BUF);
    }
    for (i = 0; i < hash_table->size; i++) {
        n = hash_table->table[i];
        while (n != NULL) {
            nn = n->next;
            hash_table->delete_key(hash_table, n->key);
            free(n);
            hash_table->numItems--;
            n = nn;
        }
        hash_table->table[i] = NULL;
    }
    if (DEBUG) {
        sprintf(DEBUG_BUF, "AFTER delete all hash size %d vs numItems %ld\n", hash_table->size, hash_table->numItems);
        debugLog(DEBUG_BUF);
    }
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

