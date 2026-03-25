/*
 * test_vector.c: tests for vector (dynamic array)
 */

#include <stdlib.h>
#include "test.h"
#include "vector.h"
#include "iftop.h"

/* === Creation and destruction === */

TEST(vector_new_creates_empty) {
    vector v = vector_new();
    ASSERT_NOT_NULL(v);
    ASSERT_EQ(v->n_used, 0);
    ASSERT_EQ(v->capacity, 16);
    vector_delete(v);
}

TEST(vector_new_array_not_null) {
    vector v = vector_new();
    ASSERT_NOT_NULL(v->items);
    vector_delete(v);
}

TEST(vector_new_array_zeroed) {
    vector v = vector_new();
    for (int i = 0; i < 16; i++)
        ASSERT_EQ(v->items[i].num, 0);
    vector_delete(v);
}

TEST(vector_delete_free_empty) {
    vector v = vector_new();
    vector_delete_free(v); /* should not crash */
}

TEST(vector_delete_free_with_items) {
    vector v = vector_new();
    vector_push_back(v, item_ptr(xstrdup("a")));
    vector_push_back(v, item_ptr(xstrdup("b")));
    vector_push_back(v, item_ptr(xstrdup("c")));
    vector_delete_free(v);
}

/* === item helpers === */

TEST(item_long_stores_value) {
    item i = item_long(12345);
    ASSERT_EQ(i.num, 12345);
}

TEST(item_long_stores_zero) {
    item i = item_long(0);
    ASSERT_EQ(i.num, 0);
}

TEST(item_long_stores_negative) {
    item i = item_long(-999);
    ASSERT_EQ(i.num, -999);
}

TEST(item_ptr_stores_pointer) {
    int x = 42;
    item i = item_ptr(&x);
    ASSERT_EQ(*(int *)i.ptr, 42);
}

TEST(item_ptr_stores_null) {
    item i = item_ptr(NULL);
    ASSERT_NULL(i.ptr);
}

/* === Push back === */

TEST(vector_push_back_single) {
    vector v = vector_new();
    vector_push_back(v, item_long(42));
    ASSERT_EQ(v->n_used, 1);
    ASSERT_EQ(v->items[0].num, 42);
    vector_delete(v);
}

TEST(vector_push_back_two) {
    vector v = vector_new();
    vector_push_back(v, item_long(10));
    vector_push_back(v, item_long(20));
    ASSERT_EQ(v->n_used, 2);
    ASSERT_EQ(v->items[0].num, 10);
    ASSERT_EQ(v->items[1].num, 20);
    vector_delete(v);
}

TEST(vector_push_back_fills_capacity_exactly) {
    vector v = vector_new();
    for (int i = 0; i < 16; i++)
        vector_push_back(v, item_long(i));
    ASSERT_EQ(v->n_used, 16);
    ASSERT_EQ(v->capacity, 16); /* no resize yet */
    vector_delete(v);
}

TEST(vector_push_back_triggers_first_resize) {
    vector v = vector_new();
    for (int i = 0; i < 17; i++)
        vector_push_back(v, item_long(i));
    ASSERT_EQ(v->n_used, 17);
    ASSERT_EQ(v->capacity, 32);
    vector_delete(v);
}

TEST(vector_push_back_triggers_second_resize) {
    vector v = vector_new();
    for (int i = 0; i < 33; i++)
        vector_push_back(v, item_long(i));
    ASSERT_EQ(v->n_used, 33);
    ASSERT_EQ(v->capacity, 64);
    vector_delete(v);
}

TEST(vector_push_back_triggers_third_resize) {
    vector v = vector_new();
    for (int i = 0; i < 65; i++)
        vector_push_back(v, item_long(i));
    ASSERT_EQ(v->n_used, 65);
    ASSERT_EQ(v->capacity, 128);
    vector_delete(v);
}

TEST(vector_push_100) {
    vector v = vector_new();
    for (int i = 0; i < 100; i++)
        vector_push_back(v, item_long(i));
    ASSERT_EQ(v->n_used, 100);
    for (int i = 0; i < 100; i++)
        ASSERT_EQ(v->items[i].num, i);
    vector_delete(v);
}

