/*
 * bench_stringmap.c: benchmarks for stringmap (binary tree)
 *
 * Covers: insert, find (hit/miss), sequential vs random key patterns,
 * and scaling behavior of the unbalanced BST.
 */

#include "bench.h"
#include "stringmap.h"
#include "iftop.h"

#define MAX_KEYS 10000
static char keys_sequential[MAX_KEYS][32];
static char keys_random[MAX_KEYS][32];

static void generate_keys(void) {
    /* Sequential keys: "key_00000", "key_00001", ... (worst case: right-chain) */
    for (int i = 0; i < MAX_KEYS; i++)
        snprintf(keys_sequential[i], sizeof(keys_sequential[i]), "key_%05d", i);

    /* Pseudo-random keys via LCG to get a more balanced tree */
    uint32_t seed = 0xcafebabe;
    for (int i = 0; i < MAX_KEYS; i++) {
        seed = seed * 1103515245 + 12345;
        snprintf(keys_random[i], sizeof(keys_random[i]), "rnd_%08x", seed);
    }
}

static void bench_insert_sequential(void) {
    BENCH_SECTION("Stringmap - Insert Sequential Keys (worst-case: right chain)");

    int sizes[] = {100, 500, 1000, 2000, 5000};
    int nsizes = sizeof(sizes) / sizeof(sizes[0]);

    for (int s = 0; s < nsizes; s++) {
        int n = sizes[s];
        char label[64];
        snprintf(label, sizeof(label), "insert %d sequential keys", n);
        BENCH_RUN(label, (n <= 1000 ? 100 : 10), {
            stringmap sm = stringmap_new();
            for (int j = 0; j < n; j++)
                stringmap_insert(sm, keys_sequential[j], item_long(j));
            stringmap_delete(sm);
        });
    }
}

static void bench_insert_random(void) {
    BENCH_SECTION("Stringmap - Insert Random Keys (balanced-ish)");

    int sizes[] = {100, 500, 1000, 2000, 5000, 10000};
    int nsizes = sizeof(sizes) / sizeof(sizes[0]);

    for (int s = 0; s < nsizes; s++) {
        int n = sizes[s];
        char label[64];
        snprintf(label, sizeof(label), "insert %d random keys", n);
        BENCH_RUN(label, (n <= 1000 ? 1000 : 100), {
            stringmap sm = stringmap_new();
            for (int j = 0; j < n; j++)
                stringmap_insert(sm, keys_random[j], item_long(j));
            stringmap_delete(sm);
        });
    }
}

static void bench_find_hit(void) {
    BENCH_SECTION("Stringmap - Find (hit)");

    /* Sequential tree (deep, right chain) */
    int n = 1000;
    stringmap sm_seq = stringmap_new();
    for (int i = 0; i < n; i++)
        stringmap_insert(sm_seq, keys_sequential[i], item_long(i));

    BENCH_RUN("find hit in 1000 sequential keys", 1000000, {
        bench_use(stringmap_find(sm_seq, keys_sequential[_i % n]) != NULL);
    });

    stringmap_delete(sm_seq);

    /* Random tree (more balanced) */
    stringmap sm_rnd = stringmap_new();
    for (int i = 0; i < n; i++)
        stringmap_insert(sm_rnd, keys_random[i], item_long(i));

    BENCH_RUN("find hit in 1000 random keys", 1000000, {
        bench_use(stringmap_find(sm_rnd, keys_random[_i % n]) != NULL);
    });

    stringmap_delete(sm_rnd);
}

static void bench_find_miss(void) {
    BENCH_SECTION("Stringmap - Find (miss)");

    int n = 1000;
    stringmap sm = stringmap_new();
    for (int i = 0; i < n; i++)
        stringmap_insert(sm, keys_random[i], item_long(i));

    /* Miss keys are in a different range */
    static char miss_keys[1000][32];
    for (int i = 0; i < 1000; i++)
        snprintf(miss_keys[i], sizeof(miss_keys[i]), "miss_%05d", i);

    BENCH_RUN("find miss in 1000 random keys", 1000000, {
        bench_use(stringmap_find(sm, miss_keys[_i % 1000]) == NULL);
    });

    stringmap_delete(sm);
}

static void bench_find_scaling(void) {
    BENCH_SECTION("Stringmap - Find Scaling (random keys)");

    int sizes[] = {100, 500, 1000, 2000, 5000, 10000};
    int nsizes = sizeof(sizes) / sizeof(sizes[0]);

    for (int s = 0; s < nsizes; s++) {
        int n = sizes[s];
        stringmap sm = stringmap_new();
        for (int i = 0; i < n; i++)
            stringmap_insert(sm, keys_random[i], item_long(i));

        char label[64];
        snprintf(label, sizeof(label), "find hit (%d entries)", n);
        BENCH_RUN(label, 1000000, {
            bench_use(stringmap_find(sm, keys_random[_i % n]) != NULL);
        });

        stringmap_delete(sm);
    }
}

static void bench_insert_find_pattern(void) {
    BENCH_SECTION("Stringmap - Insert + Find Pattern (config-like workload)");

    /* Simulates config usage: insert a few keys, then many lookups */
    BENCH_RUN("insert 20 + find 10000 (config-style)", 10000, {
        stringmap sm = stringmap_new();
        for (int j = 0; j < 20; j++)
            stringmap_insert(sm, keys_random[j], item_long(j));
        for (int j = 0; j < 10000; j++)
            bench_use(stringmap_find(sm, keys_random[j % 20]) != NULL);
        stringmap_delete(sm);
    });
}

static void bench_duplicate_insert(void) {
    BENCH_SECTION("Stringmap - Duplicate Key Insert (update pattern)");

    int n = 1000;
    stringmap sm = stringmap_new();
    for (int i = 0; i < n; i++)
        stringmap_insert(sm, keys_random[i], item_long(i));

    BENCH_RUN("insert 1000 duplicates (returns existing)", 10000, {
        for (int j = 0; j < n; j++) {
            item *existing = stringmap_insert(sm, keys_random[j], item_long(j + 1));
            if (existing) bench_use((int)existing->num);
        }
    });

    stringmap_delete(sm);
}

int main(void) {
    printf("iftopcolor stringmap benchmark suite\n");
    printf("=====================================\n");

    generate_keys();

    bench_insert_sequential();
    bench_insert_random();
    bench_find_hit();
    bench_find_miss();
    bench_find_scaling();
    bench_insert_find_pattern();
    bench_duplicate_insert();

    printf("\nDone.\n");
    return 0;
}
