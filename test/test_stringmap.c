/*
 * test_stringmap.c: tests for stringmap (binary tree)
 */

#include <stdlib.h>
#include <string.h>
#include "test.h"
#include "stringmap.h"
#include "iftop.h"

/* === Creation === */

TEST(stringmap_new_succeeds) {
    stringmap s = stringmap_new();
    ASSERT_NOT_NULL(s);
    ASSERT_NULL(s->key);
    stringmap_delete(s);
}

TEST(stringmap_new_children_null) {
    stringmap s = stringmap_new();
    ASSERT_NULL(s->lesser);
    ASSERT_NULL(s->greater);
    stringmap_delete(s);
}

/* === Insert === */

TEST(stringmap_insert_first) {
    stringmap s = stringmap_new();
    item *result = stringmap_insert(s, "hello", item_long(42));
    ASSERT_NULL(result);
    ASSERT_STR_EQ(s->key, "hello");
    ASSERT_EQ(s->data.num, 42);
    stringmap_delete(s);
}

TEST(stringmap_insert_two_left) {
    stringmap s = stringmap_new();
    stringmap_insert(s, "m", item_long(1));
    stringmap_insert(s, "a", item_long(2));
    ASSERT_NOT_NULL(s->lesser);
    ASSERT_NULL(s->greater);
    ASSERT_STR_EQ(s->lesser->key, "a");
    stringmap_delete(s);
}

TEST(stringmap_insert_two_right) {
    stringmap s = stringmap_new();
    stringmap_insert(s, "a", item_long(1));
    stringmap_insert(s, "m", item_long(2));
    ASSERT_NULL(s->lesser);
    ASSERT_NOT_NULL(s->greater);
    ASSERT_STR_EQ(s->greater->key, "m");
    stringmap_delete(s);
}

TEST(stringmap_insert_multiple) {
    stringmap s = stringmap_new();
    stringmap_insert(s, "banana", item_long(2));
    stringmap_insert(s, "apple", item_long(1));
    stringmap_insert(s, "cherry", item_long(3));
    stringmap f;
    f = stringmap_find(s, "banana"); ASSERT_NOT_NULL(f); ASSERT_EQ(f->data.num, 2);
    f = stringmap_find(s, "apple");  ASSERT_NOT_NULL(f); ASSERT_EQ(f->data.num, 1);
    f = stringmap_find(s, "cherry"); ASSERT_NOT_NULL(f); ASSERT_EQ(f->data.num, 3);
    stringmap_delete(s);
}

TEST(stringmap_insert_duplicate_returns_existing) {
    stringmap s = stringmap_new();
    stringmap_insert(s, "key", item_long(100));
    item *existing = stringmap_insert(s, "key", item_long(200));
    ASSERT_NOT_NULL(existing);
    ASSERT_EQ(existing->num, 100);
    stringmap_delete(s);
}

TEST(stringmap_insert_duplicate_can_update) {
    stringmap s = stringmap_new();
    stringmap_insert(s, "key", item_long(100));
    item *existing = stringmap_insert(s, "key", item_long(200));
    ASSERT_NOT_NULL(existing);
    *existing = item_long(200);
    stringmap f = stringmap_find(s, "key");
    ASSERT_EQ(f->data.num, 200);
    stringmap_delete(s);
}

TEST(stringmap_insert_single_char_keys) {
    stringmap s = stringmap_new();
    for (char c = 'a'; c <= 'z'; c++) {
        char key[2] = {c, '\0'};
        stringmap_insert(s, key, item_long(c));
    }
    for (char c = 'a'; c <= 'z'; c++) {
        char key[2] = {c, '\0'};
        stringmap f = stringmap_find(s, key);
        ASSERT_NOT_NULL(f);
        ASSERT_EQ(f->data.num, c);
    }
    stringmap_delete(s);
}

