/*
 * test.h: minimal unit testing harness
 */

#ifndef TEST_H
#define TEST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int test_pass_count = 0;
static int test_fail_count = 0;
static int test_total_count = 0;

#define TEST(name) \
    static void test_##name(void); \
    static void run_test_##name(void) { \
        test_total_count++; \
        printf("  %-50s ", #name); \
        test_##name(); \
        test_pass_count++; \
        printf("PASS\n"); \
    } \
    static void test_##name(void)

#define ASSERT(cond) do { \
    if (!(cond)) { \
        printf("FAIL\n    %s:%d: assertion failed: %s\n", __FILE__, __LINE__, #cond); \
        test_fail_count++; \
        return; \
    } \
} while(0)

#define ASSERT_EQ(a, b) do { \
    long long _a = (long long)(a), _b = (long long)(b); \
    if (_a != _b) { \
        printf("FAIL\n    %s:%d: expected %lld, got %lld\n", __FILE__, __LINE__, _b, _a); \
        test_fail_count++; \
        return; \
    } \
} while(0)

#define ASSERT_STR_EQ(a, b) do { \
    const char *_a = (a), *_b = (b); \
    if (strcmp(_a, _b) != 0) { \
        printf("FAIL\n    %s:%d: expected \"%s\", got \"%s\"\n", __FILE__, __LINE__, _b, _a); \
        test_fail_count++; \
        return; \
    } \
} while(0)

#define ASSERT_NULL(a) do { \
    if ((a) != NULL) { \
        printf("FAIL\n    %s:%d: expected NULL\n", __FILE__, __LINE__); \
        test_fail_count++; \
        return; \
    } \
} while(0)

#define ASSERT_NOT_NULL(a) do { \
    if ((a) == NULL) { \
        printf("FAIL\n    %s:%d: expected non-NULL\n", __FILE__, __LINE__); \
        test_fail_count++; \
        return; \
    } \
} while(0)

#define RUN(name) run_test_##name()

#define TEST_SUITE(name) \
    printf("\n=== %s ===\n", name)

#define TEST_REPORT() do { \
    printf("\n--- Results: %d passed, %d failed, %d total ---\n", \
           test_pass_count, test_fail_count, test_total_count); \
    return test_fail_count > 0 ? 1 : 0; \
} while(0)

#endif /* TEST_H */
