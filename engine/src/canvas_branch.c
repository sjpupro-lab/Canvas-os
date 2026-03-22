/*
 * Canvas OS — Branch system
 * O(1) branch switch via PageSelector
 * Max 256 branches
 */

#include "canvasos.h"
#include "canvasos_json.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define BRANCH_MAX      256
#define BRANCH_NAME_LEN 32

typedef struct {
    BranchId id;
    BranchId parent;
    uint64_t created_tick;
    uint64_t fork_hash;       /* hash at branch creation */
    char     name[BRANCH_NAME_LEN];
    bool     active;
    /* Snapshot of canvas at branch point (lazy: NULL until needed) */
    /* For simplicity: we store just the hash, not full canvas copy */
    uint64_t snapshot_hash;
} BranchEntry;

typedef struct {
    BranchEntry branches[BRANCH_MAX];
    int         count;
    BranchId    current;
    BranchId    next_id;
} BranchTable;

/* Global branch table (one per engine context in practice) */
/* We expose a simple C API here */

static BranchTable g_branch_table;

void branch_table_init(void) {
    memset(&g_branch_table, 0, sizeof(g_branch_table));
    g_branch_table.next_id  = 1;
    g_branch_table.current  = 0;
    /* Create branch 0 (main) */
    BranchEntry* main_br    = &g_branch_table.branches[0];
    main_br->id             = 0;
    main_br->parent         = 0;
    main_br->created_tick   = 0;
    main_br->active         = true;
    main_br->fork_hash      = 0;
    main_br->snapshot_hash  = 0;
    strncpy(main_br->name, "main", BRANCH_NAME_LEN - 1);
    g_branch_table.count    = 1;
}

BranchId branch_create(BranchId parent, const char* name, uint64_t tick, uint64_t hash) {
    if (g_branch_table.count >= BRANCH_MAX) return (BranchId)-1;

    BranchId new_id = g_branch_table.next_id++;
    /* Use id as index (linear scan for empty slot) */
    for (int i = 0; i < BRANCH_MAX; i++) {
        if (!g_branch_table.branches[i].active) {
            BranchEntry* e  = &g_branch_table.branches[i];
            e->id           = new_id;
            e->parent       = parent;
            e->created_tick = tick;
            e->fork_hash    = hash;
            e->snapshot_hash = hash;
            e->active       = true;
            if (name) {
                strncpy(e->name, name, BRANCH_NAME_LEN - 1);
                e->name[BRANCH_NAME_LEN - 1] = '\0';
            } else {
                snprintf(e->name, BRANCH_NAME_LEN, "branch-%u", new_id);
            }
            g_branch_table.count++;
            return new_id;
        }
    }
    return (BranchId)-1;
}

/* O(1) branch switch — just update current pointer */
bool branch_switch(BranchId id) {
    for (int i = 0; i < BRANCH_MAX; i++) {
        if (g_branch_table.branches[i].active &&
            g_branch_table.branches[i].id == id) {
            g_branch_table.current = id;
            return true;
        }
    }
    return false;
}

BranchId branch_current(void) {
    return g_branch_table.current;
}

int branch_count(void) {
    return g_branch_table.count;
}

BranchEntry* branch_find(BranchId id) {
    for (int i = 0; i < BRANCH_MAX; i++) {
        if (g_branch_table.branches[i].active &&
            g_branch_table.branches[i].id == id) {
            return &g_branch_table.branches[i];
        }
    }
    return NULL;
}

bool branch_delete(BranchId id) {
    if (id == 0) return false; /* cannot delete main */
    for (int i = 0; i < BRANCH_MAX; i++) {
        if (g_branch_table.branches[i].active &&
            g_branch_table.branches[i].id == id) {
            g_branch_table.branches[i].active = false;
            if (g_branch_table.count > 0) g_branch_table.count--;
            if (g_branch_table.current == id) g_branch_table.current = 0;
            return true;
        }
    }
    return false;
}

int branch_list_json(char* buf, int cap) {
    if (!buf || cap <= 0) return 0;
    int written = snprintf(buf, (size_t)cap, "[");
    bool first = true;
    for (int i = 0; i < BRANCH_MAX; i++) {
        BranchEntry* e = &g_branch_table.branches[i];
        if (!e->active) continue;
        char esc_name[BRANCH_NAME_LEN * 2];
        json_escape_str(e->name, esc_name, sizeof(esc_name));
        int n = snprintf(buf + written, (size_t)(cap - written),
            "%s{\"id\":%u,\"parent\":%u,\"name\":\"%s\",\"tick\":%llu}",
            first ? "" : ",",
            e->id, e->parent, esc_name,
            (unsigned long long)e->created_tick);
        if (n < 0 || written + n >= cap - 2) break;
        written += n;
        first = false;
    }
    if (written < cap - 1) {
        buf[written++] = ']';
        buf[written]   = '\0';
    }
    return written;
}