TEST(stringmap_insert_numeric_keys) {
    stringmap s = stringmap_new();
    for (int i = 0; i < 50; i++) {
        char key[16];
        snprintf(key, sizeof(key), "%d", i);
        stringmap_insert(s, key, item_long(i));
    }
    for (int i = 0; i < 50; i++) {
        char key[16];
        snprintf(key, sizeof(key), "%d", i);
        stringmap f = stringmap_find(s, key);
        ASSERT_NOT_NULL(f);
        ASSERT_EQ(f->data.num, i);
    }
    stringmap_delete(s);
}

TEST(stringmap_insert_long_keys) {
    stringmap s = stringmap_new();
    char key1[256], key2[256];
    memset(key1, 'a', 255); key1[255] = '\0';
    memset(key2, 'b', 255); key2[255] = '\0';
    stringmap_insert(s, key1, item_long(1));
    stringmap_insert(s, key2, item_long(2));
    stringmap f = stringmap_find(s, key1);
    ASSERT_NOT_NULL(f);
    ASSERT_EQ(f->data.num, 1);
    f = stringmap_find(s, key2);
    ASSERT_NOT_NULL(f);
    ASSERT_EQ(f->data.num, 2);
    stringmap_delete(s);
}

TEST(stringmap_insert_common_prefix) {
    stringmap s = stringmap_new();
    stringmap_insert(s, "prefix_a", item_long(1));
    stringmap_insert(s, "prefix_b", item_long(2));
    stringmap_insert(s, "prefix_c", item_long(3));
    stringmap_insert(s, "prefix_", item_long(0));
    ASSERT_NOT_NULL(stringmap_find(s, "prefix_a"));
    ASSERT_NOT_NULL(stringmap_find(s, "prefix_b"));
    ASSERT_NOT_NULL(stringmap_find(s, "prefix_c"));
    ASSERT_NOT_NULL(stringmap_find(s, "prefix_"));
    stringmap_delete(s);
}

TEST(stringmap_insert_into_null_returns_null) {
    item *result = stringmap_insert(NULL, "key", item_long(1));
    ASSERT_NULL(result);
}

/* === Find === */

TEST(stringmap_find_existing) {
    stringmap s = stringmap_new();
    stringmap_insert(s, "test", item_long(55));
    stringmap f = stringmap_find(s, "test");
    ASSERT_NOT_NULL(f);
    ASSERT_EQ(f->data.num, 55);
    stringmap_delete(s);
}

TEST(stringmap_find_missing) {
    stringmap s = stringmap_new();
    stringmap_insert(s, "exists", item_long(1));
    ASSERT_NULL(stringmap_find(s, "missing"));
    stringmap_delete(s);
}

TEST(stringmap_find_empty) {
    stringmap s = stringmap_new();
    ASSERT_NULL(stringmap_find(s, "anything"));
    stringmap_delete(s);
}

TEST(stringmap_find_null_map) {
    ASSERT_NULL(stringmap_find(NULL, "test"));
}

TEST(stringmap_find_prefix_no_match) {
    stringmap s = stringmap_new();
    stringmap_insert(s, "hello", item_long(1));
    ASSERT_NULL(stringmap_find(s, "hell"));
    ASSERT_NULL(stringmap_find(s, "helloo"));
    ASSERT_NULL(stringmap_find(s, "Hello"));
    stringmap_delete(s);
}

TEST(stringmap_find_similar_keys) {
    stringmap s = stringmap_new();
    stringmap_insert(s, "abc", item_long(1));
    stringmap_insert(s, "abd", item_long(2));
    stringmap_insert(s, "abe", item_long(3));
    ASSERT_EQ(stringmap_find(s, "abc")->data.num, 1);
    ASSERT_EQ(stringmap_find(s, "abd")->data.num, 2);
    ASSERT_EQ(stringmap_find(s, "abe")->data.num, 3);
    ASSERT_NULL(stringmap_find(s, "abf"));
    stringmap_delete(s);
}

TEST(stringmap_find_returns_correct_node) {
    stringmap s = stringmap_new();
    stringmap_insert(s, "key1", item_long(10));
    stringmap_insert(s, "key2", item_long(20));
    stringmap f = stringmap_find(s, "key1");
    ASSERT_STR_EQ(f->key, "key1");
    ASSERT_EQ(f->data.num, 10);
    stringmap_delete(s);
}

