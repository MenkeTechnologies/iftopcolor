/* hash table - optimized addr_hash with pool allocators and inline operations */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "../../include/addr_hash.h"
#include "../../include/iftop.h"

#define hash_table_size 2048

/* ---- Pool allocator for combined node+key ---- */

typedef struct {
    hash_node_type node;
    addr_pair key;
} node_key_pair;

#define POOL_BLOCK_SIZE 256

typedef struct pool_block_tag {
    struct pool_block_tag *next;
    node_key_pair items[POOL_BLOCK_SIZE];
} pool_block;

static pool_block *pool_blocks = NULL;
static node_key_pair *free_list = NULL;

static node_key_pair *pool_alloc(void) {
    node_key_pair *nkp;
    if (__builtin_expect(free_list != NULL, 1)) {
        nkp = free_list;
        free_list = (node_key_pair *) nkp->node.next;
        return nkp;
    }
    /* Allocate a new block */
    pool_block *block = malloc(sizeof(pool_block));
    if (!block) abort();
    block->next = pool_blocks;
    pool_blocks = block;
    /* Chain all items onto free list except the first (which we return) */
    for (int i = 1; i < POOL_BLOCK_SIZE - 1; i++) {
        block->items[i].node.next = (hash_node_type *) &block->items[i + 1];
    }
    block->items[POOL_BLOCK_SIZE - 1].node.next = NULL;
    free_list = &block->items[1];
    return &block->items[0];
}

static void pool_free(node_key_pair *nkp) {
    nkp->node.next = (hash_node_type *) free_list;
    free_list = nkp;
}

/* ---- Inline hash and compare ---- */

static inline uint32_t hash_mix(uint32_t h, uint32_t val) {
    h ^= val;
    h = (h << 5) | (h >> 27);
    h *= 0x9e3779b9;
    return h;
}

static inline int addr_hash_bucket(const addr_pair *ap) {
    uint32_t h = 0;

    if (ap->af == AF_INET6) {
        const uint32_t *addr6 = (const uint32_t *) ap->src6.s6_addr;
        h = hash_mix(h, addr6[0]);
        h = hash_mix(h, addr6[1]);
        h = hash_mix(h, addr6[2]);
        h = hash_mix(h, addr6[3]);
        h = hash_mix(h, ap->src_port);

        addr6 = (const uint32_t *) ap->dst6.s6_addr;
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

static inline int addr_compare(const void *a, const void *b) {
    return memcmp(a, b, sizeof(addr_pair)) == 0;
}

/* ---- Generic interface wrappers (for function pointers) ---- */

int compare(void *a, void *b) {
    return addr_compare(a, b);
}

int hash(void *key) {
    return addr_hash_bucket((const addr_pair *) key);
}

void *copy_key(void *orig) {
    /* Unused in the fast path - pool_alloc embeds the key */
    addr_pair *copy;
    copy = xmalloc(sizeof *copy);
    *copy = *(addr_pair *) orig;
    return copy;
}

void delete_key(void *key) {
    /* No-op when key is embedded in pool node; only called from generic path */
    (void)key;
}

/* ---- Specialized fast-path operations ---- */

hash_status_enum __attribute__((hot))
addr_hash_find(hash_type *hash_table, addr_pair *key, void **rec) {
    int bucket = addr_hash_bucket(key);
    hash_node_type *p = hash_table->table[bucket];

    while (p) {
        if (addr_compare(p->key, key)) {
            *rec = p->rec;
            return HASH_STATUS_OK;
        }
        p = p->next;
    }
    return HASH_STATUS_KEY_NOT_FOUND;
}

hash_status_enum __attribute__((hot))
addr_hash_insert(hash_type *hash_table, addr_pair *key, void *rec) {
    int bucket = addr_hash_bucket(key);
    node_key_pair *nkp = pool_alloc();
    hash_node_type *p = &nkp->node;
    hash_node_type *p0 = hash_table->table[bucket];

    nkp->key = *key;
    p->key = &nkp->key;
    p->rec = rec;
    p->next = p0;
    p->bucket = bucket;
    hash_table->table[bucket] = p;
    return HASH_STATUS_OK;
}

void __attribute__((hot))
addr_hash_delete_node(hash_type *hash_table, hash_node_type *node) {
    int bucket = node->bucket;
    hash_node_type *p0 = NULL, *p = hash_table->table[bucket];

    while (p && p != node) {
        p0 = p;
        p = p->next;
    }
    if (!p) return;

    if (p0)
        p0->next = p->next;
    else
        hash_table->table[bucket] = p->next;

    /* Node+key is a single pool allocation */
    node_key_pair *nkp = (node_key_pair *) p;
    pool_free(nkp);
}

void addr_hash_delete_all_free(hash_type *hash_table) {
    int i;
    hash_node_type *n, *nn;
    for (i = 0; i < hash_table->size; i++) {
        n = hash_table->table[i];
        while (n != NULL) {
            nn = n->next;
            free(n->rec);
            pool_free((node_key_pair *) n);
            n = nn;
        }
    }
    memset(hash_table->table, 0, hash_table->size * sizeof *hash_table->table);
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