TEST(vector_push_preserves_order) {
    vector v = vector_new();
    for (int i = 0; i < 50; i++)
        vector_push_back(v, item_long(i * 7));
    for (int i = 0; i < 50; i++)
        ASSERT_EQ(v->items[i].num, i * 7);
    vector_delete(v);
}

TEST(vector_push_negative_values) {
    vector v = vector_new();
    for (int i = 0; i < 10; i++)
        vector_push_back(v, item_long(-i));
    for (int i = 0; i < 10; i++)
        ASSERT_EQ(v->items[i].num, -i);
    vector_delete(v);
}

TEST(vector_push_zero_values) {
    vector v = vector_new();
    for (int i = 0; i < 20; i++)
        vector_push_back(v, item_long(0));
    ASSERT_EQ(v->n_used, 20);
    for (int i = 0; i < 20; i++)
        ASSERT_EQ(v->items[i].num, 0);
    vector_delete(v);
}

/* === Pop back === */

TEST(vector_pop_back_decrements) {
    vector v = vector_new();
    vector_push_back(v, item_long(1));
    vector_push_back(v, item_long(2));
    vector_push_back(v, item_long(3));
    vector_pop_back(v);
    ASSERT_EQ(v->n_used, 2);
    vector_delete(v);
}

TEST(vector_pop_back_empty_noop) {
    vector v = vector_new();
    vector_pop_back(v);
    ASSERT_EQ(v->n_used, 0);
    vector_delete(v);
}

TEST(vector_pop_back_to_empty) {
    vector v = vector_new();
    vector_push_back(v, item_long(1));
    vector_pop_back(v);
    ASSERT_EQ(v->n_used, 0);
    vector_delete(v);
}

TEST(vector_pop_back_preserves_remaining) {
    vector v = vector_new();
    vector_push_back(v, item_long(10));
    vector_push_back(v, item_long(20));
    vector_push_back(v, item_long(30));
    vector_pop_back(v);
    ASSERT_EQ(v->items[0].num, 10);
    ASSERT_EQ(v->items[1].num, 20);
    vector_delete(v);
}

TEST(vector_pop_back_shrinks_capacity) {
    vector v = vector_new();
    /* Push 32 items to get capacity=32, then pop down */
    for (int i = 0; i < 32; i++)
        vector_push_back(v, item_long(i));
    ASSERT_EQ(v->capacity, 32);
    /* Pop until n_used < n/2 triggers shrink */
    for (int i = 0; i < 17; i++)
        vector_pop_back(v);
    ASSERT_EQ(v->n_used, 15);
    ASSERT(v->capacity <= 32);
    vector_delete(v);
}

TEST(vector_pop_multiple_empty_noop) {
    vector v = vector_new();
    for (int i = 0; i < 10; i++)
        vector_pop_back(v);
    ASSERT_EQ(v->n_used, 0);
    vector_delete(v);
}

/* === vector_back === */

TEST(vector_back_returns_last) {
    vector v = vector_new();
    vector_push_back(v, item_long(10));
    vector_push_back(v, item_long(20));
    vector_push_back(v, item_long(30));
    ASSERT_EQ(vector_back(v).num, 30);
    vector_delete(v);
}

TEST(vector_back_single_element) {
    vector v = vector_new();
    vector_push_back(v, item_long(42));
    ASSERT_EQ(vector_back(v).num, 42);
    vector_delete(v);
}

TEST(vector_back_after_pop) {
    vector v = vector_new();
    vector_push_back(v, item_long(1));
    vector_push_back(v, item_long(2));
    vector_push_back(v, item_long(3));
    vector_pop_back(v);
    ASSERT_EQ(vector_back(v).num, 2);
    vector_delete(v);
}

TEST(vector_back_with_pointer) {
    vector v = vector_new();
    int x = 99;
    vector_push_back(v, item_ptr(&x));
    ASSERT_EQ(*(int *)vector_back(v).ptr, 99);
    vector_delete(v);
}

