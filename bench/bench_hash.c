/*
 * bench_hash.c: benchmarks for addr_hash and generic hash operations
 *
 * Covers: hash function distribution, insert, find (hit/miss),
 * delete, iteration, and high-load-factor behavior.
 */

#include <math.h>
#include <arpa/inet.h>
#include "bench.h"
#include "addr_hash.h"
#include "hash.h"
#include "iftop.h"

/* Defined in addr_hash.c but not in header */
extern int hash(void *key);

/* --- Helpers --- */

static addr_pair make_pair_v4(uint32_t src, uint16_t sport,
                              uint32_t dst, uint16_t dport) {
    addr_pair ap;
    memset(&ap, 0, sizeof(ap));
    ap.address_family = AF_INET;
    ap.protocol = 6;
    ap.src.s_addr = htonl(src);
    ap.src_port = sport;
    ap.dst.s_addr = htonl(dst);
    ap.dst_port = dport;
    return ap;
}

static addr_pair make_pair_v6(int index) {
    addr_pair ap;
    memset(&ap, 0, sizeof(ap));
    ap.address_family = AF_INET6;
    ap.protocol = 6;
    uint32_t *s6 = (uint32_t *)ap.src6.s6_addr;
    uint32_t *d6 = (uint32_t *)ap.dst6.s6_addr;
    s6[0] = htonl(0x20010db8);
    s6[1] = htonl(index);
    s6[2] = htonl(index * 31);
    s6[3] = htonl(index * 97);
    d6[0] = htonl(0x20010db8);
    d6[1] = htonl(index * 7);
    d6[2] = htonl(index * 13);
    d6[3] = htonl(index * 53);
    ap.src_port = (index % 60000) + 1024;
    ap.dst_port = 80;
    return ap;
}

#define MAX_FLOWS 10000
static addr_pair flows_v4[MAX_FLOWS];
static addr_pair flows_v6[MAX_FLOWS];
static int dummy_recs[MAX_FLOWS];

static void generate_flows(void) {
    for (int i = 0; i < MAX_FLOWS; i++) {
        flows_v4[i] = make_pair_v4(
            0x0a000001 + (i / 256),
            1024 + (i % 60000),
            0xc0a80001 + (i % 256),
            80 + (i % 10)
        );
        flows_v6[i] = make_pair_v6(i);
        dummy_recs[i] = i;
    }
}

/*
 * Clear pool-allocated nodes: iterate and delete each via addr_hash_delete_node.
 * Does NOT free rec pointers (they're static).
 */
static void clear_addr_hash(hash_type *ht) {
    for (int i = 0; i < ht->size; i++) {
        while (ht->table[i] != NULL) {
            hash_node_type *node = ht->table[i];
            ht->table[i] = node->next;
            addr_hash_delete_node(ht, node);
        }
    }
}

static void destroy_addr_hash(hash_type *ht) {
    clear_addr_hash(ht);
    hash_destroy(ht);
    free(ht);
}

/*
 * Clear malloc-allocated nodes (from generic hash_insert).
 */
static void destroy_generic_hash(hash_type *ht) {
    hash_delete_all(ht);
    hash_destroy(ht);
    free(ht);
}

/* --- Hash distribution analysis --- */

static void bench_hash_distribution(void) {
    BENCH_SECTION("Hash Distribution (2048 buckets)");

    int buckets[2048];

    memset(buckets, 0, sizeof(buckets));
    for (int i = 0; i < MAX_FLOWS; i++)
        buckets[hash(&flows_v4[i])]++;

    int max_chain = 0, empty = 0;
    double avg = (double)MAX_FLOWS / 2048;
    double variance = 0;
    for (int i = 0; i < 2048; i++) {
        if (buckets[i] > max_chain) max_chain = buckets[i];
        if (buckets[i] == 0) empty++;
        double diff = buckets[i] - avg;
        variance += diff * diff;
    }
    variance /= 2048;
    printf("  IPv4: %d flows -> max_chain=%d, empty=%d/2048, stddev=%.2f (ideal=%.2f)\n",
           MAX_FLOWS, max_chain, empty, sqrt(variance), sqrt(avg));

    memset(buckets, 0, sizeof(buckets));
    for (int i = 0; i < MAX_FLOWS; i++)
        buckets[hash(&flows_v6[i])]++;

    max_chain = 0; empty = 0; variance = 0;
    for (int i = 0; i < 2048; i++) {
        if (buckets[i] > max_chain) max_chain = buckets[i];
        if (buckets[i] == 0) empty++;
        double diff = buckets[i] - avg;
        variance += diff * diff;
    }
    variance /= 2048;
    printf("  IPv6: %d flows -> max_chain=%d, empty=%d/2048, stddev=%.2f (ideal=%.2f)\n",
           MAX_FLOWS, max_chain, empty, sqrt(variance), sqrt(avg));
}

/* --- Core operations --- */