/* === Pointer storage === */

TEST(stringmap_store_strings) {
    stringmap s = stringmap_new();
    stringmap_insert(s, "greeting", item_ptr(xstrdup("hello")));
    stringmap_insert(s, "farewell", item_ptr(xstrdup("goodbye")));
    ASSERT_STR_EQ((char *)stringmap_find(s, "greeting")->data.ptr, "hello");
    ASSERT_STR_EQ((char *)stringmap_find(s, "farewell")->data.ptr, "goodbye");
    stringmap_delete_free(s);
}

TEST(stringmap_store_null_value) {
    stringmap s = stringmap_new();
    stringmap_insert(s, "nil", item_ptr(NULL));
    stringmap f = stringmap_find(s, "nil");
    ASSERT_NOT_NULL(f);
    ASSERT_NULL(f->data.ptr);
    stringmap_delete(s);
}

/* === Tree structure === */

TEST(stringmap_sorted_insertion_right_chain) {
    stringmap s = stringmap_new();
    stringmap_insert(s, "a", item_long(1));
    stringmap_insert(s, "b", item_long(2));
    stringmap_insert(s, "c", item_long(3));
    stringmap_insert(s, "d", item_long(4));
    stringmap_insert(s, "e", item_long(5));
    for (char c = 'a'; c <= 'e'; c++) {
        char key[2] = {c, '\0'};
        stringmap f = stringmap_find(s, key);
        ASSERT_NOT_NULL(f);
        ASSERT_EQ(f->data.num, c - 'a' + 1);
    }
    stringmap_delete(s);
}

TEST(stringmap_reverse_sorted_insertion_left_chain) {
    stringmap s = stringmap_new();
    stringmap_insert(s, "e", item_long(5));
    stringmap_insert(s, "d", item_long(4));
    stringmap_insert(s, "c", item_long(3));
    stringmap_insert(s, "b", item_long(2));
    stringmap_insert(s, "a", item_long(1));
    for (char c = 'a'; c <= 'e'; c++) {
        char key[2] = {c, '\0'};
        stringmap f = stringmap_find(s, key);
        ASSERT_NOT_NULL(f);
        ASSERT_EQ(f->data.num, c - 'a' + 1);
    }
    stringmap_delete(s);
}

TEST(stringmap_balanced_insertion) {
    stringmap s = stringmap_new();
    stringmap_insert(s, "m", item_long(1));
    stringmap_insert(s, "f", item_long(2));
    stringmap_insert(s, "t", item_long(3));
    ASSERT_NOT_NULL(s->lesser);
    ASSERT_NOT_NULL(s->greater);
    ASSERT_STR_EQ(s->lesser->key, "f");
    ASSERT_STR_EQ(s->greater->key, "t");
    stringmap_delete(s);
}

TEST(stringmap_many_entries) {
    stringmap s = stringmap_new();
    for (int i = 0; i < 100; i++) {
        char key[16];
        snprintf(key, sizeof(key), "key_%03d", i);
        stringmap_insert(s, key, item_long(i));
    }
    for (int i = 0; i < 100; i++) {
        char key[16];
        snprintf(key, sizeof(key), "key_%03d", i);
        stringmap f = stringmap_find(s, key);
        ASSERT_NOT_NULL(f);
        ASSERT_EQ(f->data.num, i);
    }
    stringmap_delete(s);
}

TEST(stringmap_500_entries) {
    stringmap s = stringmap_new();
    for (int i = 0; i < 500; i++) {
        char key[32];
        snprintf(key, sizeof(key), "item_%05d", i);
        stringmap_insert(s, key, item_long(i));
    }
    /* Spot check */
    ASSERT_EQ(stringmap_find(s, "item_00000")->data.num, 0);
    ASSERT_EQ(stringmap_find(s, "item_00250")->data.num, 250);
    ASSERT_EQ(stringmap_find(s, "item_00499")->data.num, 499);
    ASSERT_NULL(stringmap_find(s, "item_00500"));
    stringmap_delete(s);
}