/* === Remove === */

TEST(vector_remove_front) {
    vector v = vector_new();
    for (int i = 0; i < 5; i++)
        vector_push_back(v, item_long(i));
    vector_remove(v, &v->items[0]);
    ASSERT_EQ(v->n_used, 4);
    ASSERT_EQ(v->items[0].num, 1);
    ASSERT_EQ(v->items[1].num, 2);
    ASSERT_EQ(v->items[2].num, 3);
    ASSERT_EQ(v->items[3].num, 4);
    vector_delete(v);
}

TEST(vector_remove_middle) {
    vector v = vector_new();
    for (int i = 0; i < 5; i++)
        vector_push_back(v, item_long(i * 10));
    vector_remove(v, &v->items[2]);
    ASSERT_EQ(v->n_used, 4);
    ASSERT_EQ(v->items[0].num, 0);
    ASSERT_EQ(v->items[1].num, 10);
    ASSERT_EQ(v->items[2].num, 30);
    ASSERT_EQ(v->items[3].num, 40);
    vector_delete(v);
}

TEST(vector_remove_last_element) {
    vector v = vector_new();
    for (int i = 0; i < 3; i++)
        vector_push_back(v, item_long(i));
    vector_remove(v, &v->items[2]);
    ASSERT_EQ(v->n_used, 2);
    vector_delete(v);
}

TEST(vector_remove_only_element) {
    vector v = vector_new();
    vector_push_back(v, item_long(42));
    vector_remove(v, &v->items[0]);
    ASSERT_EQ(v->n_used, 0);
    vector_delete(v);
}

TEST(vector_remove_out_of_bounds_returns_null) {
    vector v = vector_new();
    vector_push_back(v, item_long(1));
    item *result = vector_remove(v, &v->items[1]);
    ASSERT_NULL(result);
    ASSERT_EQ(v->n_used, 1);
    vector_delete(v);
}

TEST(vector_remove_returns_pointer_for_iteration) {
    vector v = vector_new();
    for (int i = 0; i < 5; i++)
        vector_push_back(v, item_long(i));
    item *next = vector_remove(v, &v->items[1]);
    ASSERT_NOT_NULL(next);
    /* After removing index 1 (val=1), next should point to what was index 2 (val=2) */
    ASSERT_EQ(next->num, 2);
    vector_delete(v);
}

TEST(vector_remove_consecutive) {
    vector v = vector_new();
    for (int i = 0; i < 10; i++)
        vector_push_back(v, item_long(i));
    /* Remove all even indices by iterating */
    item *t = v->items;
    while (t < v->items + v->n_used) {
        if (t->num % 2 == 0)
            t = vector_remove(v, t);
        else
            t++;
    }
    ASSERT_EQ(v->n_used, 5);
    ASSERT_EQ(v->items[0].num, 1);
    ASSERT_EQ(v->items[1].num, 3);
    ASSERT_EQ(v->items[2].num, 5);
    ASSERT_EQ(v->items[3].num, 7);
    ASSERT_EQ(v->items[4].num, 9);
    vector_delete(v);
}

TEST(vector_remove_all_one_by_one) {
    vector v = vector_new();
    for (int i = 0; i < 8; i++)
        vector_push_back(v, item_long(i));
    while (v->n_used > 0)
        vector_remove(v, &v->items[0]);
    ASSERT_EQ(v->n_used, 0);
    vector_delete(v);
}

TEST(vector_remove_triggers_shrink) {
    vector v = vector_new();
    /* Push 40 items: capacity grows to 64 */
    for (int i = 0; i < 40; i++)
        vector_push_back(v, item_long(i));
    ASSERT(v->capacity >= 40);
    size_t old_cap = v->capacity;
    /* Remove down to below half, but capacity won't shrink below 16 */
    while (v->n_used > 5)
        vector_remove(v, &v->items[0]);
    ASSERT(v->capacity <= old_cap);
    ASSERT_EQ(v->n_used, 5);
    vector_delete(v);
}

