/*
 * bench_vector.c: benchmarks for dynamic vector operations
 *
 * Covers: push_back, pop_back, remove, and reallocation patterns.
 */

#include "bench.h"
#include "vector.h"
#include "iftop.h"

static void bench_push_back(void) {
    BENCH_SECTION("Vector - Push Back");

    int sizes[] = {100, 1000, 10000};
    int nsizes = sizeof(sizes) / sizeof(sizes[0]);

    for (int s = 0; s < nsizes; s++) {
        int n = sizes[s];
        char label[64];
        snprintf(label, sizeof(label), "push_back %d items", n);
        BENCH_RUN(label, 1000, {
            vector v = vector_new();
            for (int j = 0; j < n; j++) {
                vector_push_back(v, item_long(j));
            }
            vector_delete(v);
        });
    }
}

static void bench_push_pop(void) {
    BENCH_SECTION("Vector - Push/Pop Pattern");

    BENCH_RUN("push 1000 then pop 1000", 10000, {
        vector v = vector_new();
        for (int j = 0; j < 1000; j++) {
            vector_push_back(v, item_long(j));
        }
        for (int j = 0; j < 1000; j++) {
            vector_pop_back(v);
        }
        vector_delete(v);
    });

    BENCH_RUN("interleaved push/pop (1000 cycles)", 10000, {
        vector v = vector_new();
        for (int j = 0; j < 1000; j++) {
            vector_push_back(v, item_long(j));
            vector_push_back(v, item_long(j + 1));
            vector_pop_back(v);
        }
        vector_delete(v);
    });
}

static void bench_remove(void) {
    BENCH_SECTION("Vector - Remove (front, causes memmove)");

    int sizes[] = {100, 500, 1000};
    int nsizes = sizeof(sizes) / sizeof(sizes[0]);

    for (int s = 0; s < nsizes; s++) {
        int n = sizes[s];
        char label[64];
        snprintf(label, sizeof(label), "remove from front (%d items)", n);
        BENCH_RUN(label, 100, {
            vector v = vector_new();
            for (int j = 0; j < n; j++) {
                vector_push_back(v, item_long(j));
            }
            while (v->n_used > 0) {
                vector_remove(v, v->items);
            }
            vector_delete(v);
        });
    }
}

static void bench_iterate(void) {
    BENCH_SECTION("Vector - Iteration");

    vector v = vector_new();
    for (int i = 0; i < 10000; i++) {
        vector_push_back(v, item_long(i));
    }

    BENCH_RUN("iterate 10000 items (vector_iterate)", 100000, {
        item *it;
        long sum = 0;
        vector_iterate(v, it) {
            sum += it->num;
        }
        bench_use((int)sum);
    });

    vector_delete(v);
}

static void bench_remove_back(void) {
    BENCH_SECTION("Vector - Remove (back, no memmove)");

    int sizes[] = {100, 500, 1000};
    int nsizes = sizeof(sizes) / sizeof(sizes[0]);

    for (int s = 0; s < nsizes; s++) {
        int n = sizes[s];
        char label[64];
        snprintf(label, sizeof(label), "remove from back (%d items)", n);
        BENCH_RUN(label, 1000, {
            vector v = vector_new();
            for (int j = 0; j < n; j++) {
                vector_push_back(v, item_long(j));
            }
            while (v->n_used > 0) {
                vector_remove(v, &v->items[v->n_used - 1]);
            }
            vector_delete(v);
        });
    }
}

static void bench_reallocate(void) {
    BENCH_SECTION("Vector - Reallocate");

    BENCH_RUN("reallocate grow 16 -> 10000", 10000, {
        vector v = vector_new();
        vector_reallocate(v, 10000);
        bench_use((int)v->capacity);
        vector_delete(v);
    });

    BENCH_RUN("push 100 then reallocate to 10000", 10000, {
        vector v = vector_new();
        for (int j = 0; j < 100; j++) {
            vector_push_back(v, item_long(j));
        }
        vector_reallocate(v, 10000);
        bench_use((int)v->items[99].num);
        vector_delete(v);
    });
}

static void bench_pointer_storage(void) {
    BENCH_SECTION("Vector - Pointer Storage");

    BENCH_RUN("push 1000 string pointers", 1000, {
        vector v = vector_new();
        for (int j = 0; j < 1000; j++) {
            vector_push_back(v, item_ptr(xstrdup("bench_string")));
        }
        vector_delete_free(v);
    });
}

static void bench_remove_middle(void) {
    BENCH_SECTION("Vector - Remove (middle, partial memmove)");

    BENCH_RUN("remove from middle (1000 items)", 100, {
        vector v = vector_new();
        for (int j = 0; j < 1000; j++) {
            vector_push_back(v, item_long(j));
        }
        while (v->n_used > 0) {
            vector_remove(v, &v->items[v->n_used / 2]);
        }
        vector_delete(v);
    });
}

int main(void) {
    BENCH_HEADER("VECTOR OPERATIONS PROFILER");

    bench_push_back();
    bench_push_pop();
    bench_remove();
    bench_remove_back();
    bench_remove_middle();
    bench_iterate();
    bench_reallocate();
    bench_pointer_storage();

    BENCH_FOOTER();
    return 0;
}
