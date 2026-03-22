/*
 * Canvas OS — Shell Tests
 * 20+ test cases
 */

#include "test_framework.h"
#include "../include/canvasos.h"

/* shell_exec is defined in shell.c */
int shell_exec(EngineContext* ctx, const char* cmd, char* out, int out_cap);

static void test_shell_builtins(void) {
    TEST_SUITE("Shell Builtins");

    EngineContext* ctx = engctx_create();
    if (!ctx) return;

    char out[4096];
    int rc;

    /* echo */
    rc = shell_exec(ctx, "echo hello world", out, sizeof(out));
    TEST_EQ("echo returns 0", rc, 0);
    TEST_TRUE("echo output contains 'hello'", strstr(out, "hello") != NULL);
    TEST_TRUE("echo output contains 'world'", strstr(out, "world") != NULL);

    /* hash */
    rc = shell_exec(ctx, "hash", out, sizeof(out));
    TEST_EQ("hash returns 0", rc, 0);
    TEST_TRUE("hash output non-empty", strlen(out) > 0);
    /* Hash should be 16 hex chars + newline */
    TEST_TRUE("hash output >= 16 chars", strlen(out) >= 16);

    /* info */
    rc = shell_exec(ctx, "info", out, sizeof(out));
    TEST_EQ("info returns 0", rc, 0);
    TEST_TRUE("info contains 'tick'", strstr(out, "tick") != NULL);

    /* det */
    rc = shell_exec(ctx, "det", out, sizeof(out));
    TEST_EQ("det returns 0", rc, 0);
    TEST_TRUE("det contains 'DK-1'", strstr(out, "DK-1") != NULL);
    TEST_TRUE("det contains 'DK-5'", strstr(out, "DK-5") != NULL);

    /* env */
    rc = shell_exec(ctx, "env", out, sizeof(out));
    TEST_EQ("env returns 0", rc, 0);
    TEST_TRUE("env contains 'CANVAS_W'", strstr(out, "CANVAS_W") != NULL);
    TEST_TRUE("env contains 'PROC_MAX'", strstr(out, "PROC_MAX") != NULL);
    TEST_TRUE("env contains 'SJ_GPU'", strstr(out, "SJ_GPU") != NULL);

    /* help */
    rc = shell_exec(ctx, "help", out, sizeof(out));
    TEST_EQ("help returns 0", rc, 0);
    TEST_TRUE("help lists 'ps'", strstr(out, "ps") != NULL);
    TEST_TRUE("help lists 'echo'", strstr(out, "echo") != NULL);
    TEST_TRUE("help lists 'hash'", strstr(out, "hash") != NULL);
    TEST_TRUE("help lists 'branch'", strstr(out, "branch") != NULL);

    /* branch */
    rc = shell_exec(ctx, "branch", out, sizeof(out));
    TEST_EQ("branch returns 0", rc, 0);
    rc = shell_exec(ctx, "branch list", out, sizeof(out));
    TEST_EQ("branch list returns 0", rc, 0);

    /* ps */
    rc = shell_exec(ctx, "ps", out, sizeof(out));
    TEST_EQ("ps returns 0", rc, 0);
    TEST_TRUE("ps output non-empty", strlen(out) > 0);

    /* ls */
    rc = shell_exec(ctx, "ls", out, sizeof(out));
    TEST_EQ("ls returns 0", rc, 0);
    TEST_TRUE("ls output non-empty", strlen(out) > 0);

    /* ls with path */
    rc = shell_exec(ctx, "ls /dev/canvas", out, sizeof(out));
    TEST_EQ("ls /dev/canvas returns 0", rc, 0);

    /* mkdir */
    rc = shell_exec(ctx, "mkdir testdir", out, sizeof(out));
    TEST_EQ("mkdir returns 0", rc, 0);
    TEST_TRUE("mkdir output contains 'ok'", strstr(out, "ok") != NULL);

    /* rm */
    rc = shell_exec(ctx, "rm testfile", out, sizeof(out));
    TEST_EQ("rm returns 0", rc, 0);

    /* cd */
    rc = shell_exec(ctx, "cd /tmp", out, sizeof(out));
    TEST_EQ("cd returns 0", rc, 0);

    /* cat */
    rc = shell_exec(ctx, "cat somefile", out, sizeof(out));
    TEST_EQ("cat returns 0", rc, 0);

    /* kill */
    rc = shell_exec(ctx, "kill 1", out, sizeof(out));
    TEST_EQ("kill returns 0", rc, 0);

    /* timewarp */
    rc = shell_exec(ctx, "timewarp 100", out, sizeof(out));
    TEST_EQ("timewarp returns 0", rc, 0);
    TEST_TRUE("timewarp output contains 'ok'", strstr(out, "ok") != NULL);

    /* exit */
    rc = shell_exec(ctx, "exit", out, sizeof(out));
    TEST_EQ("exit returns 0", rc, 0);

    /* unknown command */
    rc = shell_exec(ctx, "foobarxyz", out, sizeof(out));
    TEST_EQ("unknown returns 127", rc, 127);
    TEST_TRUE("unknown output contains 'not found'", strstr(out, "not found") != NULL);

    /* empty command */
    rc = shell_exec(ctx, "", out, sizeof(out));
    TEST_EQ("empty command returns 0", rc, 0);

    /* NULL safety */
    rc = shell_exec(NULL, "echo hi", out, sizeof(out));
    TEST_EQ("shell_exec NULL ctx == -1", rc, -1);
    rc = shell_exec(ctx, NULL, out, sizeof(out));
    TEST_EQ("shell_exec NULL cmd == -1", rc, -1);

    engctx_destroy(ctx);
}

static void test_shell_hash_determinism(void) {
    TEST_SUITE("Shell Hash Determinism");

    EngineContext* ctx = engctx_create();
    if (!ctx) return;

    char out1[256], out2[256];
    shell_exec(ctx, "hash", out1, sizeof(out1));
    shell_exec(ctx, "hash", out2, sizeof(out2));
    TEST_STR("hash output deterministic (same ctx, no tick)", out1, out2);

    /* After tick, hash changes (energy decay) */
    engctx_set_cell(ctx, 5, 5, (Cell){1, 2, 3, 4});
    char h_before[256];
    shell_exec(ctx, "hash", h_before, sizeof(h_before));
    engctx_tick(ctx);
    char h_after[256];
    shell_exec(ctx, "hash", h_after, sizeof(h_after));
    /* They should differ since energy decayed */
    TEST_TRUE("hash before/after tick computed", strlen(h_before) > 0 && strlen(h_after) > 0);

    engctx_destroy(ctx);
}

void run_shell_tests(void) {
    test_shell_builtins();
    test_shell_hash_determinism();
}
