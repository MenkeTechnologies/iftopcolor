/* hash table */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include "../../include/ns_hash.h"
#include "../../include/iftop.h"

#define hash_table_size 256

int ns_hash_compare(void *left, void *right) {
    struct in6_addr *left_addr = (struct in6_addr *) left;
    struct in6_addr *right_addr = (struct in6_addr *) right;
    return IN6_ARE_ADDR_EQUAL(left_addr, right_addr);
}

static int __inline__ hash_uint32(uint32_t value) {
    return ((value & 0x000000FF)
            + ((value & 0x0000FF00) >> 8)
            + ((value & 0x00FF0000) >> 16)
            + ((value & 0xFF000000) >> 24));
}

int ns_hash_hash(void *key) {
    int hash;
    uint32_t *addr6 = (uint32_t *) ((struct in6_addr *) key)->s6_addr;

    hash = (hash_uint32(addr6[0])
            + hash_uint32(addr6[1])
            + hash_uint32(addr6[2])
            + hash_uint32(addr6[3])) % 0xFF;

    return hash;
}

void *ns_hash_copy_key(void *orig) {
    struct in6_addr *copy;

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

