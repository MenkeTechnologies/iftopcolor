/*
 * test_util.c: tests for utility functions (xmalloc, xcalloc, etc.)
 */

#include <stdlib.h>
#include <string.h>
#include "test.h"
#include "iftop.h"

/* === xmalloc === */

TEST(xmalloc_returns_nonnull) {
    void *p = xmalloc(100);
    ASSERT_NOT_NULL(p);
    free(p);
}

TEST(xmalloc_size_1) {
    void *p = xmalloc(1);
    ASSERT_NOT_NULL(p);
    *(char *)p = 'x';
    ASSERT_EQ(*(char *)p, 'x');
    free(p);
}

TEST(xmalloc_size_8) {
    void *p = xmalloc(8);
    ASSERT_NOT_NULL(p);
    memset(p, 0xFF, 8);
    free(p);
}

TEST(xmalloc_memory_writable) {
    char *p = xmalloc(64);
    memset(p, 'A', 64);
    ASSERT_EQ(p[0], 'A');
    ASSERT_EQ(p[63], 'A');
    free(p);
}

TEST(xmalloc_large_allocation) {
    void *p = xmalloc(1024 * 1024); /* 1MB */
    ASSERT_NOT_NULL(p);
    memset(p, 0, 1024 * 1024);
    free(p);
}

TEST(xmalloc_various_sizes) {
    size_t sizes[] = {1, 7, 15, 16, 17, 31, 32, 33, 63, 64, 65, 127, 128,
                      255, 256, 512, 1024, 4096, 65536};
    for (int i = 0; i < (int)(sizeof(sizes)/sizeof(sizes[0])); i++) {
        void *p = xmalloc(sizes[i]);
        ASSERT_NOT_NULL(p);
        memset(p, 0xFF, sizes[i]);
        free(p);
    }
}

TEST(xmalloc_sequential_allocations) {
    void *ptrs[100];
    for (int i = 0; i < 100; i++) {
        ptrs[i] = xmalloc(64);
        ASSERT_NOT_NULL(ptrs[i]);
        memset(ptrs[i], i & 0xFF, 64);
    }
    /* Verify each block still has its data */
    for (int i = 0; i < 100; i++) {
        ASSERT_EQ(((char *)ptrs[i])[0], (char)(i & 0xFF));
        free(ptrs[i]);
    }
}

TEST(xmalloc_alignment) {
    /* Returned pointer should be aligned */
    for (int i = 0; i < 10; i++) {
        void *p = xmalloc(i + 1);
        ASSERT_EQ((unsigned long)p % sizeof(void *), 0);
        free(p);
    }
}

/* === xcalloc === */

TEST(xcalloc_returns_nonnull) {
    void *p = xcalloc(10, 10);
    ASSERT_NOT_NULL(p);
    free(p);
}

TEST(xcalloc_returns_zeroed) {
    char *p = xcalloc(64, 1);
    ASSERT_NOT_NULL(p);
    for (int i = 0; i < 64; i++)
        ASSERT_EQ(p[i], 0);
    free(p);
}

TEST(xcalloc_correct_size) {
    int *p = xcalloc(10, sizeof(int));
    ASSERT_NOT_NULL(p);
    for (int i = 0; i < 10; i++)
        ASSERT_EQ(p[i], 0);
    free(p);
}

TEST(xcalloc_1x1) {
    char *p = xcalloc(1, 1);
    ASSERT_NOT_NULL(p);
    ASSERT_EQ(*p, 0);
    free(p);
}

TEST(xcalloc_large) {
    char *p = xcalloc(1024, 1024);
    ASSERT_NOT_NULL(p);
    /* Spot check zeroing */
    ASSERT_EQ(p[0], 0);
    ASSERT_EQ(p[512 * 1024], 0);
    ASSERT_EQ(p[1024 * 1024 - 1], 0);
    free(p);
}

TEST(xcalloc_struct_array) {
    typedef struct { int a; long b; char c[16]; } test_struct;
    test_struct *arr = xcalloc(50, sizeof(test_struct));
    for (int i = 0; i < 50; i++) {
        ASSERT_EQ(arr[i].a, 0);
        ASSERT_EQ(arr[i].b, 0);
        ASSERT_EQ(arr[i].c[0], 0);
    }
    free(arr);
}

/* === xrealloc === */

TEST(xrealloc_grow) {
    char *p = xmalloc(16);
    memset(p, 'B', 16);
    p = xrealloc(p, 64);
    ASSERT_NOT_NULL(p);
    for (int i = 0; i < 16; i++)
        ASSERT_EQ(p[i], 'B');
    free(p);
}

TEST(xrealloc_shrink) {
    char *p = xmalloc(64);
    memset(p, 'C', 64);
    p = xrealloc(p, 8);
    ASSERT_NOT_NULL(p);
    for (int i = 0; i < 8; i++)
        ASSERT_EQ(p[i], 'C');
    free(p);
}

TEST(xrealloc_same_size) {
    char *p = xmalloc(32);
    memset(p, 'D', 32);
    p = xrealloc(p, 32);
    ASSERT_NOT_NULL(p);
    for (int i = 0; i < 32; i++)
        ASSERT_EQ(p[i], 'D');
    free(p);
}

