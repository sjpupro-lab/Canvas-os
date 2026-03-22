/*
 * Canvas OS — DK-1~5 Determinism Kernel Tests
 * 40+ test cases
 */

#include "test_framework.h"
#include "../include/canvasos.h"
#include "../include/canvasos_sched.h"
#include "../include/canvasos_ruletable.h"

/* ─── DK-1: Tick Boundary ───────────────────────────────── */
static void test_dk1(void) {
    TEST_SUITE("DK-1 Tick Boundary");

    EngineContext* ctx = engctx_create();
    TEST_NOT_NULL("ctx created", ctx);
    if (!ctx) return;

    TEST_EQ("tick starts at 0", (int)engctx_get_tick(ctx), 0);

    engctx_tick(ctx);
    TEST_EQ("tick == 1 after first tick", (int)engctx_get_tick(ctx), 1);

    engctx_tick(ctx);
    engctx_tick(ctx);
    TEST_EQ("tick == 3 after 3 ticks", (int)engctx_get_tick(ctx), 3);

    Tick t = engctx_tick_n(ctx, 10);
    TEST_EQ("tick_n(10) returns tick 13", (int)t, 13);
    TEST_EQ("get_tick matches tick_n return", (int)engctx_get_tick(ctx), 13);

    /* DK-1: tick is monotonically increasing */
    Tick before = engctx_get_tick(ctx);
    engctx_tick(ctx);
    Tick after = engctx_get_tick(ctx);
    TEST_TRUE("tick monotonically increases", after > before);

    /* tick_n(0) should not change tick */
    Tick t0 = engctx_get_tick(ctx);
    engctx_tick_n(ctx, 0);
    TEST_EQ("tick_n(0) no change", (int)engctx_get_tick(ctx), (int)t0);

    /* tick_n(1) == tick + 1 */
    Tick t1 = engctx_get_tick(ctx);
    engctx_tick_n(ctx, 1);
    TEST_EQ("tick_n(1) increments by 1", (int)engctx_get_tick(ctx), (int)t1 + 1);

    /* Large n */
    engctx_tick_n(ctx, 100);
    TEST_EQ("tick after 100 more ticks", (int)engctx_get_tick(ctx), (int)t1 + 101);

    engctx_destroy(ctx);
}

/* ─── DK-2: Integer-only arithmetic ─────────────────────── */
static void test_dk2(void) {
    TEST_SUITE("DK-2 Integer-only Arithmetic");

    /* dk_clamp */
    TEST_EQ("clamp(5,0,10)==5",  dk_clamp(5, 0, 10), 5);
    TEST_EQ("clamp(-1,0,10)==0", dk_clamp(-1, 0, 10), 0);
    TEST_EQ("clamp(11,0,10)==10",dk_clamp(11, 0, 10), 10);
    TEST_EQ("clamp(0,0,0)==0",   dk_clamp(0, 0, 0), 0);
    TEST_EQ("clamp(INT_MIN,0,255)==0", dk_clamp(-2147483647, 0, 255), 0);
    TEST_EQ("clamp(INT_MAX,0,255)==255", dk_clamp(2147483647, 0, 255), 255);

    /* dk_clamp8 */
    TEST_EQ("clamp8(0)==0",   (int)dk_clamp8(0), 0);
    TEST_EQ("clamp8(255)==255", (int)dk_clamp8(255), 255);
    TEST_EQ("clamp8(256)==255", (int)dk_clamp8(256), 255);
    TEST_EQ("clamp8(-1)==0",  (int)dk_clamp8(-1), 0);
    TEST_EQ("clamp8(127)==127", (int)dk_clamp8(127), 127);
    TEST_EQ("clamp8(128)==128", (int)dk_clamp8(128), 128);

    /* Cell fields are uint8_t — no float */
    Cell c = {0xFF, 0x80, 0x40, 0x00};
    TEST_EQ("cell.a uint8", (int)c.a, 255);
    TEST_EQ("cell.b uint8", (int)c.b, 128);
    TEST_EQ("cell.g uint8", (int)c.g, 64);
    TEST_EQ("cell.r uint8", (int)c.r, 0);
}