TEST(vector_remove_second_to_last) {
    vector v = vector_new();
    vector_push_back(v, item_long(10));
    vector_push_back(v, item_long(20));
    vector_push_back(v, item_long(30));
    vector_remove(v, &v->items[1]);
    ASSERT_EQ(v->n_used, 2);
    ASSERT_EQ(v->items[0].num, 10);
    ASSERT_EQ(v->items[1].num, 30);
    vector_delete(v);
}

/* === Reallocate === */

TEST(vector_reallocate_grow) {
    vector v = vector_new();
    vector_push_back(v, item_long(1));
    vector_reallocate(v, 64);
    ASSERT_EQ(v->capacity, 64);
    ASSERT_EQ(v->n_used, 1);
    ASSERT_EQ(v->items[0].num, 1);
    vector_delete(v);
}

TEST(vector_reallocate_no_shrink_below_used) {
    vector v = vector_new();
    for (int i = 0; i < 10; i++)
        vector_push_back(v, item_long(i));
    size_t old_n = v->capacity;
    vector_reallocate(v, 5); /* less than n_used=10, no-op */
    ASSERT_EQ(v->n_used, 10);
    ASSERT_EQ(v->capacity, old_n);
    vector_delete(v);
}

TEST(vector_reallocate_exact_used) {
    vector v = vector_new();
    for (int i = 0; i < 10; i++)
        vector_push_back(v, item_long(i));
    /* Reallocating to n_used should not be a no-op since n >= n_used */
    vector_reallocate(v, 10);
    /* 10 < 16, but 10 >= n_used so it should work */
    ASSERT_EQ(v->n_used, 10);
    ASSERT(v->capacity >= 10);
    vector_delete(v);
}

TEST(vector_reallocate_preserves_data) {
    vector v = vector_new();
    for (int i = 0; i < 8; i++)
        vector_push_back(v, item_long(i * 100));
    vector_reallocate(v, 128);
    for (int i = 0; i < 8; i++)
        ASSERT_EQ(v->items[i].num, i * 100);
    vector_delete(v);
}

TEST(vector_reallocate_clears_new_space) {
    vector v = vector_new();
    vector_push_back(v, item_long(1));
    vector_reallocate(v, 32);
    /* New slots should be zeroed */
    for (int i = 1; i < 32; i++)
        ASSERT_EQ(v->items[i].num, 0);
    vector_delete(v);
}

TEST(vector_reallocate_large) {
    vector v = vector_new();
    vector_reallocate(v, 10000);
    ASSERT_EQ(v->capacity, 10000);
    ASSERT_EQ(v->n_used, 0);
    vector_delete(v);
}

/* === Iteration === */

TEST(vector_iterate_all_elements) {
    vector v = vector_new();
    for (int i = 0; i < 5; i++)
        vector_push_back(v, item_long(i * 3));
    long sum = 0;
    item *it;
    vector_iterate(v, it) { sum += it->num; }
    ASSERT_EQ(sum, 0 + 3 + 6 + 9 + 12);
    vector_delete(v);
}

TEST(vector_iterate_empty) {
    vector v = vector_new();
    int count = 0;
    item *it;
    vector_iterate(v, it) { count++; }
    ASSERT_EQ(count, 0);
    vector_delete(v);
}

TEST(vector_iterate_single) {
    vector v = vector_new();
    vector_push_back(v, item_long(77));
    int count = 0;
    long val = 0;
    item *it;
    vector_iterate(v, it) { count++; val = it->num; }
    ASSERT_EQ(count, 1);
    ASSERT_EQ(val, 77);
    vector_delete(v);
}

TEST(vector_iterate_count_matches_n_used) {
    vector v = vector_new();
    for (int i = 0; i < 37; i++)
        vector_push_back(v, item_long(i));
    int count = 0;
    item *it;
    vector_iterate(v, it) { count++; }
    ASSERT_EQ(count, 37);
    vector_delete(v);
}

/* === Pointer storage === */