/* === Delete === */

TEST(stringmap_delete_null_safe) {
    stringmap_delete(NULL);
}

TEST(stringmap_delete_free_null_safe) {
    stringmap_delete_free(NULL);
}

TEST(stringmap_delete_single_node) {
    stringmap s = stringmap_new();
    stringmap_insert(s, "only", item_long(1));
    stringmap_delete(s);
}

TEST(stringmap_delete_free_complex) {
    stringmap s = stringmap_new();
    stringmap_insert(s, "m", item_ptr(xstrdup("middle")));
    stringmap_insert(s, "a", item_ptr(xstrdup("left")));
    stringmap_insert(s, "z", item_ptr(xstrdup("right")));
    stringmap_delete_free(s);
}

TEST(stringmap_delete_empty) {
    stringmap s = stringmap_new();
    stringmap_delete(s); /* no key set, should be fine */
}

/* === Key independence === */

TEST(stringmap_key_is_copied) {
    stringmap s = stringmap_new();
    char key[16] = "original";
    stringmap_insert(s, key, item_long(1));
    strcpy(key, "modified");
    stringmap f = stringmap_find(s, "original");
    ASSERT_NOT_NULL(f);
    ASSERT_NULL(stringmap_find(s, "modified"));
    stringmap_delete(s);
}

/* === Empty string key === */

TEST(stringmap_empty_string_key) {
    stringmap s = stringmap_new();
    stringmap_insert(s, "", item_long(99));
    stringmap f = stringmap_find(s, "");
    ASSERT_NOT_NULL(f);
    ASSERT_EQ(f->data.num, 99);
    stringmap_delete(s);
}

TEST(stringmap_empty_string_with_others) {
    stringmap s = stringmap_new();
    stringmap_insert(s, "", item_long(0));
    stringmap_insert(s, "a", item_long(1));
    stringmap_insert(s, "z", item_long(2));
    ASSERT_EQ(stringmap_find(s, "")->data.num, 0);
    ASSERT_EQ(stringmap_find(s, "a")->data.num, 1);
    ASSERT_EQ(stringmap_find(s, "z")->data.num, 2);
    stringmap_delete(s);
}

/* === Large scale === */

TEST(stringmap_1000_entries) {
    stringmap s = stringmap_new();
    for (int i = 0; i < 1000; i++) {
        char key[32];
        snprintf(key, sizeof(key), "entry_%05d", i);
        stringmap_insert(s, key, item_long(i));
    }
    /* Verify all findable */
    for (int i = 0; i < 1000; i++) {
        char key[32];
        snprintf(key, sizeof(key), "entry_%05d", i);
        stringmap f = stringmap_find(s, key);
        ASSERT_NOT_NULL(f);
        ASSERT_EQ(f->data.num, i);
    }
    /* Verify misses */
    ASSERT_NULL(stringmap_find(s, "entry_01000"));
    ASSERT_NULL(stringmap_find(s, "nonexistent"));
    stringmap_delete(s);
}

/* === Duplicate update pattern === */

TEST(stringmap_duplicate_update_all) {
    stringmap s = stringmap_new();
    for (int i = 0; i < 20; i++) {
        char key[16];
        snprintf(key, sizeof(key), "k%d", i);
        stringmap_insert(s, key, item_long(i));
    }
    /* Update all values via duplicate insert */
    for (int i = 0; i < 20; i++) {
        char key[16];
        snprintf(key, sizeof(key), "k%d", i);
        item *existing = stringmap_insert(s, key, item_long(i + 100));
        ASSERT_NOT_NULL(existing);
        *existing = item_long(i + 100);
    }
    for (int i = 0; i < 20; i++) {
        char key[16];
        snprintf(key, sizeof(key), "k%d", i);
        ASSERT_EQ(stringmap_find(s, key)->data.num, i + 100);
    }
    stringmap_delete(s);
}

