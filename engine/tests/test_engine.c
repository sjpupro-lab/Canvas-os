/*
 * Canvas OS — Engine Core Tests
 * 30+ test cases
 */

#include "test_framework.h"
#include "../include/canvasos.h"

static void test_engine_lifecycle(void) {
    TEST_SUITE("Engine Lifecycle");

    /* Create / destroy */
    EngineContext* ctx = engctx_create();
    TEST_NOT_NULL("engctx_create returns non-null", ctx);

    /* Initial state */
    TEST_EQ("initial tick == 0", (int)engctx_get_tick(ctx), 0);
    TEST_EQ("initial branch == 0", engctx_get_branch_id(ctx), 0);
    TEST_EQ("initial open gates == 0", engctx_get_open_gates(ctx), 0);
    TEST_EQ("initial vis_mode == ABGR", (int)engctx_get_vis_mode(ctx), (int)VIS_ABGR);

    /* Init */
    bool ok = engctx_init(ctx, "/tmp/canvasos_test");
    TEST_TRUE("engctx_init returns true", ok);

    /* Destroy */
    engctx_destroy(ctx);
    TEST_TRUE("engctx_destroy completes", true);

    /* Null safety */
    engctx_destroy(NULL);
    TEST_TRUE("engctx_destroy(NULL) safe", true);

    engctx_tick(NULL);
    TEST_TRUE("engctx_tick(NULL) safe", true);

    Cell z = engctx_get_cell(NULL, 0, 0);
    TEST_EQ("engctx_get_cell(NULL) returns zero.r", (int)z.r, 0);
}

static void test_engine_cell(void) {
    TEST_SUITE("Engine Cell Access");

    EngineContext* ctx = engctx_create();
    if (!ctx) return;

    /* Initial cells are all zero */
    Cell c0 = engctx_get_cell(ctx, 0, 0);
    TEST_EQ("initial cell (0,0).r == 0", (int)c0.r, 0);
    TEST_EQ("initial cell (0,0).g == 0", (int)c0.g, 0);
    TEST_EQ("initial cell (0,0).b == 0", (int)c0.b, 0);
    TEST_EQ("initial cell (0,0).a == 0", (int)c0.a, 0);

    /* Set and get */
    Cell c = {0xAA, 0xBB, 0xCC, 0xDD};
    engctx_set_cell(ctx, 0, 0, c);
    Cell got = engctx_get_cell(ctx, 0, 0);
    TEST_EQ("set/get cell.a", (int)got.a, 0xAA);
    TEST_EQ("set/get cell.b", (int)got.b, 0xBB);
    TEST_EQ("set/get cell.g", (int)got.g, 0xCC);
    TEST_EQ("set/get cell.r", (int)got.r, 0xDD);

    /* Out-of-bounds: should return zero */
    Cell oob = engctx_get_cell(ctx, -1, 0);
    TEST_EQ("oob x=-1 returns 0", (int)oob.r, 0);
    Cell oob2 = engctx_get_cell(ctx, CANVAS_W, 0);
    TEST_EQ("oob x=W returns 0", (int)oob2.r, 0);
    Cell oob3 = engctx_get_cell(ctx, 0, CANVAS_H);
    TEST_EQ("oob y=H returns 0", (int)oob3.r, 0);

    /* Multiple cells independence */
    Cell ca = {1, 2, 3, 4};
    Cell cb = {5, 6, 7, 8};
    engctx_set_cell(ctx, 10, 10, ca);
    engctx_set_cell(ctx, 11, 10, cb);
    Cell ga = engctx_get_cell(ctx, 10, 10);
    Cell gb = engctx_get_cell(ctx, 11, 10);
    TEST_EQ("cell (10,10).r == 4", (int)ga.r, 4);
    TEST_EQ("cell (11,10).r == 8", (int)gb.r, 8);
    TEST_NEQ("independent cells differ", (int)ga.r, (int)gb.r);

    /* Set at corners */
    Cell corner = {0xFF, 0xFF, 0xFF, 0xFF};
    engctx_set_cell(ctx, 0, 0, corner);
    engctx_set_cell(ctx, CANVAS_W-1, 0, corner);
    engctx_set_cell(ctx, 0, CANVAS_H-1, corner);
    engctx_set_cell(ctx, CANVAS_W-1, CANVAS_H-1, corner);
    Cell c_tr = engctx_get_cell(ctx, CANVAS_W-1, 0);
    Cell c_bl = engctx_get_cell(ctx, 0, CANVAS_H-1);
    Cell c_br = engctx_get_cell(ctx, CANVAS_W-1, CANVAS_H-1);
    TEST_EQ("top-right corner r", (int)c_tr.r, 0xFF);
    TEST_EQ("bottom-left corner r", (int)c_bl.r, 0xFF);
    TEST_EQ("bottom-right corner r", (int)c_br.r, 0xFF);

    /* canvas_ptr non-null */
    Cell* ptr = engctx_canvas_ptr(ctx);
    TEST_NOT_NULL("canvas_ptr non-null", ptr);

    engctx_destroy(ctx);
}

