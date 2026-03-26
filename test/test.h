/*
 * test.h: minimal unit testing harness
 */

#ifndef TEST_H
#define TEST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ANSI color codes */
#define T_RESET    "\033[0m"
#define T_BOLD     "\033[1m"
#define T_DIM      "\033[2m"
#define T_RED      "\033[31m"
#define T_GREEN    "\033[32m"
#define T_YELLOW   "\033[33m"
#define T_BLUE     "\033[34m"
#define T_MAGENTA  "\033[35m"
#define T_CYAN     "\033[36m"
#define T_BRED     "\033[1;31m"
#define T_BGREEN   "\033[1;32m"
#define T_BYELLOW  "\033[1;33m"
#define T_BCYAN    "\033[1;36m"
#define T_BMAGENTA "\033[1;35m"

static int test_pass_count = 0;
static int test_fail_count = 0;
static int test_total_count = 0;

#define TEST(name)                                                  \
    static void test_##name(void);                                  \
    static void run_test_##name(void) {                             \
        test_total_count++;                                         \
        printf("  " T_DIM "\xE2\x96\x90" T_RESET " %-50s ", #name); \
        test_##name();                                              \
        test_pass_count++;                                          \
        printf(T_BGREEN "\xE2\x96\x88 PASS" T_RESET "\n");          \
    }                                                               \
    static void test_##name(void)

#define ASSERT(cond)                                                                            \
    do {                                                                                        \
        if (!(cond)) {                                                                          \
            printf(T_BRED "\xE2\x96\x88 FAIL" T_RESET "\n"                                      \
                          "    " T_RED "\xE2\x96\xB6 %s:%d: assertion failed: %s" T_RESET "\n", \
                   __FILE__, __LINE__, #cond);                                                  \
            test_fail_count++;                                                                  \
            return;                                                                             \
        }                                                                                       \
    } while (0)

#define ASSERT_EQ(a, b)                                                                            \
    do {                                                                                           \
        long long _a = (long long)(a), _b = (long long)(b);                                        \
        if (_a != _b) {                                                                            \
            printf(T_BRED "\xE2\x96\x88 FAIL" T_RESET "\n"                                         \
                          "    " T_RED "\xE2\x96\xB6 %s:%d: expected %lld, got %lld" T_RESET "\n", \
                   __FILE__, __LINE__, _b, _a);                                                    \
            test_fail_count++;                                                                     \
            return;                                                                                \
        }                                                                                          \
    } while (0)

#define ASSERT_STR_EQ(a, b)                                                                      \
    do {                                                                                         \
        const char *_a = (a), *_b = (b);                                                         \
        if (strcmp(_a, _b) != 0) {                                                               \
            printf(T_BRED "\xE2\x96\x88 FAIL" T_RESET "\n"                                       \
                          "    " T_RED "\xE2\x96\xB6 %s:%d: expected \"%s\", got \"%s\"" T_RESET \
                          "\n",                                                                  \
                   __FILE__, __LINE__, _b, _a);                                                  \
            test_fail_count++;                                                                   \
            return;                                                                              \
        }                                                                                        \
    } while (0)

#define ASSERT_NULL(a)                                                                   \
    do {                                                                                 \
        if ((a) != NULL) {                                                               \
            printf(T_BRED "\xE2\x96\x88 FAIL" T_RESET "\n"                               \
                          "    " T_RED "\xE2\x96\xB6 %s:%d: expected NULL" T_RESET "\n", \
                   __FILE__, __LINE__);                                                  \
            test_fail_count++;                                                           \
            return;                                                                      \
        }                                                                                \
    } while (0)

#define ASSERT_NOT_NULL(a)                                                                   \
    do {                                                                                     \
        if ((a) == NULL) {                                                                   \
            printf(T_BRED "\xE2\x96\x88 FAIL" T_RESET "\n"                                   \
                          "    " T_RED "\xE2\x96\xB6 %s:%d: expected non-NULL" T_RESET "\n", \
                   __FILE__, __LINE__);                                                      \
            test_fail_count++;                                                               \
            return;                                                                          \
        }                                                                                    \
    } while (0)

#define RUN(name) run_test_##name()

#define TEST_SUITE(name)                                                               \
    printf("\n" T_BCYAN " \xE2\x96\x93\xE2\x96\x93\xE2\x96\x93 " T_RESET T_BOLD        \
           " %s " T_RESET T_BCYAN "\xE2\x96\x93\xE2\x96\x93\xE2\x96\x93" T_RESET "\n", \
           name)

#define TEST_COUNTS_FILE "/tmp/iftop_test_counts"

#define TEST_REPORT()                                                                             \
    do {                                                                                          \
        printf("\n" T_DIM " \xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94" \
                          "\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80"                  \
               "\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94" \
               "\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80"                                         \
               "\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94" \
               "\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80"                                         \
               "\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94" \
               "\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80"                                         \
               "\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94" \
               "\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80"                                         \
               "\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94" \
               "\x80\xE2\x94\x80\xE2\x94\x80\xE2\x94\x80" T_RESET "\n");                          \
        if (test_fail_count == 0) {                                                               \
            printf(T_BGREEN " \xE2\x96\x88\xE2\x96\x88 SYSTEM INTEGRITY VERIFIED" T_RESET T_GREEN \
                            " // %d tests passed // 0 breaches detected" T_RESET "\n",            \
                   test_pass_count);                                                              \
        } else {                                                                                  \
            printf(T_BRED " \xE2\x96\x88\xE2\x96\x88 INTEGRITY BREACH DETECTED" T_RESET T_RED     \
                          " // %d passed, %d FAILED, %d total" T_RESET "\n",                      \
                   test_pass_count, test_fail_count, test_total_count);                           \
        }                                                                                         \
        {                                                                                         \
            FILE *_cf = fopen(TEST_COUNTS_FILE, "a");                                             \
            if (_cf) {                                                                            \
                fprintf(_cf, "%d %d %d\n", test_pass_count, test_fail_count, test_total_count);   \
                fclose(_cf);                                                                      \
            }                                                                                     \
        }                                                                                         \
        return test_fail_count > 0 ? 1 : 0;                                                       \
    } while (0)

#endif /* TEST_H */
