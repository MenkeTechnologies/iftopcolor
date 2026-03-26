/*
 * test_hash.c: tests for generic hash table
 */

#include <stdlib.h>
#include <string.h>
#include "test.h"
#include "hash.h"
#include "iftop.h"

/* --- Helper: string-keyed hash --- */

static int str_compare(void *left, void *right) {
    return strcmp((char *)left, (char *)right) == 0;
}

static int str_hash(void *key) {
    char *s = (char *)key;
    unsigned int h = 0;
    while (*s) {
        h = h * 31 + (unsigned char)*s++;
    }
    return h % 64;
}

static void *str_copy_key(void *key) {
    return xstrdup((char *)key);
}

static void str_delete_key(void *key) {
    free(key);
}

static hash_type *create_str_hash(void) {
    hash_type *h = xcalloc(1, sizeof(hash_type));
    h->size = 64;
    h->compare = str_compare;
    h->hash = str_hash;
    h->copy_key = str_copy_key;
    h->delete_key = str_delete_key;
    hash_initialise(h);
    return h;
}

static void destroy_str_hash(hash_type *h) {
    hash_delete_all(h);
    hash_destroy(h);
    free(h);
}

/* --- Helper: int-keyed hash --- */

static int int_compare(void *left, void *right) {
    return *(int *)left == *(int *)right;
}

static int int_hash(void *key) {
    return (*(int *)key) % 32;
}

static void *int_copy_key(void *key) {
    int *copy = xmalloc(sizeof(int));
    *copy = *(int *)key;
    return copy;
}

static void int_delete_key(void *key) {
    free(key);
}

static hash_type *create_int_hash(void) {
    hash_type *h = xcalloc(1, sizeof(hash_type));
    h->size = 32;
    h->compare = int_compare;
    h->hash = int_hash;
    h->copy_key = int_copy_key;
    h->delete_key = int_delete_key;
    hash_initialise(h);
    return h;
}

/* Force all to bucket 0 */
static int always_zero_hash(void *key) {
    (void)key;
    return 0;
}

/* === Initialise / Destroy === */

TEST(hash_initialise_returns_ok) {
    hash_type h;
    h.size = 16;
    ASSERT_EQ(hash_initialise(&h), HASH_STATUS_OK);
    ASSERT_NOT_NULL(h.table);
    hash_destroy(&h);
}

TEST(hash_initialise_table_zeroed) {
    hash_type h;
    h.size = 16;
    hash_initialise(&h);
    for (int i = 0; i < 16; i++) {
        ASSERT_NULL(h.table[i]);
    }
    hash_destroy(&h);
}

TEST(hash_destroy_returns_ok) {
    hash_type h;
    h.size = 8;
    hash_initialise(&h);
    ASSERT_EQ(hash_destroy(&h), HASH_STATUS_OK);
}

/* === Insert + Find === */

TEST(hash_insert_and_find) {
    hash_type *h = create_str_hash();
    int val = 42;
    hash_insert(h, "key1", &val);
    void *rec = NULL;
    ASSERT_EQ(hash_find(h, "key1", &rec), HASH_STATUS_OK);
    ASSERT_EQ(*(int *)rec, 42);
    destroy_str_hash(h);
}

TEST(hash_insert_returns_ok) {
    hash_type *h = create_str_hash();
    int val = 1;
    ASSERT_EQ(hash_insert(h, "k", &val), HASH_STATUS_OK);
    destroy_str_hash(h);
}

TEST(hash_find_missing_key) {
    hash_type *h = create_str_hash();
    void *rec = NULL;
    ASSERT_EQ(hash_find(h, "nonexistent", &rec), HASH_STATUS_KEY_NOT_FOUND);
    destroy_str_hash(h);
}

TEST(hash_find_empty_table) {
    hash_type *h = create_str_hash();
    void *rec = NULL;
    ASSERT_EQ(hash_find(h, "anything", &rec), HASH_STATUS_KEY_NOT_FOUND);
    destroy_str_hash(h);
}

TEST(hash_insert_multiple) {
    hash_type *h = create_str_hash();
    int v1 = 10, v2 = 20, v3 = 30;
    hash_insert(h, "alpha", &v1);
    hash_insert(h, "beta", &v2);
    hash_insert(h, "gamma", &v3);
    void *rec = NULL;
    ASSERT_EQ(hash_find(h, "alpha", &rec), HASH_STATUS_OK);
    ASSERT_EQ(*(int *)rec, 10);
    ASSERT_EQ(hash_find(h, "beta", &rec), HASH_STATUS_OK);
    ASSERT_EQ(*(int *)rec, 20);
    ASSERT_EQ(hash_find(h, "gamma", &rec), HASH_STATUS_OK);
    ASSERT_EQ(*(int *)rec, 30);
    destroy_str_hash(h);
}

TEST(hash_insert_single_char_keys) {
    hash_type *h = create_str_hash();
    int vals[26];
    for (int i = 0; i < 26; i++) {
        vals[i] = i;
        char key[2] = {'a' + i, '\0'};
        hash_insert(h, key, &vals[i]);
    }
    for (int i = 0; i < 26; i++) {
        char key[2] = {'a' + i, '\0'};
        void *rec = NULL;
        ASSERT_EQ(hash_find(h, key, &rec), HASH_STATUS_OK);
        ASSERT_EQ(*(int *)rec, i);
    }
    destroy_str_hash(h);
}