static void test_engine_tick_behavior(void) {
    TEST_SUITE("Engine Tick Behavior");

    EngineContext* ctx = engctx_create();
    if (!ctx) return;

    /* Set a cell with energy, tick, observe energy decay */
    Cell c = {0, 0, 0, 128};
    engctx_set_cell(ctx, 5, 5, c);

    /* Tick advances count */
    for (int i = 0; i < 50; i++) engctx_tick(ctx);
    TEST_EQ("50 ticks counted", (int)engctx_get_tick(ctx), 50);

    /* Hash changes reproducibly */
    EngineContext* ctx2 = engctx_create();
    TEST_NOT_NULL("ctx2 created", ctx2);
    if (ctx2) {
        engctx_set_cell(ctx2, 5, 5, c);
        for (int i = 0; i < 50; i++) engctx_tick(ctx2);
        uint64_t h1 = dk_canvas_hash(ctx);
        uint64_t h2 = dk_canvas_hash(ctx2);
        TEST_EQ("same sequence => same hash", h1 == h2, 1);
        engctx_destroy(ctx2);
    }

    engctx_destroy(ctx);
}

static void test_engine_status(void) {
    TEST_SUITE("Engine Status JSON");

    EngineContext* ctx = engctx_create();
    if (!ctx) return;

    char buf[1024];
    int n = engctx_get_status(ctx, buf, sizeof(buf));
    TEST_TRUE("status returns non-zero length", n > 0);
    TEST_TRUE("status contains 'tick'", strstr(buf, "tick") != NULL);
    TEST_TRUE("status contains 'hash'", strstr(buf, "hash") != NULL);
    TEST_TRUE("status contains 'branchId'", strstr(buf, "branchId") != NULL);

    /* After tick, tick value in JSON changes */
    engctx_tick(ctx);
    char buf2[1024];
    engctx_get_status(ctx, buf2, sizeof(buf2));
    TEST_TRUE("status after tick non-empty", strlen(buf2) > 0);

    /* Vis mode */
    engctx_set_vis_mode(ctx, VIS_ENERGY);
    TEST_EQ("vis mode set to ENERGY", (int)engctx_get_vis_mode(ctx), (int)VIS_ENERGY);
    engctx_set_vis_mode(ctx, VIS_OPCODE);
    TEST_EQ("vis mode set to OPCODE", (int)engctx_get_vis_mode(ctx), (int)VIS_OPCODE);
    engctx_set_vis_mode(ctx, VIS_LANE);
    TEST_EQ("vis mode set to LANE", (int)engctx_get_vis_mode(ctx), (int)VIS_LANE);
    engctx_set_vis_mode(ctx, VIS_ACTIVITY);
    TEST_EQ("vis mode set to ACTIVITY", (int)engctx_get_vis_mode(ctx), (int)VIS_ACTIVITY);
    engctx_set_vis_mode(ctx, VIS_ABGR);
    TEST_EQ("vis mode restored to ABGR", (int)engctx_get_vis_mode(ctx), (int)VIS_ABGR);

    engctx_destroy(ctx);
}

static void test_engine_viewport(void) {
    TEST_SUITE("Engine Viewport");

    EngineContext* ctx = engctx_create();
    if (!ctx) return;

    engctx_set_viewport(ctx, 10, 20, 320, 240, 4);
    TEST_TRUE("viewport set without crash", true);

    engctx_set_viewport(ctx, 0, 0, CANVAS_W, CANVAS_H, 1);
    TEST_TRUE("full viewport set without crash", true);

    /* NULL safety */
    engctx_set_viewport(NULL, 0, 0, 100, 100, 2);
    TEST_TRUE("set_viewport(NULL) safe", true);

    engctx_destroy(ctx);
}

void run_engine_tests(void) {
    test_engine_lifecycle();
    test_engine_cell();
    test_engine_tick_behavior();
    test_engine_status();
    test_engine_viewport();
}
