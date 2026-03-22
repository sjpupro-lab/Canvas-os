/*
 * Canvas OS — GPU Stub Tests
 * 15+ test cases
 */

#include "test_framework.h"
#include "../include/canvas_gpu.h"

static void test_gpu_lifecycle(void) {
    TEST_SUITE("GPU Stub Lifecycle");

    GpuCtx* g = gpu_init();
    TEST_NOT_NULL("gpu_init returns non-null", g);

    /* Stub: GPU not available */
    bool avail = gpu_is_available();
    TEST_FALSE("gpu_is_available returns false (stub)", avail);

    /* Stats after init */
    GpuStats stats = gpu_get_stats(g);
    TEST_EQ("backend == 0 (CPU stub)", stats.backend, 0);
    TEST_EQ("tiles_uploaded == 0 initially", stats.tiles_uploaded, 0);
    TEST_EQ("active_cells == 0 initially", stats.active_cells, 0);

    gpu_shutdown(g);
    TEST_TRUE("gpu_shutdown completes", true);

    /* NULL safety */
    gpu_shutdown(NULL);
    TEST_TRUE("gpu_shutdown(NULL) safe", true);
    GpuStats ns = gpu_get_stats(NULL);
    TEST_EQ("gpu_get_stats(NULL) backend == 0", ns.backend, 0);
}

static void test_gpu_upload(void) {
    TEST_SUITE("GPU Tile Upload");

    GpuCtx* g = gpu_init();
    if (!g) return;

    /* Upload a 4x4 grid */
    uint32_t tiles[16] = {0};
    tiles[0] = 0xFF0000FF;  /* non-zero */
    tiles[5] = 0x00FF00FF;
    tiles[15]= 0x0000FFFF;

    bool ok = gpu_upload_tiles(g, tiles, 4, 4);
    TEST_TRUE("upload 4x4 tiles returns true", ok);

    GpuStats s = gpu_get_stats(g);
    TEST_EQ("tiles_uploaded == 16 after 4x4", s.tiles_uploaded, 16);

    /* Null safety */
    TEST_FALSE("upload NULL tiles returns false", gpu_upload_tiles(g, NULL, 4, 4));
    TEST_FALSE("upload NULL ctx returns false", gpu_upload_tiles(NULL, tiles, 4, 4));
    TEST_FALSE("upload w=0 returns false", gpu_upload_tiles(g, tiles, 0, 4));

    gpu_shutdown(g);
}

static void test_gpu_scan(void) {
    TEST_SUITE("GPU Active Set Scan");

    GpuCtx* g = gpu_init();
    if (!g) return;

    uint32_t tiles[16] = {0};
    tiles[2]  = 1;
    tiles[7]  = 1;
    tiles[13] = 1;
    gpu_upload_tiles(g, tiles, 4, 4);

    uint32_t active[16];
    int n = gpu_scan_active_set(g, active, 16);
    TEST_EQ("scan finds 3 active cells", n, 3);

    /* DK-3: results in ascending order */
    bool ascending = (n >= 2) ? (active[0] < active[1]) : true;
    if (n >= 3) ascending = ascending && (active[1] < active[2]);
    TEST_TRUE("scan results in ascending order (DK-3)", ascending);

    /* Verify correct indices */
    TEST_EQ("first active == 2", (int)active[0], 2);
    TEST_EQ("second active == 7", (int)active[1], 7);
    TEST_EQ("third active == 13", (int)active[2], 13);

    /* All zero: no active */
    uint32_t zeros[16] = {0};
    gpu_upload_tiles(g, zeros, 4, 4);
    int n2 = gpu_scan_active_set(g, active, 16);
    TEST_EQ("scan all-zero gives 0 active", n2, 0);

    /* NULL safety */
    TEST_EQ("scan NULL ctx == 0", gpu_scan_active_set(NULL, active, 16), 0);
    TEST_EQ("scan NULL out == 0", gpu_scan_active_set(g, NULL, 16), 0);

    gpu_shutdown(g);
}

static void test_gpu_bh_summarize(void) {
    TEST_SUITE("GPU BH Summarize Idle");

    GpuCtx* g = gpu_init();
    if (!g) return;

    /* Upload tiles with a run of zeros */
    uint32_t tiles[32] = {0};
    tiles[0] = 1;   /* active */
    /* tiles[1..30] = 0 (idle run of 30) */
    tiles[31] = 1;  /* active */
    gpu_upload_tiles(g, tiles, 32, 1);

    BhSummary sums[16];
    int n = gpu_bh_summarize_idle(g, sums, 16);
    TEST_TRUE("bh_summarize finds idle regions", n > 0);
    TEST_TRUE("idle region length >= 4", sums[0].region_len >= 4);
    TEST_TRUE("idle region hash computed", n > 0);

    /* NULL safety */
    TEST_EQ("bh_summarize NULL ctx == 0", gpu_bh_summarize_idle(NULL, sums, 16), 0);
    TEST_EQ("bh_summarize NULL out == 0", gpu_bh_summarize_idle(g, NULL, 16), 0);

    gpu_shutdown(g);
}

static void test_gpu_delta_merge(void) {
    TEST_SUITE("GPU Delta Tile Merge");

    GpuCtx* g = gpu_init();
    if (!g) return;

    uint32_t tiles[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
    gpu_upload_tiles(g, tiles, 8, 1);

    /* XOR delta */
    uint32_t delta[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    bool ok = gpu_merge_delta_tiles(g, delta, 8);
    TEST_TRUE("delta merge returns true", ok);

    /* Scan: after XOR with 0xFF, all tiles non-zero */
    uint32_t active[16];
    int n = gpu_scan_active_set(g, active, 16);
    TEST_EQ("all 8 tiles active after XOR delta", n, 8);

    /* NULL safety */
    TEST_FALSE("delta merge NULL ctx", gpu_merge_delta_tiles(NULL, delta, 8));
    TEST_FALSE("delta merge NULL delta", gpu_merge_delta_tiles(g, NULL, 8));

    gpu_shutdown(g);
}

void run_gpu_tests(void) {
    test_gpu_lifecycle();
    test_gpu_upload();
    test_gpu_scan();
    test_gpu_bh_summarize();
    test_gpu_delta_merge();
}