TEST(hash_insert_long_keys) {
    hash_type *h = create_str_hash();
    char long_key[256];
    memset(long_key, 'x', 255);
    long_key[255] = '\0';
    int val = 999;
    hash_insert(h, long_key, &val);
    void *rec = NULL;
    ASSERT_EQ(hash_find(h, long_key, &rec), HASH_STATUS_OK);
    ASSERT_EQ(*(int *)rec, 999);
    destroy_str_hash(h);
}

TEST(hash_insert_empty_string_key) {
    hash_type *h = create_str_hash();
    int val = 77;
    hash_insert(h, "", &val);
    void *rec = NULL;
    ASSERT_EQ(hash_find(h, "", &rec), HASH_STATUS_OK);
    ASSERT_EQ(*(int *)rec, 77);
    destroy_str_hash(h);
}

TEST(hash_overwrite_same_key) {
    hash_type *h = create_str_hash();
    int v1 = 100, v2 = 200;
    hash_insert(h, "dup", &v1);
    hash_insert(h, "dup", &v2);
    void *rec = NULL;
    ASSERT_EQ(hash_find(h, "dup", &rec), HASH_STATUS_OK);
    ASSERT_EQ(*(int *)rec, 200); /* most recent insert is at head */
    destroy_str_hash(h);
}

TEST(hash_insert_null_rec) {
    hash_type *h = create_str_hash();
    hash_insert(h, "null_rec", NULL);
    void *rec = (void *)1;
    ASSERT_EQ(hash_find(h, "null_rec", &rec), HASH_STATUS_OK);
    ASSERT_NULL(rec);
    destroy_str_hash(h);
}

TEST(hash_int_key_insert_find) {
    hash_type *h = create_int_hash();
    int key = 42, val = 100;
    hash_insert(h, &key, &val);
    void *rec = NULL;
    int lookup = 42;
    ASSERT_EQ(hash_find(h, &lookup, &rec), HASH_STATUS_OK);
    ASSERT_EQ(*(int *)rec, 100);
    hash_delete_all(h);
    hash_destroy(h);
    free(h);
}

TEST(hash_int_key_miss) {
    hash_type *h = create_int_hash();
    int key = 42, val = 100;
    hash_insert(h, &key, &val);
    void *rec = NULL;
    int lookup = 43;
    ASSERT_EQ(hash_find(h, &lookup, &rec), HASH_STATUS_KEY_NOT_FOUND);
    hash_delete_all(h);
    hash_destroy(h);
    free(h);
}

/* === Delete === */

TEST(hash_delete_existing) {
    hash_type *h = create_str_hash();
    int val = 99;
    hash_insert(h, "delete_me", &val);
    ASSERT_EQ(hash_delete(h, "delete_me"), HASH_STATUS_OK);
    void *rec = NULL;
    ASSERT_EQ(hash_find(h, "delete_me", &rec), HASH_STATUS_KEY_NOT_FOUND);
    destroy_str_hash(h);
}

TEST(hash_delete_missing) {
    hash_type *h = create_str_hash();
    ASSERT_EQ(hash_delete(h, "nope"), HASH_STATUS_KEY_NOT_FOUND);
    destroy_str_hash(h);
}

TEST(hash_delete_from_empty) {
    hash_type *h = create_str_hash();
    ASSERT_EQ(hash_delete(h, "anything"), HASH_STATUS_KEY_NOT_FOUND);
    destroy_str_hash(h);
}

TEST(hash_delete_only_entry) {
    hash_type *h = create_str_hash();
    int val = 1;
    hash_insert(h, "only", &val);
    ASSERT_EQ(hash_delete(h, "only"), HASH_STATUS_OK);
    void *rec = NULL;
    ASSERT_EQ(hash_find(h, "only", &rec), HASH_STATUS_KEY_NOT_FOUND);
    destroy_str_hash(h);
}

TEST(hash_delete_first_preserves_rest) {
    hash_type *h = create_str_hash();
    int v1 = 1, v2 = 2, v3 = 3;
    hash_insert(h, "a", &v1);
    hash_insert(h, "b", &v2);
    hash_insert(h, "c", &v3);
    hash_delete(h, "a");
    void *rec = NULL;
    ASSERT_EQ(hash_find(h, "b", &rec), HASH_STATUS_OK);
    ASSERT_EQ(hash_find(h, "c", &rec), HASH_STATUS_OK);
    ASSERT_EQ(hash_find(h, "a", &rec), HASH_STATUS_KEY_NOT_FOUND);
    destroy_str_hash(h);
}

TEST(hash_delete_then_reinsert) {
    hash_type *h = create_str_hash();
    int v1 = 1, v2 = 2;
    hash_insert(h, "key", &v1);
    hash_delete(h, "key");
    hash_insert(h, "key", &v2);
    void *rec = NULL;
    ASSERT_EQ(hash_find(h, "key", &rec), HASH_STATUS_OK);
    ASSERT_EQ(*(int *)rec, 2);
    destroy_str_hash(h);
}