/* ─── DK-3: Cell index ascending order ──────────────────── */
static void test_dk3(void) {
    TEST_SUITE("DK-3 Cell Index Ascending Order");

    /* cell_idx(0,0) = 0 */
    TEST_EQ("cell_idx(0,0)==0",   (int)cell_idx(0,0), 0);
    /* cell_idx(1,0) = 1 */
    TEST_EQ("cell_idx(1,0)==1",   (int)cell_idx(1,0), 1);
    /* cell_idx(0,1) = CANVAS_W */
    TEST_EQ("cell_idx(0,1)==W",   (int)cell_idx(0,1), CANVAS_W);
    /* cell_idx(W-1,H-1) = last */
    TEST_EQ("cell_idx(W-1,H-1)",  (int)cell_idx(CANVAS_W-1, CANVAS_H-1),
            CANVAS_W*CANVAS_H - 1);

    /* Ascending: idx(x,y) < idx(x+1,y) */
    CellIndex a = cell_idx(5, 3);
    CellIndex b = cell_idx(6, 3);
    TEST_TRUE("idx(x,y) < idx(x+1,y)", a < b);

    /* Row ascending */
    CellIndex r0 = cell_idx(0, 0);
    CellIndex r1 = cell_idx(0, 1);
    TEST_TRUE("idx(0,0) < idx(0,1)", r0 < r1);

    /* Sequential scan from 0..CANVAS_CELLS-1 is ascending */
    CellIndex prev = 0;
    bool ascending = true;
    for (int y = 0; y < 4; y++) {
        for (int x = 0; x < 4; x++) {
            CellIndex ci = cell_idx(x, y);
            if (ci < prev && !(y == 0 && x == 0)) {
                ascending = false;
            }
            prev = ci;
        }
    }
    TEST_TRUE("4x4 scan is ascending", ascending);

    /* CANVAS dimensions */
    TEST_EQ("CANVAS_W==1024", CANVAS_W, 1024);
    TEST_EQ("CANVAS_H==1024", CANVAS_H, 1024);
    TEST_EQ("CANVAS_CELLS==1M", CANVAS_CELLS, 1024*1024);
}

/* ─── DK-4: Clamp operations ────────────────────────────── */
static void test_dk4(void) {
    TEST_SUITE("DK-4 Clamp Operations");

    TEST_EQ("clamp boundary lo",  dk_clamp(0, 0, 255), 0);
    TEST_EQ("clamp boundary hi",  dk_clamp(255, 0, 255), 255);
    TEST_EQ("clamp mid value",    dk_clamp(128, 0, 255), 128);
    TEST_EQ("clamp below lo",     dk_clamp(-100, 0, 255), 0);
    TEST_EQ("clamp above hi",     dk_clamp(300, 0, 255), 255);
    TEST_EQ("clamp negative range", dk_clamp(-5, -10, -1), -5);
    TEST_EQ("clamp equal lo hi",  dk_clamp(7, 7, 7), 7);
    TEST_EQ("clamp8 overflow",    (int)dk_clamp8(1000), 255);
    TEST_EQ("clamp8 underflow",   (int)dk_clamp8(-1000), 0);

    /* Energy application: simulating energy update */
    int32_t energy = 255;
    energy = dk_clamp(energy - 1, 0, 255);
    TEST_EQ("energy decay clamped", energy, 254);
    energy = dk_clamp(energy + 10, 0, 255);
    TEST_EQ("energy boost clamped", energy, 255);
    /* Would overflow without clamp */
    energy = dk_clamp(300, 0, 255);
    TEST_EQ("energy overflow clamp", energy, 255);
}

