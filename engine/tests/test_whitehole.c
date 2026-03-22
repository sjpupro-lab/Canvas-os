/*
 * Canvas OS — WhiteHole Tests
 * 25+ test cases
 */

#include "test_framework.h"
#include "../include/canvasos_wh.h"

static void test_wh_lifecycle(void) {
    TEST_SUITE("WhiteHole Lifecycle");

    WhiteHole wh;
    wh_init(&wh);
    TEST_EQ("wh_count after init == 0", (int)wh_count(&wh), 0);
    TEST_EQ("wh head == 0 after init", (int)wh.head, 0);
    TEST_EQ("wh total == 0 after init", (int)wh.total, 0);

    wh_reset(&wh);
    TEST_EQ("wh_count after reset == 0", (int)wh_count(&wh), 0);

    /* NULL safety */
    wh_init(NULL);
    TEST_TRUE("wh_init(NULL) safe", true);
    wh_reset(NULL);
    TEST_TRUE("wh_reset(NULL) safe", true);
    TEST_EQ("wh_count(NULL) == 0", (int)wh_count(NULL), 0);
}

static void test_wh_write_read(void) {
    TEST_SUITE("WhiteHole Write/Read");

    WhiteHole wh;
    wh_init(&wh);

    /* Write one record */
    wh_log(&wh, 42, WH_EVT_TICK, 0, 42, "hello");
    TEST_EQ("count == 1 after one write", (int)wh_count(&wh), 1);

    /* Read it back */
    WhRecord r;
    bool ok = wh_read(&wh, 0, &r);
    TEST_TRUE("wh_read offset 0 succeeds", ok);
    TEST_EQ("record tick == 42", (int)r.tick, 42);
    TEST_EQ("record type == WH_EVT_TICK", (int)r.type, (int)WH_EVT_TICK);
    TEST_EQ("record value == 42", (int)r.value, 42);
    TEST_STR("record msg == 'hello'", r.msg, "hello");

    /* Write second record */
    wh_log(&wh, 100, WH_EVT_CELL, 512, 7, "cell-write");
    TEST_EQ("count == 2 after two writes", (int)wh_count(&wh), 2);

    /* newest at offset 0 */
    WhRecord r2;
    wh_read(&wh, 0, &r2);
    TEST_EQ("newest (offset 0) tick == 100", (int)r2.tick, 100);

    /* older at offset 1 */
    WhRecord r1;
    wh_read(&wh, 1, &r1);
    TEST_EQ("older (offset 1) tick == 42", (int)r1.tick, 42);

    /* Out-of-range read returns false */
    WhRecord rx;
    bool bad = wh_read(&wh, 999, &rx);
    TEST_FALSE("read offset 999 returns false (only 2 records)", bad);

    /* NULL safety */
    TEST_FALSE("wh_read NULL wh", wh_read(NULL, 0, &r));
    TEST_FALSE("wh_read NULL out", wh_read(&wh, 0, NULL));
}

static void test_wh_circular(void) {
    TEST_SUITE("WhiteHole Circular Buffer");

    WhiteHole wh;
    wh_init(&wh);

    /* Fill to capacity */
    for (int i = 0; i < WH_CAPACITY; i++) {
        wh_log(&wh, (uint64_t)i, WH_EVT_TICK, 0, (uint32_t)i, "");
    }
    TEST_EQ("count == WH_CAPACITY when full", (int)wh_count(&wh), WH_CAPACITY);
    TEST_EQ("total == WH_CAPACITY", (int)wh.total, WH_CAPACITY);

    /* head wraps to 0 exactly at capacity — reads must still work (bug fix) */
    TEST_EQ("head == 0 when exactly full", (int)wh.head, 0);
    WhRecord r_full;
    bool ok_full = wh_read(&wh, 0, &r_full);
    TEST_TRUE("wh_read still works when head==0 and buffer exactly full", ok_full);
    TEST_EQ("newest value == WH_CAPACITY-1 when exactly full",
            (int)r_full.value, WH_CAPACITY - 1);

    /* Write one more — should overwrite oldest */
    wh_log(&wh, (uint64_t)WH_CAPACITY, WH_EVT_HASH, 0, 9999, "overflow");
    TEST_EQ("count still WH_CAPACITY after overflow", (int)wh_count(&wh), WH_CAPACITY);
    TEST_EQ("total == WH_CAPACITY+1", (int)wh.total, WH_CAPACITY + 1);

    /* Newest record should be the overflow one */
    WhRecord r;
    wh_read(&wh, 0, &r);
    TEST_EQ("newest after overflow has correct value", (int)r.value, 9999);
    TEST_EQ("newest tick == WH_CAPACITY", (int)r.tick, WH_CAPACITY);
}

static void test_wh_event_types(void) {
    TEST_SUITE("WhiteHole Event Types");

    WhiteHole wh;
    wh_init(&wh);

    WhEventType types[] = {
        WH_EVT_TICK, WH_EVT_CELL, WH_EVT_BRANCH,
        WH_EVT_GATE, WH_EVT_SPAWN, WH_EVT_KILL,
        WH_EVT_SHELL, WH_EVT_TIMEWARP, WH_EVT_HASH
    };
    const char* names[] = {
        "tick","cell","branch","gate","spawn","kill","shell","timewarp","hash"
    };
    int n_types = sizeof(types)/sizeof(types[0]);

    for (int i = 0; i < n_types; i++) {
        wh_log(&wh, (uint64_t)i, types[i], (uint32_t)i, (uint32_t)i, names[i]);
    }
    TEST_EQ("all event types written", (int)wh_count(&wh), n_types);

    /* Read newest (last written = hash) */
    WhRecord r;
    wh_read(&wh, 0, &r);
    TEST_EQ("newest event is HASH", (int)r.type, (int)WH_EVT_HASH);
}

static void test_wh_json(void) {
    TEST_SUITE("WhiteHole JSON");

    WhiteHole wh;
    wh_init(&wh);

    wh_log(&wh, 1, WH_EVT_TICK, 0, 1, "t1");
    wh_log(&wh, 2, WH_EVT_CELL, 5, 2, "t2");

    char buf[2048];
    int n = wh_to_json(&wh, 2, buf, sizeof(buf));
    TEST_TRUE("wh_to_json returns > 0", n > 0);
    TEST_TRUE("json starts with '['", buf[0] == '[');
    TEST_TRUE("json contains 'tick'", strstr(buf, "tick") != NULL);

    /* Empty log */
    WhiteHole wh2;
    wh_init(&wh2);
    int n2 = wh_to_json(&wh2, 10, buf, sizeof(buf));
    TEST_TRUE("empty wh json n >= 0", n2 >= 0);
    TEST_TRUE("empty json starts with '['", buf[0] == '[');
}

void run_whitehole_tests(void) {
    test_wh_lifecycle();
    test_wh_write_read();
    test_wh_circular();
    test_wh_event_types();
    test_wh_json();
}
