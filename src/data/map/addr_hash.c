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
        free_list = (node_key_pair *)nkp->node.next;
        return nkp;
    }
    /* Allocate a new block */
    pool_block *block = malloc(sizeof(pool_block));
    if (!block) {
        abort();
    }
    block->next = pool_blocks;
    pool_blocks = block;
    /* Chain all items onto free list except the first (which we return) */
    for (int i = 1; i < POOL_BLOCK_SIZE - 1; i++) {
        block->items[i].node.next = (hash_node_type *)&block->items[i + 1];
    }
    block->items[POOL_BLOCK_SIZE - 1].node.next = NULL;
    free_list = &block->items[1];
    return &block->items[0];
}

static void pool_free(node_key_pair *nkp) {
    nkp->node.next = (hash_node_type *)free_list;
    free_list = nkp;
}

/* ---- Inline hash and compare ---- */

static inline uint32_t hash_mix(uint32_t hash_val, uint32_t val) {
    hash_val ^= val;
    hash_val = (hash_val << 5) | (hash_val >> 27);
    hash_val *= 0x9e3779b9;
    return hash_val;
}

static inline int addr_hash_bucket(const addr_pair *pair) {
    uint32_t hash_val = 0;

    if (pair->address_family == AF_INET6) {
        const uint32_t *addr6 = (const uint32_t *)pair->src6.s6_addr;
        hash_val = hash_mix(hash_val, addr6[0]);
        hash_val = hash_mix(hash_val, addr6[1]);
        hash_val = hash_mix(hash_val, addr6[2]);
        hash_val = hash_mix(hash_val, addr6[3]);
        hash_val = hash_mix(hash_val, pair->src_port);

        addr6 = (const uint32_t *)pair->dst6.s6_addr;
        hash_val = hash_mix(hash_val, addr6[0]);
        hash_val = hash_mix(hash_val, addr6[1]);
        hash_val = hash_mix(hash_val, addr6[2]);
        hash_val = hash_mix(hash_val, addr6[3]);
        hash_val = hash_mix(hash_val, pair->dst_port);
    } else {
        hash_val = hash_mix(hash_val, pair->src.s_addr);
        hash_val = hash_mix(hash_val, pair->src_port);
        hash_val = hash_mix(hash_val, pair->dst.s_addr);
        hash_val = hash_mix(hash_val, pair->dst_port);
    }

    return hash_val & (hash_table_size - 1);
}

static inline int addr_compare(const void *lhs, const void *rhs) {
    return memcmp(lhs, rhs, sizeof(addr_pair)) == 0;
}

/* ---- Generic interface wrappers (for function pointers) ---- */

int compare(void *lhs, void *rhs) {
    return addr_compare(lhs, rhs);
}

int hash(void *key) {
    return addr_hash_bucket((const addr_pair *)key);
}

void *copy_key(void *orig) {
    /* Unused in the fast path - pool_alloc embeds the key */
    addr_pair *copy;
    copy = xmalloc(sizeof *copy);
    *copy = *(addr_pair *)orig;
    return copy;
}

void delete_key(void *key) {
    /* No-op when key is embedded in pool node; only called from generic path */
    (void)key;
}

/* ---- Specialized fast-path operations ---- */

hash_status_enum __attribute__((hot)) addr_hash_find(hash_type *hash_table, addr_pair *key,
                                                     void **rec) {
    int bucket = addr_hash_bucket(key);
    hash_node_type *node = hash_table->table[bucket];

    while (node) {
        if (addr_compare(node->key, key)) {
            *rec = node->record;
            return HASH_STATUS_OK;
        }
        node = node->next;
    }
    return HASH_STATUS_KEY_NOT_FOUND;
}

hash_status_enum __attribute__((hot)) addr_hash_insert(hash_type *hash_table, addr_pair *key,
                                                       void *rec) {
    int bucket = addr_hash_bucket(key);
    node_key_pair *nkp = pool_alloc();
    hash_node_type *node = &nkp->node;
    hash_node_type *prev = hash_table->table[bucket];

    nkp->key = *key;
    node->key = &nkp->key;
    node->record = rec;
    node->next = prev;
    node->bucket = bucket;
    hash_table->table[bucket] = node;
    return HASH_STATUS_OK;
}

void __attribute__((hot)) addr_hash_delete_node(hash_type *hash_table, hash_node_type *target) {
    int bucket = target->bucket;
    hash_node_type *prev = NULL, *node = hash_table->table[bucket];

    while (node && node != target) {
        prev = node;
        node = node->next;
    }
    if (!node) {
        return;
    }

    if (prev) {
        prev->next = node->next;
    } else {
        hash_table->table[bucket] = node->next;
    }

    /* Node+key is a single pool allocation */
    node_key_pair *nkp = (node_key_pair *)node;
    pool_free(nkp);
}

void addr_hash_delete_all_free(hash_type *hash_table) {
    int i;
    hash_node_type *node, *next_node;
    for (i = 0; i < hash_table->size; i++) {
        node = hash_table->table[i];
        while (node != NULL) {
            next_node = node->next;
            free(node->record);
            pool_free((node_key_pair *)node);
            node = next_node;
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