/* === Pointer storage with delete_free === */

TEST(stringmap_delete_free_many) {
    stringmap s = stringmap_new();
    for (int i = 0; i < 50; i++) {
        char key[16], *val;
        snprintf(key, sizeof(key), "k%d", i);
        val = xmalloc(32);
        snprintf(val, 32, "value_%d", i);
        stringmap_insert(s, key, item_ptr(val));
    }
    stringmap_delete_free(s); /* should free all 50 values without crash */
}

/* === Case sensitivity === */

TEST(stringmap_case_sensitive) {
    stringmap s = stringmap_new();
    stringmap_insert(s, "Key", item_long(1));
    stringmap_insert(s, "key", item_long(2));
    stringmap_insert(s, "KEY", item_long(3));
    ASSERT_EQ(stringmap_find(s, "Key")->data.num, 1);
    ASSERT_EQ(stringmap_find(s, "key")->data.num, 2);
    ASSERT_EQ(stringmap_find(s, "KEY")->data.num, 3);
    stringmap_delete(s);
}

/* === Special character keys === */

TEST(stringmap_special_char_keys) {
    stringmap s = stringmap_new();
    stringmap_insert(s, "key-with-dashes", item_long(1));
    stringmap_insert(s, "key.with.dots", item_long(2));
    stringmap_insert(s, "key_with_underscores", item_long(3));
    stringmap_insert(s, "key/with/slashes", item_long(4));
    ASSERT_EQ(stringmap_find(s, "key-with-dashes")->data.num, 1);
    ASSERT_EQ(stringmap_find(s, "key.with.dots")->data.num, 2);
    ASSERT_EQ(stringmap_find(s, "key_with_underscores")->data.num, 3);
    ASSERT_EQ(stringmap_find(s, "key/with/slashes")->data.num, 4);
    stringmap_delete(s);
}

/* === Mixed pointer and long storage === */

TEST(stringmap_mixed_ptr_long) {
    stringmap s = stringmap_new();
    stringmap_insert(s, "count", item_long(42));
    stringmap_insert(s, "name", item_ptr(xstrdup("hello")));
    ASSERT_EQ(stringmap_find(s, "count")->data.num, 42);
    ASSERT_STR_EQ((char *)stringmap_find(s, "name")->data.ptr, "hello");
    /* Free only the string value */
    free(stringmap_find(s, "name")->data.ptr);
    stringmap_delete(s);
}

/* === Deep left chain find === */

TEST(stringmap_deep_left_chain) {
    stringmap s = stringmap_new();
    /* Insert in reverse order to create deep left chain */
    for (int i = 99; i >= 0; i--) {
        char key[16];
        snprintf(key, sizeof(key), "k%03d", i);
        stringmap_insert(s, key, item_long(i));
    }
    /* First key should be deep in left chain */
    stringmap f = stringmap_find(s, "k000");
    ASSERT_NOT_NULL(f);
    ASSERT_EQ(f->data.num, 0);
    /* Last inserted should be root */
    f = stringmap_find(s, "k099");
    ASSERT_NOT_NULL(f);
    ASSERT_EQ(f->data.num, 99);
    stringmap_delete(s);
}

/* === Delete and recreate === */

TEST(stringmap_delete_then_recreate) {
    stringmap s = stringmap_new();
    stringmap_insert(s, "a", item_long(1));
    stringmap_insert(s, "b", item_long(2));
    stringmap_delete(s);
    /* Create new one and ensure it works */
    s = stringmap_new();
    stringmap_insert(s, "x", item_long(10));
    ASSERT_EQ(stringmap_find(s, "x")->data.num, 10);
    ASSERT_NULL(stringmap_find(s, "a"));
    stringmap_delete(s);
}

/* === 2000 entries stress === */

