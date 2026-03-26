/* hash table */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include "../../include/ns_hash.h"
#include "../../include/iftop.h"

#define hash_table_size 256

int ns_hash_compare(void *left, void *right) {
    struct addr_storage *l = (struct addr_storage *)left;
    struct addr_storage *r = (struct addr_storage *)right;
    if (l->address_family != r->address_family) {
        return 0;
    }
    return memcmp(&l->addr, &r->addr, l->addr_len) == 0;
}

int ns_hash_hash(void *key) {
    struct addr_storage *a = (struct addr_storage *)key;
    unsigned char *bytes = (unsigned char *)&a->addr;
    int hash = 0;
    for (int i = 0; i < a->addr_len; i++) {
        hash += bytes[i];
    }
    return hash % 0xFF;
}

void *ns_hash_copy_key(void *orig) {
    struct addr_storage *copy;

    copy = xmalloc(sizeof *copy);
    memcpy(copy, orig, sizeof *copy);

    return copy;
}

void ns_hash_delete_key(void *key) {
    free(key);
}

/*
 * Allocate and return a hash
 */
hash_type *ns_hash_create() {
    hash_type *hash_table;
    hash_table = xcalloc(hash_table_size, sizeof *hash_table);
    hash_table->size = hash_table_size;
    hash_table->compare = &ns_hash_compare;
    hash_table->hash = &ns_hash_hash;
    hash_table->delete_key = &ns_hash_delete_key;
    hash_table->copy_key = &ns_hash_copy_key;
    hash_initialise(hash_table);
    return hash_table;
}

int ns_hash_evict_if_full(hash_type *h, int *n_entries, int max_entries) {
    if (*n_entries >= max_entries) {
        hash_delete_all_free(h);
        *n_entries = 0;
        return 1;
    }
    return 0;
}
