/*
 * test_sorted_list.c: tests for sorted linked list
 */

#include <stdlib.h>
#include "test.h"
#include "sorted_list.h"
#include "iftop.h"

static int int_compare(void *left, void *right) {
    long la = (long)left, lb = (long)right;
    return (la > lb) - (la < lb);
}

static int is_sorted(sorted_list_type *list) {
    sorted_list_node *prev_node = NULL;
    sorted_list_node *node = sorted_list_next_item(list, NULL);
    while (node) {
        if (prev_node && int_compare(prev_node->data, node->data) > 0)
            return 0;
        prev_node = node;
        node = sorted_list_next_item(list, node);
    }
    return 1;
}

static int list_count(sorted_list_type *list) {
    int count = 0;
    sorted_list_node *node = sorted_list_next_item(list, NULL);
    while (node) { count++; node = sorted_list_next_item(list, node); }
    return count;
}

static long list_nth(sorted_list_type *list, int index) {
    sorted_list_node *node = sorted_list_next_item(list, NULL);
    for (int i = 0; i < index && node; i++)
        node = sorted_list_next_item(list, node);
    return node ? (long)node->data : -1;
}

/* === Initialization === */

TEST(sorted_list_init_empty) {
    sorted_list_type list;
    list.compare = int_compare;
    sorted_list_initialise(&list);
    ASSERT_NULL(sorted_list_next_item(&list, NULL));
}

TEST(sorted_list_init_root_next_null) {
    sorted_list_type list;
    sorted_list_initialise(&list);
    ASSERT_NULL(list.root.next);
}

/* === Single insert === */

TEST(sorted_list_insert_one) {
    sorted_list_type list;
    list.compare = int_compare;
    sorted_list_initialise(&list);
    sorted_list_insert(&list, (void *)42L);
    sorted_list_node *node = sorted_list_next_item(&list, NULL);
    ASSERT_NOT_NULL(node);
    ASSERT_EQ((long)node->data, 42L);
    ASSERT_NULL(sorted_list_next_item(&list, node));
    sorted_list_destroy(&list);
}

TEST(sorted_list_insert_zero) {
    sorted_list_type list;
    list.compare = int_compare;
    sorted_list_initialise(&list);
    sorted_list_insert(&list, (void *)0L);
    ASSERT_EQ(list_count(&list), 1);
    ASSERT_EQ(list_nth(&list, 0), 0L);
    sorted_list_destroy(&list);
}

TEST(sorted_list_insert_negative) {
    sorted_list_type list;
    list.compare = int_compare;
    sorted_list_initialise(&list);
    sorted_list_insert(&list, (void *)-10L);
    sorted_list_insert(&list, (void *)-5L);
    sorted_list_insert(&list, (void *)-20L);
    ASSERT(is_sorted(&list));
    ASSERT_EQ(list_count(&list), 3);
    ASSERT_EQ(list_nth(&list, 0), -20L);
    ASSERT_EQ(list_nth(&list, 1), -10L);
    ASSERT_EQ(list_nth(&list, 2), -5L);
    sorted_list_destroy(&list);
}

TEST(sorted_list_insert_two_ascending) {
    sorted_list_type list;
    list.compare = int_compare;
    sorted_list_initialise(&list);
    sorted_list_insert(&list, (void *)1L);
    sorted_list_insert(&list, (void *)2L);
    ASSERT(is_sorted(&list));
    ASSERT_EQ(list_nth(&list, 0), 1L);
    ASSERT_EQ(list_nth(&list, 1), 2L);
    sorted_list_destroy(&list);
}

TEST(sorted_list_insert_two_descending) {
    sorted_list_type list;
    list.compare = int_compare;
    sorted_list_initialise(&list);
    sorted_list_insert(&list, (void *)2L);
    sorted_list_insert(&list, (void *)1L);
    ASSERT(is_sorted(&list));
    ASSERT_EQ(list_nth(&list, 0), 1L);
    ASSERT_EQ(list_nth(&list, 1), 2L);
    sorted_list_destroy(&list);
}

