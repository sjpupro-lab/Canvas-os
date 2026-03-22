/*
 * Canvas OS — Test Framework
 * Lightweight, no external dependencies
 */

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

/* ─── Globals (defined in test_main.c) ───────────────────── */
extern int _tests_run;
extern int _tests_passed;
extern int _tests_failed;

/* ─── Macros ──────────────────────────────────────────────── */

#define TEST_SUITE(name) \
    printf("\n[SUITE] %s\n", (name))

#define TEST(desc, expr) \
    do { \
        _tests_run++; \
        if (expr) { \
            _tests_passed++; \
            printf("  [PASS] %s\n", (desc)); \
        } else { \
            _tests_failed++; \
            printf("  [FAIL] %s  (line %d)\n", (desc), __LINE__); \
        } \
    } while(0)

#define TEST_EQ(desc, a, b) \
    TEST(desc, (a) == (b))

#define TEST_NEQ(desc, a, b) \
    TEST(desc, (a) != (b))

#define TEST_STR(desc, a, b) \
    TEST(desc, strcmp((a),(b)) == 0)

#define TEST_NOT_NULL(desc, p) \
    TEST(desc, (p) != NULL)

#define TEST_NULL(desc, p) \
    TEST(desc, (p) == NULL)

#define TEST_TRUE(desc, expr) \
    TEST(desc, !!(expr))

#define TEST_FALSE(desc, expr) \
    TEST(desc, !(expr))

/* ─── Summary ─────────────────────────────────────────────── */

static inline int test_summary(void) {
    printf("\n========================================\n");
    printf("  TOTAL : %d\n", _tests_run);
    printf("  PASS  : %d\n", _tests_passed);
    printf("  FAIL  : %d\n", _tests_failed);
    printf("========================================\n");
    if (_tests_failed == 0) {
        printf("  ALL %d TESTS PASSED\n", _tests_run);
    } else {
        printf("  %d FAILURE(S)\n", _tests_failed);
    }
    return (_tests_failed == 0) ? 0 : 1;
}