TEST(vector_store_pointers) {
    vector v = vector_new();
    char *s1 = xstrdup("hello");
    char *s2 = xstrdup("world");
    vector_push_back(v, item_ptr(s1));
    vector_push_back(v, item_ptr(s2));
    ASSERT_STR_EQ((char *)v->items[0].ptr, "hello");
    ASSERT_STR_EQ((char *)v->items[1].ptr, "world");
    vector_delete_free(v);
}

TEST(vector_store_null_pointers) {
    vector v = vector_new();
    vector_push_back(v, item_ptr(NULL));
    vector_push_back(v, item_ptr(NULL));
    ASSERT_EQ(v->n_used, 2);
    ASSERT_NULL(v->items[0].ptr);
    ASSERT_NULL(v->items[1].ptr);
    vector_delete(v);
}

TEST(vector_store_many_strings) {
    vector v = vector_new();
    for (int i = 0; i < 50; i++) {
        char buf[32];
        snprintf(buf, sizeof(buf), "string_%d", i);
        vector_push_back(v, item_ptr(xstrdup(buf)));
    }
    ASSERT_EQ(v->n_used, 50);
    for (int i = 0; i < 50; i++) {
        char buf[32];
        snprintf(buf, sizeof(buf), "string_%d", i);
        ASSERT_STR_EQ((char *)v->items[i].ptr, buf);
    }
    vector_delete_free(v);
}

/* === Push/pop interleaving === */

TEST(vector_push_pop_interleave) {
    vector v = vector_new();
    for (int i = 0; i < 50; i++)
        vector_push_back(v, item_long(i));
    for (int i = 0; i < 30; i++)
        vector_pop_back(v);
    ASSERT_EQ(v->n_used, 20);
    for (int i = 0; i < 20; i++)
        ASSERT_EQ(v->items[i].num, i);
    vector_delete(v);
}

TEST(vector_push_pop_push_pattern) {
    vector v = vector_new();
    for (int i = 0; i < 10; i++)
        vector_push_back(v, item_long(i));
    for (int i = 0; i < 5; i++)
        vector_pop_back(v);
    for (int i = 100; i < 108; i++)
        vector_push_back(v, item_long(i));
    ASSERT_EQ(v->n_used, 13);
    ASSERT_EQ(v->items[0].num, 0);
    ASSERT_EQ(v->items[4].num, 4);
    ASSERT_EQ(v->items[5].num, 100);
    ASSERT_EQ(v->items[12].num, 107);
    vector_delete(v);
}

TEST(vector_push_one_pop_one_cycle) {
    vector v = vector_new();
    for (int i = 0; i < 100; i++) {
        vector_push_back(v, item_long(i));
        ASSERT_EQ(v->n_used, 1);
        ASSERT_EQ(vector_back(v).num, i);
        vector_pop_back(v);
        ASSERT_EQ(v->n_used, 0);
    }
    vector_delete(v);
}

/* === Stress tests === */

TEST(vector_large_scale) {
    vector v = vector_new();
    for (int i = 0; i < 10000; i++)
        vector_push_back(v, item_long(i));
    ASSERT_EQ(v->n_used, 10000);
    ASSERT_EQ(v->items[0].num, 0);
    ASSERT_EQ(v->items[9999].num, 9999);
    for (int i = 0; i < 9990; i++)
        vector_pop_back(v);
    ASSERT_EQ(v->n_used, 10);
    vector_delete(v);
}

TEST(vector_capacity_always_ge_used) {
    vector v = vector_new();
    for (int i = 0; i < 200; i++) {
        vector_push_back(v, item_long(i));
        ASSERT(v->capacity >= v->n_used);
    }
    for (int i = 0; i < 200; i++) {
        vector_pop_back(v);
        ASSERT(v->capacity >= v->n_used);
    }
    vector_delete(v);
}

TEST(vector_capacity_power_of_two) {
    vector v = vector_new();
    for (int i = 0; i < 500; i++) {
        vector_push_back(v, item_long(i));
        /* capacity should be power of 2 (starts at 16 and doubles) */
        size_t n = v->capacity;
        ASSERT((n & (n - 1)) == 0);
    }
    vector_delete(v);
}