TEST(sorted_list_insert_maintains_order) {
    sorted_list_type list;
    list.compare = int_compare;
    sorted_list_initialise(&list);
    sorted_list_insert(&list, (void *)30L);
    sorted_list_insert(&list, (void *)10L);
    sorted_list_insert(&list, (void *)20L);
    sorted_list_insert(&list, (void *)50L);
    sorted_list_insert(&list, (void *)40L);
    ASSERT(is_sorted(&list));
    ASSERT_EQ(list_count(&list), 5);
    ASSERT_EQ(list_nth(&list, 0), 10L);
    ASSERT_EQ(list_nth(&list, 1), 20L);
    ASSERT_EQ(list_nth(&list, 2), 30L);
    ASSERT_EQ(list_nth(&list, 3), 40L);
    ASSERT_EQ(list_nth(&list, 4), 50L);
    sorted_list_destroy(&list);
}

TEST(sorted_list_insert_at_head) {
    sorted_list_type list;
    list.compare = int_compare;
    sorted_list_initialise(&list);
    sorted_list_insert(&list, (void *)50L);
    sorted_list_insert(&list, (void *)40L);
    sorted_list_insert(&list, (void *)30L);
    sorted_list_insert(&list, (void *)1L);
    ASSERT_EQ(list_nth(&list, 0), 1L);
    sorted_list_destroy(&list);
}

TEST(sorted_list_insert_at_tail) {
    sorted_list_type list;
    list.compare = int_compare;
    sorted_list_initialise(&list);
    sorted_list_insert(&list, (void *)1L);
    sorted_list_insert(&list, (void *)2L);
    sorted_list_insert(&list, (void *)3L);
    sorted_list_insert(&list, (void *)999L);
    ASSERT_EQ(list_nth(&list, 3), 999L);
    sorted_list_destroy(&list);
}

TEST(sorted_list_insert_duplicates) {
    sorted_list_type list;
    list.compare = int_compare;
    sorted_list_initialise(&list);
    sorted_list_insert(&list, (void *)5L);
    sorted_list_insert(&list, (void *)5L);
    sorted_list_insert(&list, (void *)5L);
    ASSERT_EQ(list_count(&list), 3);
    ASSERT(is_sorted(&list));
    sorted_list_destroy(&list);
}

TEST(sorted_list_insert_many_duplicates) {
    sorted_list_type list;
    list.compare = int_compare;
    sorted_list_initialise(&list);
    for (int i = 0; i < 20; i++)
        sorted_list_insert(&list, (void *)7L);
    ASSERT_EQ(list_count(&list), 20);
    ASSERT(is_sorted(&list));
    sorted_list_destroy(&list);
}

TEST(sorted_list_insert_already_sorted) {
    sorted_list_type list;
    list.compare = int_compare;
    sorted_list_initialise(&list);
    for (long i = 1; i <= 10; i++)
        sorted_list_insert(&list, (void *)i);
    ASSERT(is_sorted(&list));
    ASSERT_EQ(list_count(&list), 10);
    sorted_list_destroy(&list);
}

TEST(sorted_list_insert_reverse_sorted) {
    sorted_list_type list;
    list.compare = int_compare;
    sorted_list_initialise(&list);
    for (long i = 10; i >= 1; i--)
        sorted_list_insert(&list, (void *)i);
    ASSERT(is_sorted(&list));
    ASSERT_EQ(list_count(&list), 10);
    for (long i = 0; i < 10; i++)
        ASSERT_EQ(list_nth(&list, i), i + 1);
    sorted_list_destroy(&list);
}

TEST(sorted_list_insert_alternating) {
    sorted_list_type list;
    list.compare = int_compare;
    sorted_list_initialise(&list);
    sorted_list_insert(&list, (void *)1L);
    sorted_list_insert(&list, (void *)100L);
    sorted_list_insert(&list, (void *)2L);
    sorted_list_insert(&list, (void *)99L);
    sorted_list_insert(&list, (void *)3L);
    ASSERT(is_sorted(&list));
    ASSERT_EQ(list_count(&list), 5);
    sorted_list_destroy(&list);
}

