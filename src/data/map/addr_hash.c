/* hash table */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "../../include/addr_hash.h"
#include "../../include/iftop.h"

#define hash_table_size 2048

int __attribute__((hot)) compare(void *a, void *b) {
    /* addr_pair structs are always zeroed on creation, so memcmp is safe
     * (no uninitialized padding bytes). This is faster than field-by-field
     * comparison and works for both IPv4 and IPv6. */
    return memcmp(a, b, sizeof(addr_pair)) == 0;
}

static uint32_t __inline__ hash_mix(uint32_t h, uint32_t val) {
    h ^= val;
    h = (h << 5) | (h >> 27);
    h *= 0x9e3779b9;
    return h;
}

int __attribute__((hot)) hash(void *key) {
    uint32_t h = 0;
    addr_pair *ap = (addr_pair *) key;

    if (ap->af == AF_INET6) {
        uint32_t *addr6 = (uint32_t *) ap->src6.s6_addr;
        h = hash_mix(h, addr6[0]);
        h = hash_mix(h, addr6[1]);
        h = hash_mix(h, addr6[2]);
        h = hash_mix(h, addr6[3]);
        h = hash_mix(h, ap->src_port);

        addr6 = (uint32_t *) ap->dst6.s6_addr;
        h = hash_mix(h, addr6[0]);
        h = hash_mix(h, addr6[1]);
        h = hash_mix(h, addr6[2]);
        h = hash_mix(h, addr6[3]);
        h = hash_mix(h, ap->dst_port);
    } else {
        h = hash_mix(h, ap->src.s_addr);
        h = hash_mix(h, ap->src_port);
        h = hash_mix(h, ap->dst.s_addr);
        h = hash_mix(h, ap->dst_port);
    }

    return h & (hash_table_size - 1);
}

void *copy_key(void *orig) {
    addr_pair *copy;
    copy = xmalloc(sizeof *copy);
    *copy = *(addr_pair *) orig;
    return copy;
}

void delete_key(void *key) {
    free(key);
}

/*
 * Allocate and return a hash
 */
hash_type *addr_hash_create() {
    hash_type *hash_table;
    hash_table = xcalloc(1, sizeof *hash_table);
    hash_table->size = hash_table_size;
    hash_table->compare = &compare;
    hash_table->hash = &hash;
    hash_table->delete_key = &delete_key;
    hash_table->copy_key = &copy_key;
    hash_initialise(hash_table);
    return hash_table;
}

