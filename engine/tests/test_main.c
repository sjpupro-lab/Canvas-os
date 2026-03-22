/*
 * Canvas OS — Test Runner
 * Runs all 347 determinism tests
 * Expected: 0 failures
 */

#include "test_framework.h"
#include <stdio.h>

/* ─── Global test counters (shared across all TUs) ───────── */
int _tests_run    = 0;
int _tests_passed = 0;
int _tests_failed = 0;

/* Test suite functions */
void run_dk_tests(void);
void run_engine_tests(void);
void run_whitehole_tests(void);
void run_blackhole_tests(void);
void run_rulecell_tests(void);
void run_scheduler_tests(void);
void run_shell_tests(void);
void run_gpu_tests(void);
void run_determinism_tests(void);

int main(void) {
    printf("════════════════════════════════════════\n");
    printf("  Canvas OS — Full Test Suite\n");
    printf("  DK-1~5 Determinism Kernel Tests\n");
    printf("════════════════════════════════════════\n");

    run_dk_tests();
    run_engine_tests();
    run_whitehole_tests();
    run_blackhole_tests();
    run_rulecell_tests();
    run_scheduler_tests();
    run_shell_tests();
    run_gpu_tests();
    run_determinism_tests();

    return test_summary();
}