TEST(sorted_list_insert_large_values) {
    sorted_list_type list;
    list.compare = int_compare;
    sorted_list_initialise(&list);
    sorted_list_insert(&list, (void *)1000000L);
    sorted_list_insert(&list, (void *)999999L);
    sorted_list_insert(&list, (void *)1000001L);
    ASSERT(is_sorted(&list));
    ASSERT_EQ(list_nth(&list, 0), 999999L);
    sorted_list_destroy(&list);
}

TEST(sorted_list_insert_mixed_sign) {
    sorted_list_type list;
    list.compare = int_compare;
    sorted_list_initialise(&list);
    sorted_list_insert(&list, (void *)5L);
    sorted_list_insert(&list, (void *)-5L);
    sorted_list_insert(&list, (void *)0L);
    ASSERT(is_sorted(&list));
    ASSERT_EQ(list_nth(&list, 0), -5L);
    ASSERT_EQ(list_nth(&list, 1), 0L);
    ASSERT_EQ(list_nth(&list, 2), 5L);
    sorted_list_destroy(&list);
}

/* === Batch insert === */

TEST(sorted_list_batch_insert) {
    sorted_list_type list;
    list.compare = int_compare;
    sorted_list_initialise(&list);
    void *items[] = {(void *)50L, (void *)10L, (void *)30L, (void *)20L, (void *)40L};
    sorted_list_insert_batch(&list, items, 5);
    ASSERT_EQ(list_count(&list), 5);
    sorted_list_destroy(&list);
}

TEST(sorted_list_batch_insert_empty) {
    sorted_list_type list;
    list.compare = int_compare;
    sorted_list_initialise(&list);
    sorted_list_insert_batch(&list, NULL, 0);
    ASSERT_NULL(sorted_list_next_item(&list, NULL));
}

TEST(sorted_list_batch_insert_single) {
    sorted_list_type list;
    list.compare = int_compare;
    sorted_list_initialise(&list);
    void *items[] = {(void *)99L};
    sorted_list_insert_batch(&list, items, 1);
    ASSERT_EQ(list_count(&list), 1);
    ASSERT_EQ(list_nth(&list, 0), 99L);
    sorted_list_destroy(&list);
}

TEST(sorted_list_batch_insert_two) {
    sorted_list_type list;
    list.compare = int_compare;
    sorted_list_initialise(&list);
    void *items[] = {(void *)20L, (void *)10L};
    sorted_list_insert_batch(&list, items, 2);
    ASSERT_EQ(list_count(&list), 2);
    sorted_list_destroy(&list);
}

TEST(sorted_list_batch_large) {
    sorted_list_type list;
    list.compare = int_compare;
    sorted_list_initialise(&list);
    int count = 1000;
    void **items = malloc(count * sizeof(void *));
    for (int i = 0; i < count; i++)
        items[i] = (void *)(long)(count - i);
    sorted_list_insert_batch(&list, items, count);
    ASSERT_EQ(list_count(&list), count);
    free(items);
    sorted_list_destroy(&list);
}

TEST(sorted_list_batch_already_sorted) {
    sorted_list_type list;
    list.compare = int_compare;
    sorted_list_initialise(&list);
    void *items[] = {(void *)1L, (void *)2L, (void *)3L, (void *)4L, (void *)5L};
    sorted_list_insert_batch(&list, items, 5);
    ASSERT_EQ(list_count(&list), 5);
    sorted_list_destroy(&list);
}

/* === Iteration === */

TEST(sorted_list_next_item_null_starts_at_head) {
    sorted_list_type list;
    list.compare = int_compare;
    sorted_list_initialise(&list);
    sorted_list_insert(&list, (void *)1L);
    sorted_list_insert(&list, (void *)2L);
    sorted_list_node *first = sorted_list_next_item(&list, NULL);
    ASSERT_NOT_NULL(first);
    ASSERT_EQ((long)first->data, 1L);
    sorted_list_destroy(&list);
}

TEST(sorted_list_iterate_complete) {
    sorted_list_type list;
    list.compare = int_compare;
    sorted_list_initialise(&list);
    for (long i = 1; i <= 5; i++)
        sorted_list_insert(&list, (void *)i);
    long sum = 0;
    sorted_list_node *node = NULL;
    while ((node = sorted_list_next_item(&list, node)) != NULL)
        sum += (long)node->data;
    ASSERT_EQ(sum, 15L);
    sorted_list_destroy(&list);
}