TEST(stringmap_2000_entries) {
    stringmap s = stringmap_new();
    for (int i = 0; i < 2000; i++) {
        char key[32];
        snprintf(key, sizeof(key), "stress_%05d", i);
        stringmap_insert(s, key, item_long(i));
    }
    /* Verify all findable */
    for (int i = 0; i < 2000; i++) {
        char key[32];
        snprintf(key, sizeof(key), "stress_%05d", i);
        stringmap f = stringmap_find(s, key);
        ASSERT_NOT_NULL(f);
        ASSERT_EQ(f->data.num, i);
    }
    /* Verify misses */
    ASSERT_NULL(stringmap_find(s, "stress_02000"));
    ASSERT_NULL(stringmap_find(s, "nope"));
    stringmap_delete(s);
}

/* === Pointer values with delete_free === */

TEST(stringmap_delete_free_100) {
    stringmap s = stringmap_new();
    for (int i = 0; i < 100; i++) {
        char key[16];
        snprintf(key, sizeof(key), "p%d", i);
        char *val = xmalloc(16);
        snprintf(val, 16, "val_%d", i);
        stringmap_insert(s, key, item_ptr(val));
    }
    stringmap_delete_free(s);
}

/* === Whitespace in keys === */

TEST(stringmap_whitespace_keys) {
    stringmap s = stringmap_new();
    stringmap_insert(s, "a b", item_long(1));
    stringmap_insert(s, "a\tb", item_long(2));
    stringmap_insert(s, " a", item_long(3));
    ASSERT_EQ(stringmap_find(s, "a b")->data.num, 1);
    ASSERT_EQ(stringmap_find(s, "a\tb")->data.num, 2);
    ASSERT_EQ(stringmap_find(s, " a")->data.num, 3);
    ASSERT_NULL(stringmap_find(s, "ab"));
    stringmap_delete(s);
}

int main(void) {
    TEST_SUITE("Stringmap Tests");

    RUN(stringmap_new_succeeds);
    RUN(stringmap_new_children_null);
    RUN(stringmap_insert_first);
    RUN(stringmap_insert_two_left);
    RUN(stringmap_insert_two_right);
    RUN(stringmap_insert_multiple);
    RUN(stringmap_insert_duplicate_returns_existing);
    RUN(stringmap_insert_duplicate_can_update);
    RUN(stringmap_insert_single_char_keys);
    RUN(stringmap_insert_numeric_keys);
    RUN(stringmap_insert_long_keys);
    RUN(stringmap_insert_common_prefix);
    RUN(stringmap_insert_into_null_returns_null);
    RUN(stringmap_find_existing);
    RUN(stringmap_find_missing);
    RUN(stringmap_find_empty);
    RUN(stringmap_find_null_map);
    RUN(stringmap_find_prefix_no_match);
    RUN(stringmap_find_similar_keys);
    RUN(stringmap_find_returns_correct_node);
    RUN(stringmap_store_strings);
    RUN(stringmap_store_null_value);
    RUN(stringmap_sorted_insertion_right_chain);
    RUN(stringmap_reverse_sorted_insertion_left_chain);
    RUN(stringmap_balanced_insertion);
    RUN(stringmap_many_entries);
    RUN(stringmap_500_entries);
    RUN(stringmap_delete_null_safe);
    RUN(stringmap_delete_free_null_safe);
    RUN(stringmap_delete_single_node);
    RUN(stringmap_delete_free_complex);
    RUN(stringmap_delete_empty);
    RUN(stringmap_key_is_copied);
    RUN(stringmap_empty_string_key);
    RUN(stringmap_empty_string_with_others);
    RUN(stringmap_1000_entries);
    RUN(stringmap_duplicate_update_all);
    RUN(stringmap_delete_free_many);
    RUN(stringmap_case_sensitive);
    RUN(stringmap_special_char_keys);
    RUN(stringmap_mixed_ptr_long);
    RUN(stringmap_deep_left_chain);
    RUN(stringmap_delete_then_recreate);
    RUN(stringmap_2000_entries);
    RUN(stringmap_delete_free_100);
    RUN(stringmap_whitespace_keys);

    TEST_REPORT();
}
