/*
 * Canvas OS — Rule Cell Engine Tests
 * 25+ test cases
 */

#include "test_framework.h"
#include "../include/canvasos_ruletable.h"
#include "../include/canvasos.h"

static void test_ruletable_init(void) {
    TEST_SUITE("RuleTable Init");

    RuleTable rt;
    ruletable_init(&rt);

    /* All entries disabled after init */
    bool all_disabled = true;
    for (int i = 0; i < RULE_TABLE_SIZE; i++) {
        if (rt.entries[i].enabled) { all_disabled = false; break; }
    }
    TEST_TRUE("all rules disabled after init", all_disabled);

    /* Rule IDs are set correctly */
    TEST_EQ("rt entry[0].rule_id == 0", (int)rt.entries[0].rule_id, 0);
    TEST_EQ("rt entry[255].rule_id == 255", (int)rt.entries[255].rule_id, 255);
    TEST_EQ("rt RULE_TABLE_SIZE == 256", RULE_TABLE_SIZE, 256);

    /* NULL safety */
    ruletable_init(NULL);
    TEST_TRUE("ruletable_init(NULL) safe", true);
}

static void test_ruletable_set_get(void) {
    TEST_SUITE("RuleTable Set/Get");

    RuleTable rt;
    ruletable_init(&rt);

    /* Set rule 0 */
    ruletable_set(&rt, 0, ADDR_CELL, EXEC_FS, 100, 200, 5);
    RuleEntry* e = ruletable_get(&rt, 0);
    TEST_NOT_NULL("ruletable_get(0) non-null", e);
    TEST_EQ("rule 0 addr_mode == CELL", (int)e->addr_mode, (int)ADDR_CELL);
    TEST_EQ("rule 0 exec_mode == FS", (int)e->exec_mode, (int)EXEC_FS);
    TEST_EQ("rule 0 addr_arg == 100", (int)e->addr_arg, 100);
    TEST_EQ("rule 0 exec_arg == 200", (int)e->exec_arg, 200);
    TEST_EQ("rule 0 priority == 5", (int)e->priority, 5);
    TEST_TRUE("rule 0 enabled", e->enabled);

    /* Set rule 255 */
    ruletable_set(&rt, 255, ADDR_TILE, EXEC_GATE, 50, 60, 128);
    RuleEntry* e2 = ruletable_get(&rt, 255);
    TEST_NOT_NULL("ruletable_get(255) non-null", e2);
    TEST_EQ("rule 255 addr_mode == TILE", (int)e2->addr_mode, (int)ADDR_TILE);
    TEST_EQ("rule 255 exec_mode == GATE", (int)e2->exec_mode, (int)EXEC_GATE);

    /* Enable/disable */
    ruletable_enable(&rt, 0, false);
    TEST_FALSE("rule 0 disabled", rt.entries[0].enabled);
    ruletable_enable(&rt, 0, true);
    TEST_TRUE("rule 0 re-enabled", rt.entries[0].enabled);

    /* AddrModes */
    ruletable_set(&rt, 1, ADDR_NONE, EXEC_NOP, 0, 0, 0);
    TEST_EQ("ADDR_NONE", (int)rt.entries[1].addr_mode, (int)ADDR_NONE);
    ruletable_set(&rt, 2, ADDR_SLOT, EXEC_COMMIT, 0, 0, 0);
    TEST_EQ("ADDR_SLOT", (int)rt.entries[2].addr_mode, (int)ADDR_SLOT);

    /* NULL safety */
    ruletable_set(NULL, 0, ADDR_NONE, EXEC_NOP, 0, 0, 0);
    TEST_TRUE("ruletable_set(NULL) safe", true);
    TEST_NULL("ruletable_get(NULL) == null", ruletable_get(NULL, 0));
}

