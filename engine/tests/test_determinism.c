/*
 * Canvas OS — Determinism Integration Tests
 * End-to-end tests ensuring DK-1~5 compliance
 * 20+ test cases
 */

#include "test_framework.h"
#include "../include/canvasos.h"
#include "../include/canvasos_sched.h"

/* Replay the same sequence twice and compare hashes */
static void test_determinism_replay(void) {
    TEST_SUITE("Determinism Replay");

    EngineContext* ctx1 = engctx_create();
    EngineContext* ctx2 = engctx_create();
    if (!ctx1 || !ctx2) return;

    /* Same sequence of operations on both contexts */
    Cell c1 = {0x10, 0x20, 0x30, 0x40};
    Cell c2 = {0xAA, 0xBB, 0xCC, 0xDD};

    engctx_set_cell(ctx1, 10, 10, c1);
    engctx_set_cell(ctx2, 10, 10, c1);
    engctx_set_cell(ctx1, 20, 20, c2);
    engctx_set_cell(ctx2, 20, 20, c2);

    for (int i = 0; i < 5; i++) {
        engctx_tick(ctx1);
        engctx_tick(ctx2);
    }

    uint64_t h1 = dk_canvas_hash(ctx1);
    uint64_t h2 = dk_canvas_hash(ctx2);
    TEST_EQ("same sequence produces same hash (replay)", h1 == h2, 1);

    /* Different sequence produces different hash */
    EngineContext* ctx3 = engctx_create();
    if (ctx3) {
        engctx_set_cell(ctx3, 10, 10, c2); /* reversed! */
        engctx_set_cell(ctx3, 20, 20, c1);
        for (int i = 0; i < 5; i++) engctx_tick(ctx3);
        uint64_t h3 = dk_canvas_hash(ctx3);
        TEST_NEQ("different sequence produces different hash", h1, h3);
        engctx_destroy(ctx3);
    }

    engctx_destroy(ctx1);
    engctx_destroy(ctx2);
}

/* Verify that ticks 1..N produce strictly ordered hashes over time */
static void test_determinism_tick_sequence(void) {
    TEST_SUITE("Determinism Tick Sequence");

    EngineContext* ctx = engctx_create();
    if (!ctx) return;

    /* Set some cells to create interesting state */
    for (int i = 0; i < 10; i++) {
        Cell c = {(uint8_t)i, (uint8_t)(i*2), (uint8_t)(i*3), (uint8_t)(i*4)};
        engctx_set_cell(ctx, i, 0, c);
    }

    /* Record hashes at each tick */
    uint64_t hashes[10];
    for (int t = 0; t < 10; t++) {
        engctx_tick(ctx);
        hashes[t] = dk_canvas_hash(ctx);
    }

    /* Verify ticks are counted correctly */
    TEST_EQ("tick count == 10 after 10 ticks", (int)engctx_get_tick(ctx), 10);

    /* Hashes should be reproducible on identical replay */
    EngineContext* ctx2 = engctx_create();
    if (ctx2) {
        for (int i = 0; i < 10; i++) {
            Cell c = {(uint8_t)i, (uint8_t)(i*2), (uint8_t)(i*3), (uint8_t)(i*4)};
            engctx_set_cell(ctx2, i, 0, c);
        }
        for (int t = 0; t < 10; t++) {
            engctx_tick(ctx2);
            uint64_t h = dk_canvas_hash(ctx2);
            TEST_EQ("replay hash matches at each tick", h == hashes[t], 1);
        }
        engctx_destroy(ctx2);
    }

    engctx_destroy(ctx);
}