TEST(hash_delete_all_entries) {
    hash_type *h = create_str_hash();
    int vals[10];
    for (int i = 0; i < 10; i++) {
        vals[i] = i;
        char key[16];
        snprintf(key, sizeof(key), "k%d", i);
        hash_insert(h, key, &vals[i]);
    }
    for (int i = 0; i < 10; i++) {
        char key[16];
        snprintf(key, sizeof(key), "k%d", i);
        ASSERT_EQ(hash_delete(h, key), HASH_STATUS_OK);
    }
    hash_node_type *node = NULL;
    ASSERT_EQ(hash_next_item(h, &node), HASH_STATUS_KEY_NOT_FOUND);
    destroy_str_hash(h);
}

/* === Delete node === */

TEST(hash_delete_node_by_pointer) {
    hash_type *h = create_str_hash();
    int v1 = 1, v2 = 2, v3 = 3;
    hash_insert(h, "a", &v1);
    hash_insert(h, "b", &v2);
    hash_insert(h, "c", &v3);
    hash_node_type *node = NULL;
    hash_node_type *target = NULL;
    hash_next_item(h, &node);
    while (node) {
        if (str_compare(node->key, "b")) {
            target = node;
            break;
        }
        hash_next_item(h, &node);
    }
    ASSERT_NOT_NULL(target);
    ASSERT_EQ(hash_delete_node(h, target), HASH_STATUS_OK);
    void *rec = NULL;
    ASSERT_EQ(hash_find(h, "b", &rec), HASH_STATUS_KEY_NOT_FOUND);
    ASSERT_EQ(hash_find(h, "a", &rec), HASH_STATUS_OK);
    ASSERT_EQ(hash_find(h, "c", &rec), HASH_STATUS_OK);
    destroy_str_hash(h);
}

TEST(hash_delete_node_first_in_bucket) {
    /* Force all into bucket 0, delete head */
    hash_type *h = xcalloc(1, sizeof(hash_type));
    h->size = 4;
    h->compare = str_compare;
    h->hash = always_zero_hash;
    h->copy_key = str_copy_key;
    h->delete_key = str_delete_key;
    hash_initialise(h);
    int v1 = 1, v2 = 2;
    hash_insert(h, "first", &v1);
    hash_insert(h, "second", &v2);
    /* "second" is at head of chain (most recently inserted) */
    hash_node_type *head = h->table[0];
    ASSERT_EQ(hash_delete_node(h, head), HASH_STATUS_OK);
    void *rec = NULL;
    ASSERT_EQ(hash_find(h, "first", &rec), HASH_STATUS_OK);
    hash_delete_all(h);
    hash_destroy(h);
    free(h);
}

TEST(hash_delete_node_last_in_bucket) {
    hash_type *h = xcalloc(1, sizeof(hash_type));
    h->size = 4;
    h->compare = str_compare;
    h->hash = always_zero_hash;
    h->copy_key = str_copy_key;
    h->delete_key = str_delete_key;
    hash_initialise(h);
    int v1 = 1, v2 = 2;
    hash_insert(h, "first", &v1);
    hash_insert(h, "second", &v2);
    /* "first" is tail */
    hash_node_type *tail = h->table[0]->next;
    ASSERT_EQ(hash_delete_node(h, tail), HASH_STATUS_OK);
    void *rec = NULL;
    ASSERT_EQ(hash_find(h, "second", &rec), HASH_STATUS_OK);
    ASSERT_EQ(hash_find(h, "first", &rec), HASH_STATUS_KEY_NOT_FOUND);
    hash_delete_all(h);
    hash_destroy(h);
    free(h);
}

/* === Collision handling === */

TEST(hash_collision_chain_all_findable) {
    hash_type *h = xcalloc(1, sizeof(hash_type));
    h->size = 4;
    h->compare = str_compare;
    h->hash = always_zero_hash;
    h->copy_key = str_copy_key;
    h->delete_key = str_delete_key;
    hash_initialise(h);
    int v1 = 1, v2 = 2, v3 = 3;
    hash_insert(h, "x", &v1);
    hash_insert(h, "y", &v2);
    hash_insert(h, "z", &v3);
    void *rec = NULL;
    ASSERT_EQ(hash_find(h, "x", &rec), HASH_STATUS_OK);
    ASSERT_EQ(*(int *)rec, 1);
    ASSERT_EQ(hash_find(h, "y", &rec), HASH_STATUS_OK);
    ASSERT_EQ(*(int *)rec, 2);
    ASSERT_EQ(hash_find(h, "z", &rec), HASH_STATUS_OK);
    ASSERT_EQ(*(int *)rec, 3);
    hash_delete_all(h);
    hash_destroy(h);
    free(h);
}