/* ─── DK-5: FNV-1a hash ─────────────────────────────────── */
static void test_dk5(void) {
    TEST_SUITE("DK-5 FNV-1a Hash");

    /* Known FNV-1a 64-bit test vectors */
    /* Empty string */
    uint64_t h_empty = dk_fnv1a((const uint8_t*)"", 0);
    TEST_EQ("FNV-1a empty == offset basis",
            h_empty == UINT64_C(14695981039346656037), 1);

    /* "a" */
    uint64_t h_a = dk_fnv1a((const uint8_t*)"a", 1);
    TEST_TRUE("FNV-1a 'a' non-zero", h_a != 0);
    TEST_NEQ("FNV-1a 'a' != empty", h_a, h_empty);

    /* "b" */
    uint64_t h_b = dk_fnv1a((const uint8_t*)"b", 1);
    TEST_NEQ("FNV-1a 'a' != 'b'", h_a, h_b);

    /* Determinism: same input => same output */
    uint64_t h1 = dk_fnv1a((const uint8_t*)"canvas", 6);
    uint64_t h2 = dk_fnv1a((const uint8_t*)"canvas", 6);
    TEST_EQ("FNV-1a deterministic", h1 == h2, 1);

    /* Different lengths differ */
    uint64_t h_can  = dk_fnv1a((const uint8_t*)"canvas", 6);
    uint64_t h_canv = dk_fnv1a((const uint8_t*)"canvas_", 7);
    TEST_NEQ("FNV-1a length-sensitive", h_can, h_canv);

    /* Null bytes matter */
    uint8_t z1[4] = {0, 0, 0, 0};
    uint8_t z2[4] = {0, 0, 0, 1};
    uint64_t hz1 = dk_fnv1a(z1, 4);
    uint64_t hz2 = dk_fnv1a(z2, 4);
    TEST_NEQ("FNV-1a null-byte sensitive", hz1, hz2);

    /* Canvas hash determinism */
    EngineContext* ctx = engctx_create();
    TEST_NOT_NULL("ctx for hash", ctx);
    if (ctx) {
        uint64_t ch1 = dk_canvas_hash(ctx);
        uint64_t ch2 = dk_canvas_hash(ctx);
        TEST_EQ("canvas hash deterministic", ch1 == ch2, 1);

        /* Hash changes after cell modification */
        Cell c = {1, 2, 3, 4};
        engctx_set_cell(ctx, 0, 0, c);
        uint64_t ch3 = dk_canvas_hash(ctx);
        TEST_NEQ("hash changes after cell write", ch1, ch3);

        /* Hash consistent with known FNV-1a algorithm */
        uint64_t manual_hash = dk_fnv1a(
            (const uint8_t*)engctx_canvas_ptr(ctx),
            sizeof(Cell) * CANVAS_CELLS);
        TEST_EQ("canvas hash == manual FNV-1a",
                ch3 == manual_hash, 1);

        /* After tick, cell data is unchanged (energy decay is in separate array),
         * so the canvas hash remains the same within the same canvas state. */
        uint64_t hbefore = dk_canvas_hash(ctx);
        engctx_tick(ctx);
        uint64_t hafter  = dk_canvas_hash(ctx);
        /* Energy is in a separate array — cell data (what is hashed) doesn't change */
        TEST_EQ("canvas hash stable across tick (cell data unchanged)", hbefore == hafter, 1);
        /* Hash is stable: calling twice gives same value */
        TEST_EQ("hash is idempotent", dk_canvas_hash(ctx) == hafter, 1);

        engctx_destroy(ctx);
    }

    /* FNV-1a: longer message produces valid hash */
    uint8_t big[1024];
    for (int i = 0; i < 1024; i++) big[i] = (uint8_t)i;
    uint64_t h_big = dk_fnv1a(big, 1024);
    TEST_TRUE("FNV-1a 1KB non-zero", h_big != 0);

    /* FNV-1a: single byte range */
    uint64_t hvals[256];
    bool all_unique = true;
    for (int i = 0; i < 256; i++) {
        uint8_t b = (uint8_t)i;
        hvals[i] = dk_fnv1a(&b, 1);
    }
    for (int i = 0; i < 256 && all_unique; i++) {
        for (int j = i + 1; j < 256 && all_unique; j++) {
            if (hvals[i] == hvals[j]) all_unique = false;
        }
    }
    TEST_TRUE("FNV-1a all 256 single-byte hashes unique", all_unique);
}

void run_dk_tests(void) {
    test_dk1();
    test_dk2();
    test_dk3();
    test_dk4();
    test_dk5();
}