static void test_bpage_chain(void) {
    TEST_SUITE("BpageChain");

    BpageChain bc;
    bpage_chain_init(&bc);
    TEST_EQ("bpage count after init == 0", bc.count, 0);

    /* Add rules */
    bool ok0 = bpage_chain_add(&bc, 0);
    TEST_TRUE("add rule 0 ok", ok0);
    TEST_EQ("count == 1 after add", bc.count, 1);

    bool ok1 = bpage_chain_add(&bc, 5);
    TEST_TRUE("add rule 5 ok", ok1);
    TEST_EQ("count == 2", bc.count, 2);

    /* Fill to max */
    for (int i = 0; i < BPAGE_CHAIN_MAX - 2; i++) {
        bpage_chain_add(&bc, (uint8_t)(10 + i));
    }
    TEST_EQ("count == BPAGE_CHAIN_MAX", bc.count, BPAGE_CHAIN_MAX);

    /* Overflow returns false */
    bool overflow = bpage_chain_add(&bc, 99);
    TEST_FALSE("add beyond max returns false", overflow);
    TEST_EQ("count unchanged at max", bc.count, BPAGE_CHAIN_MAX);

    /* Clear */
    bpage_chain_clear(&bc);
    TEST_EQ("count == 0 after clear", bc.count, 0);

    /* NULL safety */
    bpage_chain_init(NULL);
    TEST_TRUE("bpage_chain_init(NULL) safe", true);
    TEST_FALSE("bpage_chain_add(NULL) false", bpage_chain_add(NULL, 0));
}

static void test_bpage_resolve(void) {
    TEST_SUITE("BpageChain Resolve");

    RuleTable rt;
    ruletable_init(&rt);
    ruletable_set(&rt, 10, ADDR_CELL, EXEC_FS, 100, 77, 1);
    ruletable_set(&rt, 20, ADDR_TILE, EXEC_GATE, 50, 88, 2);

    BpageChain bc;
    bpage_chain_init(&bc);
    bpage_chain_add(&bc, 10);
    bpage_chain_add(&bc, 20);
    bpage_chain_add(&bc, 30); /* not in table (disabled) */

    BpageEntry out[BPAGE_CHAIN_MAX];
    int n = bpage_chain_resolve(&bc, &rt, out, BPAGE_CHAIN_MAX);

    /* Only 2 enabled rules */
    TEST_EQ("resolve returns 2 enabled rules", n, 2);
    TEST_EQ("first entry rule_id == 10", (int)out[0].rule_id, 10);
    TEST_EQ("first entry exec_value == 77", (int)out[0].exec_value, 77);
    TEST_EQ("second entry rule_id == 20", (int)out[1].rule_id, 20);
    TEST_EQ("second entry exec_value == 88", (int)out[1].exec_value, 88);

    /* NULL safety */
    int n2 = bpage_chain_resolve(NULL, &rt, out, BPAGE_CHAIN_MAX);
    TEST_EQ("resolve NULL bc == 0", n2, 0);
    int n3 = bpage_chain_resolve(&bc, NULL, out, BPAGE_CHAIN_MAX);
    TEST_EQ("resolve NULL rt == 0", n3, 0);
    int n4 = bpage_chain_resolve(&bc, &rt, NULL, BPAGE_CHAIN_MAX);
    TEST_EQ("resolve NULL out == 0", n4, 0);
}

static void test_lane_exec(void) {
    TEST_SUITE("Lane Execution");

    EngineContext* ctx = engctx_create();
    if (!ctx) return;

    LaneExecKey k = {0, 0};
    lane_exec_tick(ctx, k);
    TEST_TRUE("lane_exec_tick completes without crash", true);

    /* Different lanes */
    LaneExecKey k2 = {1, 0};
    lane_exec_tick(ctx, k2);
    TEST_TRUE("lane_exec_tick lane 1 completes", true);

    /* High lane/page values */
    LaneExecKey k3 = {255, 255};
    lane_exec_tick(ctx, k3);
    TEST_TRUE("lane_exec_tick max key completes", true);

    /* NULL safety */
    lane_exec_tick(NULL, k);
    TEST_TRUE("lane_exec_tick NULL ctx safe", true);

    engctx_destroy(ctx);
}

void run_rulecell_tests(void) {
    test_ruletable_init();
    test_ruletable_set_get();
    test_bpage_chain();
    test_bpage_resolve();
    test_lane_exec();
}