TEST(hash_collision_delete_middle) {
    hash_type *h = xcalloc(1, sizeof(hash_type));
    h->size = 4;
    h->compare = str_compare;
    h->hash = always_zero_hash;
    h->copy_key = str_copy_key;
    h->delete_key = str_delete_key;
    hash_initialise(h);
    int v1 = 1, v2 = 2, v3 = 3;
    hash_insert(h, "x", &v1);
    hash_insert(h, "y", &v2);
    hash_insert(h, "z", &v3);
    hash_delete(h, "y");
    void *rec = NULL;
    ASSERT_EQ(hash_find(h, "y", &rec), HASH_STATUS_KEY_NOT_FOUND);
    ASSERT_EQ(hash_find(h, "x", &rec), HASH_STATUS_OK);
    ASSERT_EQ(hash_find(h, "z", &rec), HASH_STATUS_OK);
    hash_delete_all(h);
    hash_destroy(h);
    free(h);
}

TEST(hash_collision_delete_head) {
    hash_type *h = xcalloc(1, sizeof(hash_type));
    h->size = 4;
    h->compare = str_compare;
    h->hash = always_zero_hash;
    h->copy_key = str_copy_key;
    h->delete_key = str_delete_key;
    hash_initialise(h);
    int v1 = 1, v2 = 2;
    hash_insert(h, "a", &v1);
    hash_insert(h, "b", &v2);
    hash_delete(h, "b"); /* b is head */
    void *rec = NULL;
    ASSERT_EQ(hash_find(h, "a", &rec), HASH_STATUS_OK);
    ASSERT_EQ(hash_find(h, "b", &rec), HASH_STATUS_KEY_NOT_FOUND);
    hash_delete_all(h);
    hash_destroy(h);
    free(h);
}

TEST(hash_collision_delete_tail) {
    hash_type *h = xcalloc(1, sizeof(hash_type));
    h->size = 4;
    h->compare = str_compare;
    h->hash = always_zero_hash;
    h->copy_key = str_copy_key;
    h->delete_key = str_delete_key;
    hash_initialise(h);
    int v1 = 1, v2 = 2;
    hash_insert(h, "a", &v1);
    hash_insert(h, "b", &v2);
    hash_delete(h, "a"); /* a is tail */
    void *rec = NULL;
    ASSERT_EQ(hash_find(h, "b", &rec), HASH_STATUS_OK);
    ASSERT_EQ(hash_find(h, "a", &rec), HASH_STATUS_KEY_NOT_FOUND);
    hash_delete_all(h);
    hash_destroy(h);
    free(h);
}

TEST(hash_collision_long_chain) {
    hash_type *h = xcalloc(1, sizeof(hash_type));
    h->size = 4;
    h->compare = str_compare;
    h->hash = always_zero_hash;
    h->copy_key = str_copy_key;
    h->delete_key = str_delete_key;
    hash_initialise(h);
    int vals[20];
    for (int i = 0; i < 20; i++) {
        vals[i] = i;
        char key[8];
        snprintf(key, sizeof(key), "k%d", i);
        hash_insert(h, key, &vals[i]);
    }
    for (int i = 0; i < 20; i++) {
        char key[8];
        snprintf(key, sizeof(key), "k%d", i);
        void *rec = NULL;
        ASSERT_EQ(hash_find(h, key, &rec), HASH_STATUS_OK);
        ASSERT_EQ(*(int *)rec, i);
    }
    hash_delete_all(h);
    hash_destroy(h);
    free(h);
}

/* === Iteration === */

TEST(hash_next_item_iterates_all) {
    hash_type *h = create_str_hash();
    int vals[5] = {0, 1, 2, 3, 4};
    char keys[][8] = {"k0", "k1", "k2", "k3", "k4"};
    for (int i = 0; i < 5; i++) {
        hash_insert(h, keys[i], &vals[i]);
    }
    int count = 0;
    hash_node_type *node = NULL;
    while (hash_next_item(h, &node) == HASH_STATUS_OK) {
        count++;
    }
    ASSERT_EQ(count, 5);
    destroy_str_hash(h);
}

TEST(hash_next_item_empty) {
    hash_type *h = create_str_hash();
    hash_node_type *node = NULL;
    ASSERT_EQ(hash_next_item(h, &node), HASH_STATUS_KEY_NOT_FOUND);
    ASSERT_NULL(node);
    destroy_str_hash(h);
}

TEST(hash_next_item_single) {
    hash_type *h = create_str_hash();
    int val = 1;
    hash_insert(h, "only", &val);
    hash_node_type *node = NULL;
    ASSERT_EQ(hash_next_item(h, &node), HASH_STATUS_OK);
    ASSERT_NOT_NULL(node);
    ASSERT_EQ(*(int *)node->record, 1);
    ASSERT_EQ(hash_next_item(h, &node), HASH_STATUS_KEY_NOT_FOUND);
    destroy_str_hash(h);
}

TEST(hash_next_item_traverses_chain) {
    hash_type *h = xcalloc(1, sizeof(hash_type));
    h->size = 4;
    h->compare = str_compare;
    h->hash = always_zero_hash;
    h->copy_key = str_copy_key;
    h->delete_key = str_delete_key;
    hash_initialise(h);
    int v1 = 1, v2 = 2, v3 = 3;
    hash_insert(h, "a", &v1);
    hash_insert(h, "b", &v2);
    hash_insert(h, "c", &v3);
    int count = 0;
    hash_node_type *node = NULL;
    while (hash_next_item(h, &node) == HASH_STATUS_OK) {
        count++;
    }
    ASSERT_EQ(count, 3);
    hash_delete_all(h);
    hash_destroy(h);
    free(h);
}

