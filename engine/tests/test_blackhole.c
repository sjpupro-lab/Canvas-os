/*
 * Canvas OS — BlackHole Tests
 * 20+ test cases
 */

#include "test_framework.h"
#include "../include/canvasos_bh.h"

static void test_bh_lifecycle(void) {
    TEST_SUITE("BlackHole Lifecycle");

    BlackHole bh;
    bh_init(&bh);
    TEST_EQ("bh_count after init == 0", bh_count(&bh), 0);
    TEST_EQ("total_compressed after init == 0", (int)bh_total_compressed(&bh), 0);

    bh_reset(&bh);
    TEST_EQ("bh_count after reset == 0", bh_count(&bh), 0);

    bh_init(NULL);
    TEST_TRUE("bh_init(NULL) safe", true);
    bh_reset(NULL);
    TEST_TRUE("bh_reset(NULL) safe", true);
}

static void test_bh_compress(void) {
    TEST_SUITE("BlackHole Compression");

    BlackHole bh;
    bh_init(&bh);

    /* Create a run of identical values (> BH_MIN_RUN) */
    uint32_t cells[64];
    for (int i = 0; i < 64; i++) cells[i] = 0; /* all zero */

    int added = bh_compress(&bh, cells, 64, 0);
    TEST_TRUE("compress zeros produces entries", added >= 1);
    TEST_TRUE("count > 0 after compress", bh_count(&bh) > 0);
    TEST_TRUE("total_compressed > 0", bh_total_compressed(&bh) > 0);

    /* Non-compressible (all different) */
    BlackHole bh2;
    bh_init(&bh2);
    uint32_t unique[64];
    for (int i = 0; i < 64; i++) unique[i] = (uint32_t)i;
    int added2 = bh_compress(&bh2, unique, 64, 0);
    TEST_EQ("no compression for unique values", added2, 0);
    TEST_EQ("no entries for unique", bh_count(&bh2), 0);

    /* Mixed: some runs, some unique */
    BlackHole bh3;
    bh_init(&bh3);
    uint32_t mixed[32];
    for (int i = 0; i < 16; i++) mixed[i] = 0xFF;   /* run of 16 */
    for (int i = 16; i < 32; i++) mixed[i] = (uint32_t)i; /* unique */
    int added3 = bh_compress(&bh3, mixed, 32, 0);
    TEST_TRUE("mixed: at least one compressed run", added3 >= 1);

    /* Null safety */
    TEST_EQ("compress NULL cells == 0", bh_compress(&bh, NULL, 10, 0), 0);
    TEST_EQ("compress NULL bh == 0", bh_compress(NULL, cells, 64, 0), 0);
    TEST_EQ("compress n=0 == 0", bh_compress(&bh, cells, 0, 0), 0);
}

static void test_bh_decompress(void) {
    TEST_SUITE("BlackHole Decompress");

    BlackHole bh;
    bh_init(&bh);

    /* Compress a known pattern */
    uint32_t cells[32];
    for (int i = 0; i < 32; i++) cells[i] = 0xDEADBEEF;
    bh_compress(&bh, cells, 32, 0);

    /* Decompress */
    uint32_t out[32];
    bool ok = bh_decompress(&bh, 0, out, 32);
    TEST_TRUE("decompress entry 0 succeeds", ok);
    TEST_EQ("decompressed value[0] == 0xDEADBEEF",
            (int)out[0], (int)0xDEADBEEF);
    TEST_EQ("decompressed value[15] == 0xDEADBEEF",
            (int)out[15], (int)0xDEADBEEF);

    /* Bad entry id */
    bool bad = bh_decompress(&bh, 999, out, 32);
    TEST_FALSE("decompress bad id returns false", bad);

    /* NULL safety */
    TEST_FALSE("decompress NULL bh", bh_decompress(NULL, 0, out, 32));
    TEST_FALSE("decompress NULL out", bh_decompress(&bh, 0, NULL, 32));
}

static void test_bh_find(void) {
    TEST_SUITE("BlackHole Find");

    BlackHole bh;
    bh_init(&bh);

    uint32_t cells[32];
    for (int i = 0; i < 32; i++) cells[i] = 0xABCD;
    bh_compress(&bh, cells, 32, 100); /* base_idx = 100 */

    /* Find cell in range */
    int idx = bh_find(&bh, 110);  /* 110 is in 100..131 */
    TEST_TRUE("find within range >= 0", idx >= 0);

    /* Find cell outside range */
    int bad = bh_find(&bh, 200);
    TEST_EQ("find outside range == -1", bad, -1);

    int bad2 = bh_find(&bh, 50);
    TEST_EQ("find below base == -1", bad2, -1);

    /* NULL safety */
    TEST_EQ("bh_find(NULL) == -1", bh_find(NULL, 0), -1);
}

static void test_bh_json(void) {
    TEST_SUITE("BlackHole JSON");

    BlackHole bh;
    bh_init(&bh);

    uint32_t cells[16];
    for (int i = 0; i < 16; i++) cells[i] = 0xFF00FF;
    bh_compress(&bh, cells, 16, 0);

    char buf[2048];
    int n = bh_to_json(&bh, buf, sizeof(buf));
    TEST_TRUE("bh_to_json returns > 0", n > 0);
    TEST_TRUE("json contains 'count'", strstr(buf, "count") != NULL);

    /* Empty bh */
    BlackHole bh2;
    bh_init(&bh2);
    int n2 = bh_to_json(&bh2, buf, sizeof(buf));
    TEST_TRUE("empty bh json n >= 0", n2 >= 0);
}

void run_blackhole_tests(void) {
    test_bh_lifecycle();
    test_bh_compress();
    test_bh_decompress();
    test_bh_find();
    test_bh_json();
}
