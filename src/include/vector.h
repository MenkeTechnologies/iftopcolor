/*
 * vector.h:
 * simple vectors
 *
 */

#ifndef __VECTOR_H_ /* include guard */
#define __VECTOR_H_

typedef union _item {
    void *ptr;
    long num;
} item;

#define _inline inline

static _inline item item_long(const long num) {
    item u;
    u.num = num;
    return u;
}

static _inline item item_ptr(void *const ptr) {
    item u;
    u.ptr = ptr;
    return u;
}

typedef struct _vector {
    item *items;
    size_t capacity, n_used;
} *vector;

vector vector_new(void);

void vector_delete(vector);

void vector_delete_free(vector);

void vector_push_back(vector, const item);

void vector_pop_back(vector);

item vector_back(const vector);

item *vector_remove(vector, item *t);

void vector_reallocate(vector, const size_t capacity);

/* A macro to iterate over a vector */
#define vector_iterate(_v, _t)  for ((_t) = (_v)->items; (_t) < (_v)->items + (_v)->n_used; ++(_t))

#endif /* __VECTOR_H_ */
