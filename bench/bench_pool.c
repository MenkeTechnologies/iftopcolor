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
        flows[i].address_family = AF_INET;
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
        for (int j = 0; j < N_FLOWS; j++) {
            addr_hash_insert(ht, &flows[j], &dummy_recs[j]);
        }
        clear_addr_nodes(ht);
        hash_destroy(ht);
        free(ht);
    });

    BENCH_RUN("malloc: insert 2000 + delete all", 1000, {
        hash_type *ht = addr_hash_create();
        for (int j = 0; j < N_FLOWS; j++) {
            hash_insert(ht, &flows[j], &dummy_recs[j]);
        }
        hash_delete_all(ht);
        hash_destroy(ht);
        free(ht);
    });

    BENCH_RUN("pool: churn 500 insert/delete x 10 cycles", 100, {
        hash_type *ht = addr_hash_create();
        for (int cycle = 0; cycle < 10; cycle++) {
            for (int j = 0; j < 500; j++) {
                addr_hash_insert(ht, &flows[j], &dummy_recs[j]);
            }
            clear_addr_nodes(ht);
        }
        hash_destroy(ht);
        free(ht);
    });

    BENCH_RUN("malloc: churn 500 insert/delete x 10 cycles", 100, {
        hash_type *ht = addr_hash_create();
        for (int cycle = 0; cycle < 10; cycle++) {
            for (int j = 0; j < 500; j++) {
                hash_insert(ht, &flows[j], &dummy_recs[j]);
            }
            hash_delete_all(ht);
        }
        hash_destroy(ht);
        free(ht);
    });
}

static void bench_multi_block(void) {
    BENCH_SECTION("Multi-Block Pool Allocation (>256 items per block)");

    BENCH_RUN("pool: insert 1000 (4 blocks)", 1000, {
        hash_type *ht = addr_hash_create();
        for (int j = 0; j < 1000; j++) {
            addr_hash_insert(ht, &flows[j % N_FLOWS], &dummy_recs[j % N_FLOWS]);
        }
        clear_addr_nodes(ht);
        hash_destroy(ht);
        free(ht);
    });

    BENCH_RUN("malloc: insert 1000", 1000, {
        hash_type *ht = addr_hash_create();
        for (int j = 0; j < 1000; j++) {
            hash_insert(ht, &flows[j % N_FLOWS], &dummy_recs[j % N_FLOWS]);
        }
        hash_delete_all(ht);
        hash_destroy(ht);
        free(ht);
    });
}

static void bench_pool_reuse(void) {
    BENCH_SECTION("Pool Reuse (fill-clear-refill)");

    BENCH_RUN("pool: insert 500, clear, refill x 20", 100, {
        hash_type *ht = addr_hash_create();
        for (int r = 0; r < 20; r++) {
            for (int j = 0; j < 500; j++) {
                addr_hash_insert(ht, &flows[j], &dummy_recs[j]);
            }
            clear_addr_nodes(ht);
        }
        hash_destroy(ht);
        free(ht);
    });
}

static void bench_find_after_churn(void) {
    BENCH_SECTION("Find Performance After Churn");

    BENCH_RUN("insert 1000, delete 500, find remaining 500", 100, {
        hash_type *ht = addr_hash_create();
        for (int j = 0; j < 1000; j++) {
            addr_hash_insert(ht, &flows[j], &dummy_recs[j]);
        }
        /* Delete first 500 */
        for (int b = 0; b < ht->size; b++) {
            while (ht->table[b] != NULL) {
                hash_node_type *node = ht->table[b];
                ht->table[b] = node->next;
                addr_hash_delete_node(ht, node);
                break;
            }
        }
        /* Find remaining entries */
        for (int j = 500; j < 1000; j++) {
            void *rec;
            addr_hash_find(ht, &flows[j], &rec);
        }
        clear_addr_nodes(ht);
        hash_destroy(ht);
        free(ht);
    });
}

int main(void) {
    BENCH_HEADER("POOL ALLOCATOR STRESS TEST");

    generate_flows();
    bench_alloc_patterns();
    bench_multi_block();
    bench_pool_reuse();
    bench_find_after_churn();

    BENCH_FOOTER();
    return 0;
}