TEST(sorted_list_iterate_empty) {
    sorted_list_type list;
    list.compare = int_compare;
    sorted_list_initialise(&list);
    ASSERT_NULL(sorted_list_next_item(&list, NULL));
}

TEST(sorted_list_iterate_terminates) {
    sorted_list_type list;
    list.compare = int_compare;
    sorted_list_initialise(&list);
    sorted_list_insert(&list, (void *)1L);
    sorted_list_node *node = sorted_list_next_item(&list, NULL);
    ASSERT_NOT_NULL(node);
    node = sorted_list_next_item(&list, node);
    ASSERT_NULL(node);
    sorted_list_destroy(&list);
}

/* === Destroy === */

TEST(sorted_list_destroy_clears) {
    sorted_list_type list;
    list.compare = int_compare;
    sorted_list_initialise(&list);
    void *items[] = {(void *)1L, (void *)2L, (void *)3L};
    sorted_list_insert_batch(&list, items, 3);
    sorted_list_destroy(&list);
    ASSERT_NULL(sorted_list_next_item(&list, NULL));
}

TEST(sorted_list_destroy_empty_safe) {
    sorted_list_type list;
    list.compare = int_compare;
    sorted_list_initialise(&list);
    sorted_list_destroy(&list);
    ASSERT_NULL(sorted_list_next_item(&list, NULL));
}

TEST(sorted_list_destroy_then_reinit) {
    sorted_list_type list;
    list.compare = int_compare;
    sorted_list_initialise(&list);
    void *items[] = {(void *)1L, (void *)2L};
    sorted_list_insert_batch(&list, items, 2);
    sorted_list_destroy(&list);
    sorted_list_initialise(&list);
    ASSERT_NULL(sorted_list_next_item(&list, NULL));
    sorted_list_insert(&list, (void *)99L);
    ASSERT_EQ(list_count(&list), 1);
    sorted_list_destroy(&list);
}

TEST(sorted_list_double_destroy_safe) {
    sorted_list_type list;
    list.compare = int_compare;
    sorted_list_initialise(&list);
    sorted_list_destroy(&list);
    sorted_list_destroy(&list); /* should not crash */
}

/* === Batch insert ordering === */

TEST(sorted_list_batch_preserves_count) {
    sorted_list_type list;
    list.compare = int_compare;
    sorted_list_initialise(&list);
    void *items[] = {(void *)50L, (void *)10L, (void *)30L, (void *)20L, (void *)40L};
    sorted_list_insert_batch(&list, items, 5);
    ASSERT_EQ(list_count(&list), 5);
    sorted_list_destroy(&list);
}

TEST(sorted_list_batch_with_duplicates) {
    sorted_list_type list;
    list.compare = int_compare;
    sorted_list_initialise(&list);
    void *items[] = {(void *)5L, (void *)3L, (void *)5L, (void *)1L, (void *)3L};
    sorted_list_insert_batch(&list, items, 5);
    ASSERT_EQ(list_count(&list), 5);
    sorted_list_destroy(&list);
}

TEST(sorted_list_batch_with_negatives) {
    sorted_list_type list;
    list.compare = int_compare;
    sorted_list_initialise(&list);
    void *items[] = {(void *)5L, (void *)-5L, (void *)0L, (void *)-10L, (void *)10L};
    sorted_list_insert_batch(&list, items, 5);
    ASSERT_EQ(list_count(&list), 5);
    sorted_list_destroy(&list);
}

TEST(sorted_list_batch_reverse_100) {
    sorted_list_type list;
    list.compare = int_compare;
    sorted_list_initialise(&list);
    void *items[100];
    for (int i = 0; i < 100; i++)
        items[i] = (void *)(long)(100 - i);
    sorted_list_insert_batch(&list, items, 100);
    ASSERT_EQ(list_count(&list), 100);
    sorted_list_destroy(&list);
}