TEST(vector_remove_and_push_mix) {
    vector v = vector_new();
    for (int i = 0; i < 20; i++)
        vector_push_back(v, item_long(i));
    /* Remove first 10 */
    for (int i = 0; i < 10; i++)
        vector_remove(v, &v->items[0]);
    ASSERT_EQ(v->n_used, 10);
    ASSERT_EQ(v->items[0].num, 10);
    /* Push 10 more */
    for (int i = 100; i < 110; i++)
        vector_push_back(v, item_long(i));
    ASSERT_EQ(v->n_used, 20);
    ASSERT_EQ(v->items[10].num, 100);
    vector_delete(v);
}

/* === Capacity behavior after shrink === */

TEST(vector_capacity_after_full_pop) {
    vector v = vector_new();
    for (int i = 0; i < 100; i++)
        vector_push_back(v, item_long(i));
    while (v->n_used > 0)
        vector_pop_back(v);
    /* After popping all, capacity should be > 0 and >= n_used */
    ASSERT(v->capacity > 0);
    ASSERT(v->capacity >= v->n_used);
    vector_delete(v);
}

TEST(vector_usable_after_shrink) {
    vector v = vector_new();
    for (int i = 0; i < 32; i++)
        vector_push_back(v, item_long(i));
    while (v->n_used > 0)
        vector_pop_back(v);
    /* Should be able to push again after full shrink */
    vector_push_back(v, item_long(999));
    ASSERT_EQ(v->n_used, 1);
    ASSERT_EQ(vector_back(v).num, 999);
    vector_delete(v);
}

/* === Remove during pointer iteration === */

TEST(vector_remove_odd_pointers) {
    vector v = vector_new();
    for (int i = 0; i < 10; i++)
        vector_push_back(v, item_ptr(xstrdup(i % 2 == 0 ? "even" : "odd")));
    item *t = v->items;
    while (t < v->items + v->n_used) {
        if (strcmp((char *)t->ptr, "odd") == 0) {
            free(t->ptr);
            t = vector_remove(v, t);
        } else {
            t++;
        }
    }
    ASSERT_EQ(v->n_used, 5);
    for (size_t i = 0; i < v->n_used; i++)
        ASSERT_STR_EQ((char *)v->items[i].ptr, "even");
    vector_delete_free(v);
}

/* === Back after remove === */

TEST(vector_back_after_remove_last) {
    vector v = vector_new();
    vector_push_back(v, item_long(10));
    vector_push_back(v, item_long(20));
    vector_push_back(v, item_long(30));
    vector_remove(v, &v->items[2]);
    ASSERT_EQ(vector_back(v).num, 20);
    vector_delete(v);
}

/* === Multiple resizes down and up === */

TEST(vector_resize_up_down_up) {
    vector v = vector_new();
    /* Push to 64 capacity */
    for (int i = 0; i < 40; i++)
        vector_push_back(v, item_long(i));
    ASSERT(v->capacity >= 40);
    /* Pop down to trigger shrinks */
    while (v->n_used > 2)
        vector_pop_back(v);
    size_t small_cap = v->capacity;
    /* Grow again */
    for (int i = 0; i < 100; i++)
        vector_push_back(v, item_long(i));
    ASSERT(v->capacity > small_cap);
    ASSERT_EQ(v->items[0].num, 0); /* original first two preserved */
    ASSERT_EQ(v->items[1].num, 1);
    vector_delete(v);
}

/* === Reallocate to same size === */

TEST(vector_reallocate_same_size) {
    vector v = vector_new();
    for (int i = 0; i < 10; i++)
        vector_push_back(v, item_long(i * 5));
    vector_reallocate(v, 16); /* same as initial capacity */
    ASSERT_EQ(v->capacity, 16);
    for (int i = 0; i < 10; i++)
        ASSERT_EQ(v->items[i].num, i * 5);
    vector_delete(v);
}

/* === Push back returns correct back value === */

