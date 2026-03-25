/*
 * bench_ns_hash.c: benchmarks for ns_hash (IPv6 address to name cache)
 *
 * Covers: insert, find (hit/miss), delete, iteration, and scaling.
 */

#include <arpa/inet.h>
#include "bench.h"
#include "ns_hash.h"
#include "hash.h"
#include "iftop.h"

#define MAX_ADDRS 5000
static struct in6_addr addrs[MAX_ADDRS];
static char names[MAX_ADDRS][64];

static void generate_addrs(void) {
    for (int i = 0; i < MAX_ADDRS; i++) {
        char addr_str[64];
        snprintf(addr_str, sizeof(addr_str), "2001:db8:%x::%x",
                 (i / 256) + 1, (i % 256) + 1);
        memset(&addrs[i], 0, sizeof(struct in6_addr));
        inet_pton(AF_INET6, addr_str, &addrs[i]);
        snprintf(names[i], sizeof(names[i]), "host-%d.example.com", i);
    }
}

static int ns_hash_count(hash_type *h) {
    int count = 0;
    hash_node_type *node = NULL;
    while (hash_next_item(h, &node) == HASH_STATUS_OK) count++;
    return count;
}

static void bench_insert(void) {
    BENCH_SECTION("NS Hash - Insert");

    int sizes[] = {50, 100, 200, 500, 1000};
    int nsizes = sizeof(sizes) / sizeof(sizes[0]);

    for (int s = 0; s < nsizes; s++) {
        int n = sizes[s];
        char label[64];
        snprintf(label, sizeof(label), "insert %d IPv6 addresses", n);
        BENCH_RUN(label, 1000, {
            hash_type *h = ns_hash_create();
            for (int j = 0; j < n; j++)
                hash_insert(h, &addrs[j], xstrdup(names[j]));
            hash_delete_all_free(h);
            hash_destroy(h);
            free(h);
        });
    }
}

static void bench_find_hit(void) {
    BENCH_SECTION("NS Hash - Find (hit)");

    int sizes[] = {50, 100, 200, 500};
    int nsizes = sizeof(sizes) / sizeof(sizes[0]);

    for (int s = 0; s < nsizes; s++) {
        int n = sizes[s];
        hash_type *h = ns_hash_create();
        for (int i = 0; i < n; i++)
            hash_insert(h, &addrs[i], xstrdup(names[i]));

        char label[64];
        snprintf(label, sizeof(label), "find hit (%d entries)", n);
        BENCH_RUN(label, 1000000, {
            void *rec;
            bench_use(hash_find(h, &addrs[_i % n], &rec));
        });

        hash_delete_all_free(h);
        hash_destroy(h);
        free(h);
    }
}

static void bench_find_miss(void) {
    BENCH_SECTION("NS Hash - Find (miss)");

    int n = 200;
    hash_type *h = ns_hash_create();
    for (int i = 0; i < n; i++)
        hash_insert(h, &addrs[i], xstrdup(names[i]));

    /* Miss addresses are from a different range */
    BENCH_RUN("find miss (200 entries)", 1000000, {
        void *rec;
        bench_use(hash_find(h, &addrs[1000 + (_i % 1000)], &rec));
    });

    hash_delete_all_free(h);
    hash_destroy(h);
    free(h);
}

static void bench_delete(void) {
    BENCH_SECTION("NS Hash - Insert + Delete");

    BENCH_RUN("insert 200 + delete all", 1000, {
        hash_type *h = ns_hash_create();
        for (int j = 0; j < 200; j++)
            hash_insert(h, &addrs[j], xstrdup(names[j]));
        hash_delete_all_free(h);
        hash_destroy(h);
        free(h);
    });

    BENCH_RUN("insert 200 + delete individually", 1000, {
        hash_type *h = ns_hash_create();
        for (int j = 0; j < 200; j++)
            hash_insert(h, &addrs[j], xstrdup(names[j]));
        for (int j = 0; j < 200; j++)
            hash_delete(h, &addrs[j]);
        hash_destroy(h);
        free(h);
    });
}

static void bench_iteration(void) {
    BENCH_SECTION("NS Hash - Iteration");

    int n = 500;
    hash_type *h = ns_hash_create();
    for (int i = 0; i < n; i++)
        hash_insert(h, &addrs[i], xstrdup(names[i]));

    BENCH_RUN("iterate 500 entries", 100000, {
        int count = 0;
        hash_node_type *node = NULL;
        while (hash_next_item(h, &node) == HASH_STATUS_OK)
            count++;
        bench_use(count);
    });

    hash_delete_all_free(h);
    hash_destroy(h);
    free(h);
}

static void bench_churn(void) {
    BENCH_SECTION("NS Hash - Churn (insert/delete cycles)");

    BENCH_RUN("churn 100 insert/delete x 20 cycles", 100, {
        hash_type *h = ns_hash_create();
        for (int cycle = 0; cycle < 20; cycle++) {
            for (int j = 0; j < 100; j++)
                hash_insert(h, &addrs[j], xstrdup(names[j]));
            hash_delete_all_free(h);
        }
        hash_destroy(h);
        free(h);
    });
}

int main(void) {
    printf("iftopcolor ns_hash benchmark suite\n");
    printf("===================================\n");

    generate_addrs();

    bench_insert();
    bench_find_hit();
    bench_find_miss();
    bench_delete();
    bench_iteration();
    bench_churn();

    printf("\nDone.\n");
    return 0;
}
