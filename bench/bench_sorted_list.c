/*
 * bench_sorted_list.c: benchmarks for sorted list operations
 *
 * Covers: single insert vs batch insert, iteration, and scaling behavior.
 */

#include "bench.h"
#include "sorted_list.h"
#include "iftop.h"

/* Comparison function: sort longs descending (like bandwidth display) */
static int compare_long_desc(void *left, void *right) {
    long la = (long)(intptr_t)left;
    long lb = (long)(intptr_t)right;
    if (la < lb) return 1;
    if (la > lb) return -1;
    return 0;
}

/* Pre-generate random-ish values */
#define MAX_ITEMS 10000
static void *items[MAX_ITEMS];
static void *items_batch[MAX_ITEMS]; /* separate copy for batch */

static void generate_items(void) {
    /* LCG for deterministic pseudo-random values */
    uint32_t seed = 0xdeadbeef;
    for (int i = 0; i < MAX_ITEMS; i++) {
        seed = seed * 1103515245 + 12345;
        items[i] = (void *)(intptr_t)(seed % 1000000);
        items_batch[i] = items[i];
    }
}

static void bench_single_insert(void) {
    BENCH_SECTION("Sorted List - Single Insert (O(n^2))");

    int sizes[] = {100, 500, 1000, 2000, 5000};
    int nsizes = sizeof(sizes) / sizeof(sizes[0]);

    for (int s = 0; s < nsizes; s++) {
        int n = sizes[s];
        char label[64];
        snprintf(label, sizeof(label), "insert %d items (one-by-one)", n);
        BENCH_RUN(label, (n <= 1000 ? 100 : 10), {
            sorted_list_type list;
            list.compare = compare_long_desc;
            sorted_list_initialise(&list);
            for (int j = 0; j < n; j++) {
                sorted_list_insert(&list, items[j]);
            }
            sorted_list_destroy(&list);
        });
    }
}

static void bench_batch_insert(void) {
    BENCH_SECTION("Sorted List - Batch Insert (O(n log n))");

    int sizes[] = {100, 500, 1000, 2000, 5000, 10000};
    int nsizes = sizeof(sizes) / sizeof(sizes[0]);

    for (int s = 0; s < nsizes; s++) {
        int n = sizes[s];
        char label[64];
        snprintf(label, sizeof(label), "insert %d items (batch)", n);
        BENCH_RUN(label, 1000, {
            /* Restore items_batch since qsort modifies in place */
            memcpy(items_batch, items, n * sizeof(void *));
            sorted_list_type list;
            list.compare = compare_long_desc;
            sorted_list_initialise(&list);
            sorted_list_insert_batch(&list, items_batch, n);
            sorted_list_destroy(&list);
        });
    }
}

static void bench_iteration(void) {
    BENCH_SECTION("Sorted List - Iteration");

    int n = 1000;
    memcpy(items_batch, items, n * sizeof(void *));
    sorted_list_type list;
    list.compare = compare_long_desc;
    sorted_list_initialise(&list);
    sorted_list_insert_batch(&list, items_batch, n);

    BENCH_RUN("iterate 1000 items", 100000, {
        sorted_list_node *node = NULL;
        int count = 0;
        while ((node = sorted_list_next_item(&list, node)) != NULL) {
            count++;
        }
        bench_use(count);
    });

    sorted_list_destroy(&list);
}

static void bench_single_vs_batch(void) {
    BENCH_SECTION("Sorted List - Single vs Batch Speedup");

    int n = 1000;

    uint64_t t0 = bench_now_ns();
    for (int r = 0; r < 100; r++) {
        sorted_list_type list;
        list.compare = compare_long_desc;
        sorted_list_initialise(&list);
        for (int j = 0; j < n; j++) {
            sorted_list_insert(&list, items[j]);
        }
        sorted_list_destroy(&list);
    }
    uint64_t single_ns = bench_now_ns() - t0;

    t0 = bench_now_ns();
    for (int r = 0; r < 100; r++) {
        memcpy(items_batch, items, n * sizeof(void *));
        sorted_list_type list;
        list.compare = compare_long_desc;
        sorted_list_initialise(&list);
        sorted_list_insert_batch(&list, items_batch, n);
        sorted_list_destroy(&list);
    }
    uint64_t batch_ns = bench_now_ns() - t0;

    printf("  %d items x100: single=%.2fms  batch=%.2fms  speedup=%.1fx\n",
           n, single_ns / 1e6, batch_ns / 1e6,
           (double)single_ns / (double)batch_ns);
}

int main(void) {
    printf("iftopcolor sorted_list benchmark suite\n");
    printf("=======================================\n");

    generate_items();

    bench_single_insert();
    bench_batch_insert();
    bench_iteration();
    bench_single_vs_batch();

    printf("\nDone.\n");
    return 0;
}