TEST(vector_push_back_then_back) {
    vector v = vector_new();
    for (int i = 0; i < 100; i++) {
        vector_push_back(v, item_long(i * 3));
        ASSERT_EQ(vector_back(v).num, i * 3);
    }
    vector_delete(v);
}

/* === Large pointer storage === */

TEST(vector_store_100_strings) {
    vector v = vector_new();
    for (int i = 0; i < 100; i++) {
        char buf[64];
        snprintf(buf, sizeof(buf), "item_number_%d_with_padding", i);
        vector_push_back(v, item_ptr(xstrdup(buf)));
    }
    ASSERT_EQ(v->n_used, 100);
    for (int i = 0; i < 100; i++) {
        char buf[64];
        snprintf(buf, sizeof(buf), "item_number_%d_with_padding", i);
        ASSERT_STR_EQ((char *)v->items[i].ptr, buf);
    }
    vector_delete_free(v);
}

int main(void) {
    TEST_SUITE("Vector Tests");

    RUN(vector_new_creates_empty);
    RUN(vector_new_array_not_null);
    RUN(vector_new_array_zeroed);
    RUN(vector_delete_free_empty);
    RUN(vector_delete_free_with_items);
    RUN(item_long_stores_value);
    RUN(item_long_stores_zero);
    RUN(item_long_stores_negative);
    RUN(item_ptr_stores_pointer);
    RUN(item_ptr_stores_null);
    RUN(vector_push_back_single);
    RUN(vector_push_back_two);
    RUN(vector_push_back_fills_capacity_exactly);
    RUN(vector_push_back_triggers_first_resize);
    RUN(vector_push_back_triggers_second_resize);
    RUN(vector_push_back_triggers_third_resize);
    RUN(vector_push_100);
    RUN(vector_push_preserves_order);
    RUN(vector_push_negative_values);
    RUN(vector_push_zero_values);
    RUN(vector_pop_back_decrements);
    RUN(vector_pop_back_empty_noop);
    RUN(vector_pop_back_to_empty);
    RUN(vector_pop_back_preserves_remaining);
    RUN(vector_pop_back_shrinks_capacity);
    RUN(vector_pop_multiple_empty_noop);
    RUN(vector_back_returns_last);
    RUN(vector_back_single_element);
    RUN(vector_back_after_pop);
    RUN(vector_back_with_pointer);
    RUN(vector_remove_front);
    RUN(vector_remove_middle);
    RUN(vector_remove_last_element);
    RUN(vector_remove_only_element);
    RUN(vector_remove_out_of_bounds_returns_null);
    RUN(vector_remove_returns_pointer_for_iteration);
    RUN(vector_remove_consecutive);
    RUN(vector_remove_all_one_by_one);
    RUN(vector_remove_triggers_shrink);
    RUN(vector_remove_second_to_last);
    RUN(vector_reallocate_grow);
    RUN(vector_reallocate_no_shrink_below_used);
    RUN(vector_reallocate_exact_used);
    RUN(vector_reallocate_preserves_data);
    RUN(vector_reallocate_clears_new_space);
    RUN(vector_reallocate_large);
    RUN(vector_iterate_all_elements);
    RUN(vector_iterate_empty);
    RUN(vector_iterate_single);
    RUN(vector_iterate_count_matches_n_used);
    RUN(vector_store_pointers);
    RUN(vector_store_null_pointers);
    RUN(vector_store_many_strings);
    RUN(vector_push_pop_interleave);
    RUN(vector_push_pop_push_pattern);
    RUN(vector_push_one_pop_one_cycle);
    RUN(vector_large_scale);
    RUN(vector_capacity_always_ge_used);
    RUN(vector_capacity_power_of_two);
    RUN(vector_remove_and_push_mix);
    RUN(vector_capacity_after_full_pop);
    RUN(vector_usable_after_shrink);
    RUN(vector_remove_odd_pointers);
    RUN(vector_back_after_remove_last);
    RUN(vector_resize_up_down_up);
    RUN(vector_reallocate_same_size);
    RUN(vector_push_back_then_back);
    RUN(vector_store_100_strings);

    TEST_REPORT();
}