TEST(xrealloc_to_1) {
    char *p = xmalloc(100);
    memset(p, 'E', 100);
    p = xrealloc(p, 1);
    ASSERT_NOT_NULL(p);
    ASSERT_EQ(p[0], 'E');
    free(p);
}

TEST(xrealloc_grow_large) {
    char *p = xmalloc(16);
    memset(p, 'F', 16);
    p = xrealloc(p, 65536);
    ASSERT_NOT_NULL(p);
    for (int i = 0; i < 16; i++)
        ASSERT_EQ(p[i], 'F');
    free(p);
}

TEST(xrealloc_preserves_all_data) {
    int *p = xmalloc(100 * sizeof(int));
    for (int i = 0; i < 100; i++) p[i] = i * 7;
    p = xrealloc(p, 200 * sizeof(int));
    for (int i = 0; i < 100; i++)
        ASSERT_EQ(p[i], i * 7);
    free(p);
}

TEST(xrealloc_multiple_grows) {
    char *p = xmalloc(1);
    *p = 'G';
    for (int size = 2; size <= 1024; size *= 2) {
        p = xrealloc(p, size);
        ASSERT_NOT_NULL(p);
        ASSERT_EQ(p[0], 'G');
    }
    free(p);
}

/* === xstrdup === */

TEST(xstrdup_copies_string) {
    char *orig = "hello world";
    char *copy = xstrdup(orig);
    ASSERT_NOT_NULL(copy);
    ASSERT_STR_EQ(copy, "hello world");
    ASSERT(copy != orig);
    free(copy);
}

TEST(xstrdup_empty_string) {
    char *copy = xstrdup("");
    ASSERT_NOT_NULL(copy);
    ASSERT_STR_EQ(copy, "");
    free(copy);
}

TEST(xstrdup_single_char) {
    char *copy = xstrdup("x");
    ASSERT_STR_EQ(copy, "x");
    free(copy);
}

TEST(xstrdup_long_string) {
    char long_str[1024];
    memset(long_str, 'z', 1023);
    long_str[1023] = '\0';
    char *copy = xstrdup(long_str);
    ASSERT_STR_EQ(copy, long_str);
    free(copy);
}

TEST(xstrdup_special_chars) {
    char *copy = xstrdup("hello\tworld\nnewline\r\0hidden");
    ASSERT_STR_EQ(copy, "hello\tworld\nnewline\r");
    free(copy);
}

TEST(xstrdup_is_independent) {
    char orig[] = "mutable";
    char *copy = xstrdup(orig);
    orig[0] = 'X';
    ASSERT_STR_EQ(copy, "mutable");
    ASSERT_EQ(orig[0], 'X');
    free(copy);
}

TEST(xstrdup_copy_is_mutable) {
    char *copy = xstrdup("immutable_source");
    copy[0] = 'X';
    ASSERT_EQ(copy[0], 'X');
    free(copy);
}

/* === xfree === */

TEST(xfree_null_safe) {
    xfree(NULL); /* should not crash */
}

TEST(xfree_valid_pointer) {
    void *p = xmalloc(32);
    xfree(p);
}

TEST(xfree_after_xmalloc) {
    for (int i = 0; i < 100; i++) {
        void *p = xmalloc(i + 1);
        xfree(p);
    }
}

TEST(xfree_after_xcalloc) {
    void *p = xcalloc(10, 10);
    xfree(p);
}

TEST(xfree_after_xstrdup) {
    char *p = xstrdup("test");
    xfree(p);
}

int main(void) {
    TEST_SUITE("Util Tests");

    RUN(xmalloc_returns_nonnull);
    RUN(xmalloc_size_1);
    RUN(xmalloc_size_8);
    RUN(xmalloc_memory_writable);
    RUN(xmalloc_large_allocation);
    RUN(xmalloc_various_sizes);
    RUN(xmalloc_sequential_allocations);
    RUN(xmalloc_alignment);
    RUN(xcalloc_returns_nonnull);
    RUN(xcalloc_returns_zeroed);
    RUN(xcalloc_correct_size);
    RUN(xcalloc_1x1);
    RUN(xcalloc_large);
    RUN(xcalloc_struct_array);
    RUN(xrealloc_grow);
    RUN(xrealloc_shrink);
    RUN(xrealloc_same_size);
    RUN(xrealloc_to_1);
    RUN(xrealloc_grow_large);
    RUN(xrealloc_preserves_all_data);
    RUN(xrealloc_multiple_grows);
    RUN(xstrdup_copies_string);
    RUN(xstrdup_empty_string);
    RUN(xstrdup_single_char);
    RUN(xstrdup_long_string);
    RUN(xstrdup_special_chars);
    RUN(xstrdup_is_independent);
    RUN(xstrdup_copy_is_mutable);
    RUN(xfree_null_safe);
    RUN(xfree_valid_pointer);
    RUN(xfree_after_xmalloc);
    RUN(xfree_after_xcalloc);
    RUN(xfree_after_xstrdup);

    TEST_REPORT();
}