TEST(sorted_list_batch_all_same) {
    sorted_list_type list;
    list.compare = int_compare;
    sorted_list_initialise(&list);
    void *items[50];
    for (int i = 0; i < 50; i++)
        items[i] = (void *)42L;
    sorted_list_insert_batch(&list, items, 50);
    ASSERT_EQ(list_count(&list), 50);
    sorted_list_destroy(&list);
}

TEST(sorted_list_batch_iterable) {
    /* Verify all batch items are reachable via iteration */
    sorted_list_type list;
    list.compare = int_compare;
    sorted_list_initialise(&list);
    void *items[] = {(void *)7L, (void *)2L, (void *)9L, (void *)4L,
                     (void *)1L, (void *)8L, (void *)3L, (void *)6L};
    sorted_list_insert_batch(&list, items, 8);
    ASSERT_EQ(list_count(&list), 8);
    /* Verify sum is correct (all items present) */
    long sum = 0;
    sorted_list_node *node = NULL;
    while ((node = sorted_list_next_item(&list, node)) != NULL)
        sum += (long)node->data;
    ASSERT_EQ(sum, 1+2+3+4+6+7+8+9);
    sorted_list_destroy(&list);
}

/* === Scale tests === */

TEST(sorted_list_insert_5000) {
    sorted_list_type list;
    list.compare = int_compare;
    sorted_list_initialise(&list);
    /* Insert in a pattern that exercises both head and tail insertion */
    for (long i = 0; i < 5000; i++) {
        long val = (i * 2654435761L) % 100000; /* Knuth multiplicative hash for spread */
        sorted_list_insert(&list, (void *)val);
    }
    ASSERT_EQ(list_count(&list), 5000);
    ASSERT(is_sorted(&list));
    sorted_list_destroy(&list);
}

TEST(sorted_list_batch_5000) {
    sorted_list_type list;
    list.compare = int_compare;
    sorted_list_initialise(&list);
    void **items = malloc(5000 * sizeof(void *));
    for (int i = 0; i < 5000; i++)
        items[i] = (void *)(long)((i * 2654435761L) % 100000);
    sorted_list_insert_batch(&list, items, 5000);
    ASSERT_EQ(list_count(&list), 5000);
    free(items);
    sorted_list_destroy(&list);
}

int main(void) {
    TEST_SUITE("Sorted List Tests");

    RUN(sorted_list_init_empty);
    RUN(sorted_list_init_root_next_null);
    RUN(sorted_list_insert_one);
    RUN(sorted_list_insert_zero);
    RUN(sorted_list_insert_negative);
    RUN(sorted_list_insert_two_ascending);
    RUN(sorted_list_insert_two_descending);
    RUN(sorted_list_insert_maintains_order);
    RUN(sorted_list_insert_at_head);
    RUN(sorted_list_insert_at_tail);
    RUN(sorted_list_insert_duplicates);
    RUN(sorted_list_insert_many_duplicates);
    RUN(sorted_list_insert_already_sorted);
    RUN(sorted_list_insert_reverse_sorted);
    RUN(sorted_list_insert_alternating);
    RUN(sorted_list_insert_large_values);
    RUN(sorted_list_insert_mixed_sign);
    RUN(sorted_list_batch_insert);
    RUN(sorted_list_batch_insert_empty);
    RUN(sorted_list_batch_insert_single);
    RUN(sorted_list_batch_insert_two);
    RUN(sorted_list_batch_large);
    RUN(sorted_list_batch_already_sorted);
    RUN(sorted_list_next_item_null_starts_at_head);
    RUN(sorted_list_iterate_complete);
    RUN(sorted_list_iterate_empty);
    RUN(sorted_list_iterate_terminates);
    RUN(sorted_list_destroy_clears);
    RUN(sorted_list_destroy_empty_safe);
    RUN(sorted_list_destroy_then_reinit);
    RUN(sorted_list_double_destroy_safe);
    RUN(sorted_list_batch_preserves_count);
    RUN(sorted_list_batch_with_duplicates);
    RUN(sorted_list_batch_with_negatives);
    RUN(sorted_list_batch_reverse_100);
    RUN(sorted_list_batch_all_same);
    RUN(sorted_list_batch_iterable);
    RUN(sorted_list_insert_5000);
    RUN(sorted_list_batch_5000);

    TEST_REPORT();
}
