/*
 * Canvas OS — Scheduler Tests
 * 20+ test cases
 */

#include "test_framework.h"
#include "../include/canvasos_sched.h"

static void test_sched_lifecycle(void) {
    TEST_SUITE("Scheduler Lifecycle");

    Scheduler s;
    sched_init(&s);
    TEST_EQ("count after init == 0", sched_count(&s), 0);
    TEST_EQ("next_pid after init == 1", (int)s.next_pid, 1);

    sched_reset(&s);
    TEST_EQ("count after reset == 0", sched_count(&s), 0);

    /* NULL safety */
    sched_init(NULL);
    TEST_TRUE("sched_init(NULL) safe", true);
    TEST_EQ("sched_count(NULL) == 0", sched_count(NULL), 0);
}

static void test_sched_spawn(void) {
    TEST_SUITE("Scheduler Spawn");

    Scheduler s;
    sched_init(&s);

    /* Spawn first process */
    ProcId p1 = sched_spawn(&s, 0, "init");
    TEST_TRUE("first spawn returns non-zero pid", p1 != 0);
    TEST_EQ("count == 1 after spawn", sched_count(&s), 1);

    /* Spawn second */
    ProcId p2 = sched_spawn(&s, p1, "worker");
    TEST_TRUE("second spawn returns non-zero pid", p2 != 0);
    TEST_NEQ("pids are unique", p1, p2);
    TEST_EQ("count == 2 after 2 spawns", sched_count(&s), 2);

    /* Spawn with no name */
    ProcId p3 = sched_spawn(&s, 0, NULL);
    TEST_TRUE("spawn with NULL name ok", p3 != 0);
    TEST_EQ("count == 3", sched_count(&s), 3);

    /* Verify entries */
    ProcEntry* e1 = sched_find(&s, p1);
    TEST_NOT_NULL("find p1 non-null", e1);
    TEST_EQ("p1 state == RUNNING", (int)e1->state, (int)PROC_STATE_RUNNING);
    TEST_STR("p1 name == 'init'", e1->name, "init");

    ProcEntry* e2 = sched_find(&s, p2);
    TEST_NOT_NULL("find p2 non-null", e2);
    TEST_EQ("p2 parent == p1", (int)e2->parent_pid, (int)p1);

    /* PROC_MAX limit */
    Scheduler s2;
    sched_init(&s2);
    ProcId pids[PROC_MAX];
    for (int i = 0; i < PROC_MAX; i++) {
        pids[i] = sched_spawn(&s2, 0, "worker");
    }
    TEST_EQ("count == PROC_MAX when full", sched_count(&s2), PROC_MAX);

    /* Next spawn should fail */
    ProcId overflow = sched_spawn(&s2, 0, "overflow");
    TEST_EQ("spawn at PROC_MAX+1 returns 0", (int)overflow, 0);
    TEST_EQ("count unchanged at PROC_MAX", sched_count(&s2), PROC_MAX);
}

static void test_sched_kill(void) {
    TEST_SUITE("Scheduler Kill");

    Scheduler s;
    sched_init(&s);

    ProcId p1 = sched_spawn(&s, 0, "target");
    ProcId p2 = sched_spawn(&s, 0, "other");

    /* Kill p1 */
    bool ok = sched_kill(&s, p1);
    TEST_TRUE("kill existing pid returns true", ok);

    ProcEntry* e1 = sched_find(&s, p1);
    TEST_NOT_NULL("find zombie p1", e1);
    TEST_EQ("p1 state == ZOMBIE after kill", (int)e1->state, (int)PROC_STATE_ZOMBIE);

    /* Reap zombies */
    sched_reap(&s);
    ProcEntry* dead = sched_find(&s, p1);
    TEST_NULL("find after reap returns null", dead);
    TEST_EQ("count decremented after reap", sched_count(&s), 1);

    /* p2 still alive */
    ProcEntry* e2 = sched_find(&s, p2);
    TEST_NOT_NULL("p2 still alive", e2);
    TEST_EQ("p2 still RUNNING", (int)e2->state, (int)PROC_STATE_RUNNING);

    /* Kill by name */
    ProcId p3 = sched_spawn(&s, 0, "named_proc");
    bool ok2 = sched_kill_name(&s, "named_proc");
    TEST_TRUE("kill_name returns true", ok2);
    ProcEntry* e3 = sched_find(&s, p3);
    TEST_NOT_NULL("killed-by-name is zombie", e3);
    TEST_EQ("zombie state", (int)e3->state, (int)PROC_STATE_ZOMBIE);

    /* Kill non-existent */
    bool bad = sched_kill(&s, 99999);
    TEST_FALSE("kill non-existent returns false", bad);

    /* kill_name sets exit_code (same as sched_kill) */
    ProcId p4 = sched_spawn(&s, 0, "check_exit");
    sched_kill_name(&s, "check_exit");
    ProcEntry* e4 = sched_find(&s, p4);
    TEST_NOT_NULL("kill_name proc found as zombie", e4);
    TEST_EQ("kill_name sets exit_code == 1", e4->exit_code, 1);

    /* NULL safety */
    TEST_FALSE("kill_name NULL", sched_kill_name(NULL, "x"));
}

static void test_sched_tick(void) {
    TEST_SUITE("Scheduler Tick");

    Scheduler s;
    sched_init(&s);

    ProcId p1 = sched_spawn(&s, 0, "timed");
    sched_tick_all(&s);
    sched_tick_all(&s);
    sched_tick_all(&s);

    ProcEntry* e = sched_find(&s, p1);
    TEST_NOT_NULL("find timed proc", e);
    TEST_EQ("ticks_alive == 3 after 3 tick_all", (int)e->ticks_alive, 3);

    /* JSON output */
    char buf[2048];
    int n = sched_to_json(&s, buf, sizeof(buf));
    TEST_TRUE("sched_to_json returns > 0", n > 0);
    TEST_TRUE("json starts with '['", buf[0] == '[');
    TEST_TRUE("json contains 'pid'", strstr(buf, "pid") != NULL);

    /* JSON escaping: name with quotes should not break JSON */
    Scheduler s2;
    sched_init(&s2);
    sched_spawn(&s2, 0, "proc\"with\\quotes");
    char buf2[2048];
    sched_to_json(&s2, buf2, sizeof(buf2));
    /* The raw quote should not appear unescaped in output */
    TEST_FALSE("JSON escaped: raw quote not in name field",
               strstr(buf2, "\"proc\"with") != NULL);

    /* List */
    ProcEntry list[PROC_MAX];
    int cnt = sched_list(&s, list, PROC_MAX);
    TEST_EQ("sched_list returns 1", cnt, 1);
    TEST_EQ("listed proc pid matches", (int)list[0].pid, (int)p1);

    /* NULL safety */
    sched_tick_all(NULL);
    TEST_TRUE("sched_tick_all(NULL) safe", true);
}

void run_scheduler_tests(void) {
    test_sched_lifecycle();
    test_sched_spawn();
    test_sched_kill();
    test_sched_tick();
}
