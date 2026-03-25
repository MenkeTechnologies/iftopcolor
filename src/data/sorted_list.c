/*
 * sorted_list.c:
 *
 */

#include <stdlib.h>
#include <string.h>
#include "../include/sorted_list.h"
#include "../include/iftop.h"

/* Comparison function wrapper for qsort */
static int (*qsort_compare_fn)(void *, void *);

static int qsort_compare_wrapper(const void *left, const void *right) {
    void *aa = *(void *const *)left;
    void *bb = *(void *const *)right;
    return qsort_compare_fn(aa, bb) ? 1 : -1;
}

void sorted_list_insert(sorted_list_type *list, void *data) {
    sorted_list_node *node, *prev;

    prev = &(list->root);

    while (prev->next != NULL && list->compare(data, prev->next->data) > 0) {
        prev = prev->next;
    }

    node = xmalloc(sizeof *node);

    node->next = prev->next;
    node->data = data;
    prev->next = node;
}

/*
 * Bulk insert: collect items into array, qsort, build list.
 * O(n log n) instead of O(n^2).
 */
void sorted_list_insert_batch(sorted_list_type *list, void **items, int count) {
    sorted_list_node *nodes, *prev;
    int i;

    if (count == 0) return;

    qsort_compare_fn = list->compare;
    qsort(items, count, sizeof(void *), qsort_compare_wrapper);

    /* Single bulk allocation for all nodes */
    nodes = xmalloc(count * sizeof(sorted_list_node));

    prev = &(list->root);
    for (i = 0; i < count; i++) {
        nodes[i].data = items[i];
        nodes[i].next = NULL;
        prev->next = &nodes[i];
        prev = &nodes[i];
    }
}


sorted_list_node *sorted_list_next_item(sorted_list_type *list, sorted_list_node *prev) {
    if (prev == NULL) {
        return list->root.next;
    } else {
        return prev->next;
    }
}

void sorted_list_destroy(sorted_list_type *list) {
    /* Batch-allocated nodes: the first node is the base of the contiguous block */
    if (list->root.next != NULL) {
        free(list->root.next);
    }
    list->root.next = NULL;
}

void sorted_list_initialise(sorted_list_type *list) {
    list->root.next = NULL;
}