TEST(hash_iteration_visits_all_buckets) {
    hash_type *h = create_str_hash();
    int vals[64];
    /* Insert items likely to spread across buckets */
    for (int i = 0; i < 64; i++) {
        vals[i] = i;
        char key[16];
        snprintf(key, sizeof(key), "spread_%d", i);
        hash_insert(h, key, &vals[i]);
    }
    int count = 0;
    hash_node_type *node = NULL;
    while (hash_next_item(h, &node) == HASH_STATUS_OK) {
        count++;
    }
    ASSERT_EQ(count, 64);
    destroy_str_hash(h);
}

/* === Bulk operations === */

TEST(hash_delete_all_clears) {
    hash_type *h = create_str_hash();
    int v = 1;
    for (int i = 0; i < 20; i++) {
        char key[16];
        snprintf(key, sizeof(key), "key_%d", i);
        hash_insert(h, key, &v);
    }
    hash_delete_all(h);
    hash_node_type *node = NULL;
    ASSERT_EQ(hash_next_item(h, &node), HASH_STATUS_KEY_NOT_FOUND);
    destroy_str_hash(h);
}

TEST(hash_delete_all_then_reinsert) {
    hash_type *h = create_str_hash();
    int v = 1;
    hash_insert(h, "before", &v);
    hash_delete_all(h);
    int v2 = 2;
    hash_insert(h, "after", &v2);
    void *rec = NULL;
    ASSERT_EQ(hash_find(h, "after", &rec), HASH_STATUS_OK);
    ASSERT_EQ(*(int *)rec, 2);
    ASSERT_EQ(hash_find(h, "before", &rec), HASH_STATUS_KEY_NOT_FOUND);
    destroy_str_hash(h);
}

TEST(hash_delete_all_free_frees_records) {
    hash_type *h = create_str_hash();
    for (int i = 0; i < 10; i++) {
        char key[16];
        snprintf(key, sizeof(key), "k%d", i);
        int *val = malloc(sizeof(int));
        *val = i;
        hash_insert(h, key, val);
    }
    hash_delete_all_free(h);
    hash_node_type *node = NULL;
    ASSERT_EQ(hash_next_item(h, &node), HASH_STATUS_KEY_NOT_FOUND);
    hash_destroy(h);
    free(h);
}

TEST(hash_delete_all_empty_table) {
    hash_type *h = create_str_hash();
    hash_delete_all(h); /* should not crash */
    hash_node_type *node = NULL;
    ASSERT_EQ(hash_next_item(h, &node), HASH_STATUS_KEY_NOT_FOUND);
    destroy_str_hash(h);
}

/* === Scale tests === */

TEST(hash_many_entries) {
    hash_type *h = create_str_hash();
    int vals[200];
    for (int i = 0; i < 200; i++) {
        vals[i] = i;
        char key[16];
        snprintf(key, sizeof(key), "entry_%d", i);
        hash_insert(h, key, &vals[i]);
    }
    for (int i = 0; i < 200; i++) {
        void *rec = NULL;
        char key[16];
        snprintf(key, sizeof(key), "entry_%d", i);
        ASSERT_EQ(hash_find(h, key, &rec), HASH_STATUS_OK);
        ASSERT_EQ(*(int *)rec, i);
    }
    destroy_str_hash(h);
}

TEST(hash_1000_entries) {
    hash_type *h = create_str_hash();
    int vals[1000];
    for (int i = 0; i < 1000; i++) {
        vals[i] = i * 3;
        char key[32];
        snprintf(key, sizeof(key), "big_entry_%d", i);
        hash_insert(h, key, &vals[i]);
    }
    for (int i = 0; i < 1000; i++) {
        void *rec = NULL;
        char key[32];
        snprintf(key, sizeof(key), "big_entry_%d", i);
        ASSERT_EQ(hash_find(h, key, &rec), HASH_STATUS_OK);
        ASSERT_EQ(*(int *)rec, i * 3);
    }
    destroy_str_hash(h);
}

TEST(hash_insert_delete_cycle) {
    hash_type *h = create_str_hash();
    int val = 0;
    for (int round = 0; round < 5; round++) {
        for (int i = 0; i < 50; i++) {
            char key[16];
            snprintf(key, sizeof(key), "cycle_%d", i);
            hash_insert(h, key, &val);
        }
        for (int i = 0; i < 50; i++) {
            char key[16];
            snprintf(key, sizeof(key), "cycle_%d", i);
            hash_delete(h, key);
        }
        hash_node_type *node = NULL;
        ASSERT_EQ(hash_next_item(h, &node), HASH_STATUS_KEY_NOT_FOUND);
    }
    destroy_str_hash(h);
}

