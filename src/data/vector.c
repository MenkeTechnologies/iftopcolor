/*
 * vector.c:
 * simple vectors
 *
 */

#include <stdlib.h>
#include <string.h>

#include "../include/vector.h"
#include "../include/iftop.h"

vector vector_new(void) {
    vector vec;

    vec = xcalloc(1, sizeof *vec);
    if (!vec) return NULL;

    vec->items = xcalloc(16, sizeof *vec->items);
    vec->capacity = 16;
    vec->n_used = 0;
    return vec;
}

void vector_delete(vector vec) {
    xfree(vec->items);
    xfree(vec);
}

void vector_delete_free(vector vec) {
    item *iter;
    vector_iterate(vec, iter) {
        xfree(iter->ptr);
    }
    xfree(vec->items);
    xfree(vec);
}

void vector_push_back(vector vec, const item elem) {
    if (vec->n_used == vec->capacity) vector_reallocate(vec, vec->capacity * 2);
    vec->items[vec->n_used++] = elem;
}

void vector_pop_back(vector vec) {
    if (vec->n_used > 0) {
        --vec->n_used;
        if (vec->n_used < vec->capacity / 2) vector_reallocate(vec, vec->capacity / 2);
    }
}

item vector_back(vector vec) {
    return vec->items[vec->n_used - 1];
}

item *vector_remove(vector vec, item *target) {
    if (target >= vec->items + vec->n_used) return NULL;
    if (target < vec->items + vec->n_used - 1)
        memmove(target, target + 1, (vec->n_used - (target - vec->items)) * sizeof(item));
    memset(vec->items + vec->n_used--, 0, sizeof(item));
    if (vec->n_used < vec->capacity / 2 && vec->capacity > 16) {
        size_t idx = target - vec->items;
        vector_reallocate(vec, vec->capacity / 2);
        target = vec->items + idx;
    }
    return target;
}

void vector_reallocate(vector vec, const size_t capacity) {
    size_t clear_count;
    if (capacity < vec->n_used || capacity <= 0) return;
    vec->items = xrealloc(vec->items, capacity * sizeof(item));
    clear_count = capacity - vec->n_used;
    if (clear_count > 0)
        memset(vec->items + vec->n_used, 0, clear_count * sizeof(item));
    vec->capacity = capacity;
}
