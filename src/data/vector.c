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
    vector v;

    v = xcalloc(1, sizeof *v);
    if (!v) return NULL;

    v->items = xcalloc(16, sizeof *v->items);
    v->capacity = 16;
    v->n_used = 0;
    return v;
}

void vector_delete(vector v) {
    xfree(v->items);
    xfree(v);
}

void vector_delete_free(vector v) {
    item *i;
    vector_iterate(v, i) {
        xfree(i->ptr);
    }
    xfree(v->items);
    xfree(v);
}

void vector_push_back(vector v, const item t) {
    if (v->n_used == v->capacity) vector_reallocate(v, v->capacity * 2);
    v->items[v->n_used++] = t;
}

void vector_pop_back(vector v) {
    if (v->n_used > 0) {
        --v->n_used;
        if (v->n_used < v->capacity / 2) vector_reallocate(v, v->capacity / 2);
    }
}

item vector_back(vector v) {
    return v->items[v->n_used - 1];
}

item *vector_remove(vector v, item *t) {
    if (t >= v->items + v->n_used) return NULL;
    if (t < v->items + v->n_used - 1)
        memmove(t, t + 1, (v->n_used - (t - v->items)) * sizeof(item));
    memset(v->items + v->n_used--, 0, sizeof(item));
    if (v->n_used < v->capacity / 2 && v->capacity > 16) {
        size_t i = t - v->items;
        vector_reallocate(v, v->capacity / 2);
        t = v->items + i;
    }
    return t;
}

void vector_reallocate(vector v, const size_t capacity) {
    size_t clear_count;
    if (capacity < v->n_used || capacity <= 0) return;
    v->items = xrealloc(v->items, capacity * sizeof(item));
    clear_count = capacity - v->n_used;
    if (clear_count > 0)
        memset(v->items + v->n_used, 0, clear_count * sizeof(item));
    v->capacity = capacity;
}