TEST(hash_bucket_cached_correctly) {
    hash_type *h = create_str_hash();
    int val = 1;
    hash_insert(h, "test_bucket", &val);
    hash_node_type *node = NULL;
    hash_next_item(h, &node);
    ASSERT_NOT_NULL(node);
    /* The cached bucket should match the hash function result */
    int expected = str_hash("test_bucket");
    ASSERT_EQ(node->bucket, expected);
    destroy_str_hash(h);
}

TEST(hash_key_is_copied) {
    hash_type *h = create_str_hash();
    char key[16] = "original";
    int val = 1;
    hash_insert(h, key, &val);
    /* Modify the original key - shouldn't affect the hash */
    strcpy(key, "modified");
    void *rec = NULL;
    ASSERT_EQ(hash_find(h, "original", &rec), HASH_STATUS_OK);
    ASSERT_EQ(hash_find(h, "modified", &rec), HASH_STATUS_KEY_NOT_FOUND);
    destroy_str_hash(h);
}

/* === Iteration during modification === */

TEST(hash_delete_all_then_insert_cycle) {
    hash_type *h = create_str_hash();
    int val = 1;
    /* Repeatedly fill and empty the hash */
    for (int round = 0; round < 10; round++) {
        for (int i = 0; i < 100; i++) {
            char key[16];
            snprintf(key, sizeof(key), "r%d_k%d", round, i);
            hash_insert(h, key, &val);
        }
        hash_delete_all(h);
        hash_node_type *node = NULL;
        ASSERT_EQ(hash_next_item(h, &node), HASH_STATUS_KEY_NOT_FOUND);
    }
    destroy_str_hash(h);
}

TEST(hash_delete_node_only_in_bucket) {
    /* Single node in a bucket, delete via node pointer */
    hash_type *h = create_str_hash();
    int val = 42;
    hash_insert(h, "unique", &val);
    hash_node_type *node = NULL;
    hash_next_item(h, &node);
    ASSERT_NOT_NULL(node);
    ASSERT_EQ(hash_delete_node(h, node), HASH_STATUS_OK);
    node = NULL;
    ASSERT_EQ(hash_next_item(h, &node), HASH_STATUS_KEY_NOT_FOUND);
    destroy_str_hash(h);
}

/* === Overwrite behavior === */

TEST(hash_overwrite_many_times) {
    hash_type *h = create_str_hash();
    int vals[100];
    for (int i = 0; i < 100; i++) {
        vals[i] = i;
        hash_insert(h, "same_key", &vals[i]);
    }
    /* Most recent insert should be found first */
    void *rec = NULL;
    ASSERT_EQ(hash_find(h, "same_key", &rec), HASH_STATUS_OK);
    ASSERT_EQ(*(int *)rec, 99);
    destroy_str_hash(h);
}

/* === Delete and reinsert stress === */

TEST(hash_alternating_insert_delete) {
    hash_type *h = create_str_hash();
    int val = 0;
    /* Insert odd, delete even pattern */
    for (int i = 0; i < 200; i++) {
        char key[16];
        snprintf(key, sizeof(key), "alt_%d", i);
        hash_insert(h, key, &val);
    }
    for (int i = 0; i < 200; i += 2) {
        char key[16];
        snprintf(key, sizeof(key), "alt_%d", i);
        ASSERT_EQ(hash_delete(h, key), HASH_STATUS_OK);
    }
    /* Verify odd keys remain, even keys gone */
    for (int i = 0; i < 200; i++) {
        char key[16];
        void *rec = NULL;
        snprintf(key, sizeof(key), "alt_%d", i);
        if (i % 2 == 0) {
            ASSERT_EQ(hash_find(h, key, &rec), HASH_STATUS_KEY_NOT_FOUND);
        } else {
            ASSERT_EQ(hash_find(h, key, &rec), HASH_STATUS_OK);
        }
    }
    destroy_str_hash(h);
}

/* === Iteration count with collisions === */

TEST(hash_collision_iterate_count) {
    hash_type *h = xcalloc(1, sizeof(hash_type));
    h->size = 4;
    h->compare = str_compare;
    h->hash = always_zero_hash;
    h->copy_key = str_copy_key;
    h->delete_key = str_delete_key;
    hash_initialise(h);
    int vals[10];
    for (int i = 0; i < 10; i++) {
        vals[i] = i;
        char key[8];
        snprintf(key, sizeof(key), "c%d", i);
        hash_insert(h, key, &vals[i]);
    }
    int count = 0;
    hash_node_type *node = NULL;
    while (hash_next_item(h, &node) == HASH_STATUS_OK) {
        count++;
    }
    ASSERT_EQ(count, 10);
    hash_delete_all(h);
    hash_destroy(h);
    free(h);
}

/* === 5000 entries stress test === */

TEST(hash_5000_entries) {
    hash_type *h = create_str_hash();
    int vals[5000];
    for (int i = 0; i < 5000; i++) {
        vals[i] = i * 7;
        char key[32];
        snprintf(key, sizeof(key), "stress_%d", i);
        hash_insert(h, key, &vals[i]);
    }
    for (int i = 0; i < 5000; i++) {
        void *rec = NULL;
        char key[32];
        snprintf(key, sizeof(key), "stress_%d", i);
        ASSERT_EQ(hash_find(h, key, &rec), HASH_STATUS_OK);
        ASSERT_EQ(*(int *)rec, i * 7);
    }
    /* Iteration count */
    int count = 0;
    hash_node_type *node = NULL;
    while (hash_next_item(h, &node) == HASH_STATUS_OK) {
        count++;
    }
    ASSERT_EQ(count, 5000);
    destroy_str_hash(h);
}