static void bench_insert(void) {
    BENCH_SECTION("Hash Insert");

    BENCH_RUN("addr_hash_insert (1000 IPv4 flows)", 100, {
        hash_type *ht = addr_hash_create();
        for (int j = 0; j < 1000; j++)
            addr_hash_insert(ht, &flows_v4[j], &dummy_recs[j]);
        destroy_addr_hash(ht);
    });

    BENCH_RUN("hash_insert (1000 IPv4 flows, generic)", 100, {
        hash_type *ht = addr_hash_create();
        for (int j = 0; j < 1000; j++)
            hash_insert(ht, &flows_v4[j], &dummy_recs[j]);
        destroy_generic_hash(ht);
    });

    BENCH_RUN("addr_hash_insert (1000 IPv6 flows)", 100, {
        hash_type *ht = addr_hash_create();
        for (int j = 0; j < 1000; j++)
            addr_hash_insert(ht, &flows_v6[j], &dummy_recs[j]);
        destroy_addr_hash(ht);
    });
}

static void bench_find(void) {
    BENCH_SECTION("Hash Find");

    hash_type *ht = addr_hash_create();
    int n = 1000;
    for (int i = 0; i < n; i++)
        addr_hash_insert(ht, &flows_v4[i], &dummy_recs[i]);

    BENCH_RUN("addr_hash_find hit (1000 entries, IPv4)", 1000000, {
        void *rec;
        bench_use(addr_hash_find(ht, &flows_v4[_i % n], &rec));
    });

    BENCH_RUN("hash_find hit (1000 entries, IPv4)", 1000000, {
        void *rec;
        bench_use(hash_find(ht, &flows_v4[_i % n], &rec));
    });

    BENCH_RUN("addr_hash_find miss (IPv4)", 1000000, {
        void *rec;
        bench_use(addr_hash_find(ht, &flows_v4[5000 + (_i % 1000)], &rec));
    });

    destroy_addr_hash(ht);

    /* IPv6 */
    ht = addr_hash_create();
    for (int i = 0; i < n; i++)
        addr_hash_insert(ht, &flows_v6[i], &dummy_recs[i]);

    BENCH_RUN("addr_hash_find hit (1000 entries, IPv6)", 1000000, {
        void *rec;
        bench_use(addr_hash_find(ht, &flows_v6[_i % n], &rec));
    });

    destroy_addr_hash(ht);
}

static void bench_find_scaling(void) {
    BENCH_SECTION("Hash Find Scaling (addr_hash_find, IPv4)");

    int sizes[] = {100, 500, 1000, 2000, 5000, 10000};
    int nsizes = sizeof(sizes) / sizeof(sizes[0]);

    for (int s = 0; s < nsizes; s++) {
        int n = sizes[s];
        hash_type *ht = addr_hash_create();
        for (int i = 0; i < n; i++)
            addr_hash_insert(ht, &flows_v4[i], &dummy_recs[i]);

        char label[64];
        snprintf(label, sizeof(label), "find hit (%d entries)", n);
        BENCH_RUN(label, 1000000, {
            void *rec;
            bench_use(addr_hash_find(ht, &flows_v4[_i % n], &rec));
        });

        destroy_addr_hash(ht);
    }
}

static void bench_delete(void) {
    BENCH_SECTION("Hash Delete");

    BENCH_RUN("insert+delete 1000 flows (addr_hash)", 100, {
        hash_type *ht = addr_hash_create();
        for (int j = 0; j < 1000; j++)
            addr_hash_insert(ht, &flows_v4[j], &dummy_recs[j]);
        hash_node_type *node = NULL;
        while (hash_next_item(ht, &node) == HASH_STATUS_OK) {
            hash_node_type *to_delete = node;
            hash_next_item(ht, &node);
            addr_hash_delete_node(ht, to_delete);
            if (node == NULL) break;
        }
        hash_destroy(ht);
        free(ht);
    });
}

static void bench_iteration(void) {
    BENCH_SECTION("Hash Iteration");

    hash_type *ht = addr_hash_create();
    for (int i = 0; i < 1000; i++)
        addr_hash_insert(ht, &flows_v4[i], &dummy_recs[i]);

    BENCH_RUN("iterate 1000 entries (hash_next_item)", 10000, {
        hash_node_type *node = NULL;
        int count = 0;
        while (hash_next_item(ht, &node) == HASH_STATUS_OK)
            count++;
        bench_use(count);
    });

    destroy_addr_hash(ht);
}

static void bench_mixed_workload(void) {
    BENCH_SECTION("Mixed Workload (simulated packet processing)");

    BENCH_RUN("find-or-insert 10000 packets (2000 unique flows)", 100, {
        hash_type *ht = addr_hash_create();
        for (int j = 0; j < 10000; j++) {
            int idx = j % 2000;
            void *rec;
            if (addr_hash_find(ht, &flows_v4[idx], &rec) != HASH_STATUS_OK)
                addr_hash_insert(ht, &flows_v4[idx], &dummy_recs[idx]);
        }
        destroy_addr_hash(ht);
    });

    BENCH_RUN("find-or-insert 10000 packets (200 unique flows)", 100, {
        hash_type *ht = addr_hash_create();
        for (int j = 0; j < 10000; j++) {
            int idx = j % 200;
            void *rec;
            if (addr_hash_find(ht, &flows_v4[idx], &rec) != HASH_STATUS_OK)
                addr_hash_insert(ht, &flows_v4[idx], &dummy_recs[idx]);
        }
        destroy_addr_hash(ht);
    });
}

int main(void) {
    BENCH_HEADER("HASH TABLE PERFORMANCE PROFILER");

    generate_flows();

    bench_hash_distribution();
    bench_insert();
    bench_find();
    bench_find_scaling();
    bench_delete();
    bench_iteration();
    bench_mixed_workload();

    BENCH_FOOTER();
    return 0;
}