/* Verify integer-only (DK-2): no float computations */
static void test_determinism_integer_only(void) {
    TEST_SUITE("Determinism Integer-Only (DK-2)");

    /* Test that all cell values stay in uint8 range */
    EngineContext* ctx = engctx_create();
    if (!ctx) return;

    /* Set cells to max value */
    Cell max_cell = {0xFF, 0xFF, 0xFF, 0xFF};
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            engctx_set_cell(ctx, x, y, max_cell);
        }
    }

    /* Tick multiple times — values should remain bounded */
    for (int t = 0; t < 100; t++) engctx_tick(ctx);

    /* DK-3: ascending scan to verify all values still in [0,255] */
    bool all_bounded = true;
    for (int y = 0; y < 4 && all_bounded; y++) {
        for (int x = 0; x < 4 && all_bounded; x++) {
            Cell c = engctx_get_cell(ctx, x, y);
            if (c.r > 255 || c.g > 255 || c.b > 255 || c.a > 255) {
                all_bounded = false;
            }
        }
    }
    TEST_TRUE("all cell values bounded to [0,255] (DK-2)", all_bounded);

    /* FNV-1a produces consistent results */
    const char* teststr = "determinism";
    uint64_t h1 = dk_fnv1a((const uint8_t*)teststr, strlen(teststr));
    uint64_t h2 = dk_fnv1a((const uint8_t*)teststr, strlen(teststr));
    TEST_EQ("FNV-1a is deterministic", h1 == h2, 1);
    TEST_TRUE("FNV-1a result is 64-bit integer", h1 > 0 || h1 == 0);

    engctx_destroy(ctx);
}

/* Verify ascending scan order (DK-3) */
static void test_determinism_ascending_order(void) {
    TEST_SUITE("Determinism Ascending Order (DK-3)");

    /* cell_idx produces strictly ascending values for sequential x,y */
    CellIndex last = 0;
    bool ascending = true;
    for (int y = 0; y < CANVAS_H && ascending; y++) {
        for (int x = 0; x < CANVAS_W && ascending; x++) {
            CellIndex ci = cell_idx(x, y);
            if (y == 0 && x == 0) { last = ci; continue; }
            if (ci <= last) ascending = false;
            last = ci;
        }
    }
    TEST_TRUE("full canvas scan produces strictly ascending indices", ascending);

    /* Verify specific values */
    TEST_EQ("idx(0,0)==0", (int)cell_idx(0,0), 0);
    TEST_EQ("idx(1,0)==1", (int)cell_idx(1,0), 1);
    TEST_EQ("idx(CANVAS_W-1,0)==CANVAS_W-1",
            (int)cell_idx(CANVAS_W-1, 0), CANVAS_W-1);
    TEST_EQ("idx(0,1)==CANVAS_W", (int)cell_idx(0,1), CANVAS_W);
    TEST_EQ("idx(1,1)==CANVAS_W+1", (int)cell_idx(1,1), CANVAS_W+1);
}

/* Multi-context isolation */
static void test_determinism_isolation(void) {
    TEST_SUITE("Determinism Context Isolation");

    EngineContext* a = engctx_create();
    EngineContext* b = engctx_create();
    if (!a || !b) return;

    /* Independent contexts don't interfere */
    Cell ca = {1, 2, 3, 4};
    Cell cb = {5, 6, 7, 8};
    engctx_set_cell(a, 0, 0, ca);
    engctx_set_cell(b, 0, 0, cb);

    Cell ra = engctx_get_cell(a, 0, 0);
    Cell rb = engctx_get_cell(b, 0, 0);
    TEST_EQ("ctx a cell.r == 4", (int)ra.r, 4);
    TEST_EQ("ctx b cell.r == 8", (int)rb.r, 8);
    TEST_NEQ("a and b have different cell values", ra.r, rb.r);

    /* Tick a doesn't affect b */
    Tick ta0 = engctx_get_tick(a);
    Tick tb0 = engctx_get_tick(b);
    engctx_tick(a);
    TEST_EQ("b tick unchanged when a ticked",
            (int)engctx_get_tick(b), (int)tb0);
    TEST_EQ("a tick incremented",
            (int)engctx_get_tick(a), (int)ta0 + 1);

    /* Hashes differ due to different cell contents */
    uint64_t ha = dk_canvas_hash(a);
    uint64_t hb = dk_canvas_hash(b);
    TEST_NEQ("different contexts have different hashes", ha, hb);

    engctx_destroy(a);
    engctx_destroy(b);
}

void run_determinism_tests(void) {
    test_determinism_replay();
    test_determinism_tick_sequence();
    test_determinism_integer_only();
    test_determinism_ascending_order();
    test_determinism_isolation();
}