/* === Int-keyed hash tests === */

TEST(hash_int_key_multiple) {
    hash_type *h = create_int_hash();
    int keys[20], vals[20];
    for (int i = 0; i < 20; i++) {
        keys[i] = i * 100;
        vals[i] = i;
        hash_insert(h, &keys[i], &vals[i]);
    }
    for (int i = 0; i < 20; i++) {
        void *rec = NULL;
        ASSERT_EQ(hash_find(h, &keys[i], &rec), HASH_STATUS_OK);
        ASSERT_EQ(*(int *)rec, i);
    }
    hash_delete_all(h);
    hash_destroy(h);
    free(h);
}

TEST(hash_int_key_delete) {
    hash_type *h = create_int_hash();
    int key = 42, val = 99;
    hash_insert(h, &key, &val);
    ASSERT_EQ(hash_delete(h, &key), HASH_STATUS_OK);
    void *rec = NULL;
    ASSERT_EQ(hash_find(h, &key, &rec), HASH_STATUS_KEY_NOT_FOUND);
    hash_destroy(h);
    free(h);
}

TEST(hash_int_key_collision) {
    /* Keys that hash to the same bucket (mod 32) */
    hash_type *h = create_int_hash();
    int k1 = 5, k2 = 37, k3 = 69; /* all hash to 5 */
    int v1 = 10, v2 = 20, v3 = 30;
    hash_insert(h, &k1, &v1);
    hash_insert(h, &k2, &v2);
    hash_insert(h, &k3, &v3);
    void *rec = NULL;
    ASSERT_EQ(hash_find(h, &k1, &rec), HASH_STATUS_OK);
    ASSERT_EQ(*(int *)rec, 10);
    ASSERT_EQ(hash_find(h, &k2, &rec), HASH_STATUS_OK);
    ASSERT_EQ(*(int *)rec, 20);
    ASSERT_EQ(hash_find(h, &k3, &rec), HASH_STATUS_OK);
    ASSERT_EQ(*(int *)rec, 30);
    hash_delete_all(h);
    hash_destroy(h);
    free(h);
}

/* === High load factor === */

static int str_hash_small(void *key) {
    char *s = (char *)key;
    unsigned int h = 0;
    while (*s) {
        h = h * 31 + (unsigned char)*s++;
    }
    return h % 4;
}

TEST(hash_high_load_factor) {
    /* 200 entries in 4-bucket table */
    hash_type *h = xcalloc(1, sizeof(hash_type));
    h->size = 4;
    h->compare = str_compare;
    h->hash = str_hash_small;
    h->copy_key = str_copy_key;
    h->delete_key = str_delete_key;
    hash_initialise(h);
    int vals[200];
    for (int i = 0; i < 200; i++) {
        vals[i] = i;
        char key[32];
        snprintf(key, sizeof(key), "load_%d", i);
        hash_insert(h, key, &vals[i]);
    }
    /* All should still be findable */
    for (int i = 0; i < 200; i++) {
        char key[32];
        snprintf(key, sizeof(key), "load_%d", i);
        void *rec = NULL;
        ASSERT_EQ(hash_find(h, key, &rec), HASH_STATUS_OK);
        ASSERT_EQ(*(int *)rec, i);
    }
    hash_delete_all(h);
    hash_destroy(h);
    free(h);
}

/* === Small table stress === */

TEST(hash_single_bucket_stress) {
    hash_type *h = xcalloc(1, sizeof(hash_type));
    h->size = 4;
    h->compare = str_compare;
    h->hash = always_zero_hash;
    h->copy_key = str_copy_key;
    h->delete_key = str_delete_key;
    hash_initialise(h);
    int vals[50];
    for (int i = 0; i < 50; i++) {
        vals[i] = i;
        char key[16];
        snprintf(key, sizeof(key), "s%d", i);
        hash_insert(h, key, &vals[i]);
    }
    ASSERT_NOT_NULL(h->table[0]);
    int count = 0;
    hash_node_type *node = NULL;
    while (hash_next_item(h, &node) == HASH_STATUS_OK) {
        count++;
    }
    ASSERT_EQ(count, 50);
    /* Delete every other entry */
    for (int i = 0; i < 50; i += 2) {
        char key[16];
        snprintf(key, sizeof(key), "s%d", i);
        hash_delete(h, key);
    }
    count = 0;
    node = NULL;
    while (hash_next_item(h, &node) == HASH_STATUS_OK) {
        count++;
    }
    ASSERT_EQ(count, 25);
    hash_delete_all(h);
    hash_destroy(h);
    free(h);
}

/* === Delete node during iteration === */

