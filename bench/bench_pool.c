/*
 * bench_pool.c: benchmarks for pool allocator vs malloc/free
 *
 * Measures the overhead difference between the pool allocator
 * used in addr_hash and direct malloc/free calls.
 */

#include <arpa/inet.h>
#include "bench.h"
#include "addr_hash.h"
#include "hash.h"
#include "iftop.h"

#define N_FLOWS 2000
static addr_pair flows[N_FLOWS];
static int dummy_recs[N_FLOWS];

static void generate_flows(void) {
    for (int i = 0; i < N_FLOWS; i++) {
        memset(&flows[i], 0, sizeof(addr_pair));
        flows[i].af = AF_INET;
        flows[i].protocol = 6;
        flows[i].src.s_addr = htonl(0x0a000001 + i);
        flows[i].src_port = 1024 + (i % 60000);
        flows[i].dst.s_addr = htonl(0xc0a80001);
        flows[i].dst_port = 80;
        dummy_recs[i] = i;
    }
}

/* Clear pool-allocated nodes without freeing recs */
static void clear_addr_nodes(hash_type *ht) {
    for (int i = 0; i < ht->size; i++) {
        while (ht->table[i] != NULL) {
            hash_node_type *node = ht->table[i];
            ht->table[i] = node->next;
            addr_hash_delete_node(ht, node);
        }
    }
}

static void bench_alloc_patterns(void) {
    BENCH_SECTION("Allocation: Pool (addr_hash) vs Malloc (generic hash)");

    BENCH_RUN("pool: insert 2000 + delete all", 1000, {
        hash_type *ht = addr_hash_create();
        for (int j = 0; j < N_FLOWS; j++)
            addr_hash_insert(ht, &flows[j], &dummy_recs[j]);
        clear_addr_nodes(ht);
        hash_destroy(ht);
        free(ht);
    });

    BENCH_RUN("malloc: insert 2000 + delete all", 1000, {
        hash_type *ht = addr_hash_create();
        for (int j = 0; j < N_FLOWS; j++)
            hash_insert(ht, &flows[j], &dummy_recs[j]);
        hash_delete_all(ht);
        hash_destroy(ht);
        free(ht);
    });

    BENCH_RUN("pool: churn 500 insert/delete x 10 cycles", 100, {
        hash_type *ht = addr_hash_create();
        for (int cycle = 0; cycle < 10; cycle++) {
            for (int j = 0; j < 500; j++)
                addr_hash_insert(ht, &flows[j], &dummy_recs[j]);
            clear_addr_nodes(ht);
        }
        hash_destroy(ht);
        free(ht);
    });

    BENCH_RUN("malloc: churn 500 insert/delete x 10 cycles", 100, {
        hash_type *ht = addr_hash_create();
        for (int cycle = 0; cycle < 10; cycle++) {
            for (int j = 0; j < 500; j++)
                hash_insert(ht, &flows[j], &dummy_recs[j]);
            hash_delete_all(ht);
        }
        hash_destroy(ht);
        free(ht);
    });
}

int main(void) {
    printf("iftopcolor pool allocator benchmark\n");
    printf("====================================\n");

    generate_flows();
    bench_alloc_patterns();

    printf("\nDone.\n");
    return 0;
}