TEST(hash_delete_node_during_iteration) {
    hash_type *h = create_str_hash();
    int vals[10];
    for (int i = 0; i < 10; i++) {
        vals[i] = i;
        char key[8];
        snprintf(key, sizeof(key), "d%d", i);
        hash_insert(h, key, &vals[i]);
    }
    /* Delete all nodes via repeated iteration from start */
    int deleted = 0;
    hash_node_type *node = NULL;
    while (hash_next_item(h, &node) == HASH_STATUS_OK) {
        hash_delete_node(h, node);
        deleted++;
        node = NULL; /* restart iteration */
    }
    ASSERT_EQ(deleted, 10);
    /* Table should be empty now */
    node = NULL;
    ASSERT_EQ(hash_next_item(h, &node), HASH_STATUS_KEY_NOT_FOUND);
    destroy_str_hash(h);
}

/* === Iteration sum === */

TEST(hash_iterate_sum_records) {
    hash_type *h = create_str_hash();
    int vals[10];
    for (int i = 0; i < 10; i++) {
        vals[i] = i + 1;
        char key[8];
        snprintf(key, sizeof(key), "v%d", i);
        hash_insert(h, key, &vals[i]);
    }
    int sum = 0;
    hash_node_type *node = NULL;
    while (hash_next_item(h, &node) == HASH_STATUS_OK) {
        sum += *(int *)node->record;
    }
    ASSERT_EQ(sum, 55); /* 1+2+...+10 */
    destroy_str_hash(h);
}

/* === Re-destroy is safe === */

TEST(hash_double_destroy) {
    hash_type h;
    h.size = 8;
    hash_initialise(&h);
    hash_destroy(&h);
    /* table is NULL after destroy, second destroy should handle it */
}

/* === Delete non-existent from collision chain === */

TEST(hash_delete_missing_from_chain) {
    hash_type *h = xcalloc(1, sizeof(hash_type));
    h->size = 4;
    h->compare = str_compare;
    h->hash = always_zero_hash;
    h->copy_key = str_copy_key;
    h->delete_key = str_delete_key;
    hash_initialise(h);
    int v = 1;
    hash_insert(h, "exists", &v);
    ASSERT_EQ(hash_delete(h, "nope"), HASH_STATUS_KEY_NOT_FOUND);
    void *rec = NULL;
    ASSERT_EQ(hash_find(h, "exists", &rec), HASH_STATUS_OK);
    hash_delete_all(h);
    hash_destroy(h);
    free(h);
}

int main(void) {
    TEST_SUITE("Generic Hash Table Tests");

    RUN(hash_initialise_returns_ok);
    RUN(hash_initialise_table_zeroed);
    RUN(hash_destroy_returns_ok);
    RUN(hash_insert_and_find);
    RUN(hash_insert_returns_ok);
    RUN(hash_find_missing_key);
    RUN(hash_find_empty_table);
    RUN(hash_insert_multiple);
    RUN(hash_insert_single_char_keys);
    RUN(hash_insert_long_keys);
    RUN(hash_insert_empty_string_key);
    RUN(hash_overwrite_same_key);
    RUN(hash_insert_null_rec);
    RUN(hash_int_key_insert_find);
    RUN(hash_int_key_miss);
    RUN(hash_delete_existing);
    RUN(hash_delete_missing);
    RUN(hash_delete_from_empty);
    RUN(hash_delete_only_entry);
    RUN(hash_delete_first_preserves_rest);
    RUN(hash_delete_then_reinsert);
    RUN(hash_delete_all_entries);
    RUN(hash_delete_node_by_pointer);
    RUN(hash_delete_node_first_in_bucket);
    RUN(hash_delete_node_last_in_bucket);
    RUN(hash_collision_chain_all_findable);
    RUN(hash_collision_delete_middle);
    RUN(hash_collision_delete_head);
    RUN(hash_collision_delete_tail);
    RUN(hash_collision_long_chain);
    RUN(hash_next_item_iterates_all);
    RUN(hash_next_item_empty);
    RUN(hash_next_item_single);
    RUN(hash_next_item_traverses_chain);
    RUN(hash_iteration_visits_all_buckets);
    RUN(hash_delete_all_clears);
    RUN(hash_delete_all_then_reinsert);
    RUN(hash_delete_all_free_frees_records);
    RUN(hash_delete_all_empty_table);
    RUN(hash_many_entries);
    RUN(hash_1000_entries);
    RUN(hash_insert_delete_cycle);
    RUN(hash_bucket_cached_correctly);
    RUN(hash_key_is_copied);
    RUN(hash_delete_all_then_insert_cycle);
    RUN(hash_delete_node_only_in_bucket);
    RUN(hash_overwrite_many_times);
    RUN(hash_alternating_insert_delete);
    RUN(hash_collision_iterate_count);
    RUN(hash_5000_entries);
    RUN(hash_int_key_multiple);
    RUN(hash_int_key_delete);
    RUN(hash_int_key_collision);
    RUN(hash_high_load_factor);
    RUN(hash_single_bucket_stress);
    RUN(hash_delete_node_during_iteration);
    RUN(hash_iterate_sum_records);
    RUN(hash_double_destroy);
    RUN(hash_delete_missing_from_chain);

    TEST_REPORT();
}
