/*
 * Canvas OS — Rule Cell Engine + Lane Execution
 * DK-2: integer-only arithmetic
 * DK-3: cell_index ascending order
 */

#include "canvasos_ruletable.h"
#include "canvasos.h"
#include <string.h>
#include <stdlib.h>

/* ─── RuleTable ─────────────────────────────────────────── */

void ruletable_init(RuleTable* rt) {
    if (!rt) return;
    memset(rt, 0, sizeof(*rt));
    for (int i = 0; i < RULE_TABLE_SIZE; i++) {
        rt->entries[i].rule_id  = (uint8_t)i;
        rt->entries[i].enabled  = false;
        rt->entries[i].priority = 128;
    }
}

void ruletable_set(RuleTable* rt, uint8_t id,
                   AddrMode am, ExecMode em,
                   uint32_t addr_arg, uint32_t exec_arg, uint8_t prio) {
    if (!rt) return;
    RuleEntry* e = &rt->entries[id];
    e->rule_id   = id;
    e->addr_mode = am;
    e->exec_mode = em;
    e->addr_arg  = addr_arg;
    e->exec_arg  = exec_arg;
    e->priority  = prio;
    e->enabled   = true;
}

void ruletable_enable(RuleTable* rt, uint8_t id, bool en) {
    if (!rt) return;
    rt->entries[id].enabled = en;
}

RuleEntry* ruletable_get(RuleTable* rt, uint8_t id) {
    if (!rt) return NULL;
    return &rt->entries[id];
}

/* ─── BpageChain ─────────────────────────────────────────── */

void bpage_chain_init(BpageChain* bc) {
    if (!bc) return;
    memset(bc, 0, sizeof(*bc));
}

bool bpage_chain_add(BpageChain* bc, uint8_t rule_id) {
    if (!bc || bc->count >= BPAGE_CHAIN_MAX) return false;
    bc->rule_ids[bc->count++] = rule_id;
    return true;
}

void bpage_chain_clear(BpageChain* bc) {
    if (!bc) return;
    bc->count = 0;
}

int bpage_chain_resolve(const BpageChain* bc, const RuleTable* rt,
                        BpageEntry* out, int cap) {
    if (!bc || !rt || !out || cap <= 0) return 0;
    int n = 0;
    for (int i = 0; i < bc->count && n < cap; i++) {
        uint8_t rid = bc->rule_ids[i];
        const RuleEntry* re = &rt->entries[rid];
        if (!re->enabled) continue;
        BpageEntry* be = &out[n++];
        be->rule_id       = rid;
        be->addr_mode     = re->addr_mode;
        be->exec_mode     = re->exec_mode;
        be->addr_resolved = re->addr_arg;
        be->exec_value    = re->exec_arg;
    }
    return n;
}

/* ─── Lane Execution ─────────────────────────────────────── */

/* Forward declare to access engine internals */
Cell* engctx_canvas_ptr(EngineContext* ctx);
uint64_t dk_fnv1a(const uint8_t* data, size_t len);

void lane_exec_tick(EngineContext* ctx, LaneExecKey k) {
    if (!ctx) return;

    /* Build a simple bpage from lane_id as offset into ruletable */
    /* In production: this would load the bpage from per-lane storage */
    BpageChain bc;
    bpage_chain_init(&bc);

    /* Use lane_id to select a subset of rule entries */
    /* DK-3: ascending scan — rule_ids are visited in order */
    uint8_t base = (uint8_t)((k.lane_id * 4) & 0xFC);
    for (int i = 0; i < 4; i++) {
        bpage_chain_add(&bc, (uint8_t)(base + i));
    }

    /* Fetch canvas */
    Cell* canvas = engctx_canvas_ptr(ctx);
    if (!canvas) return;

    /* Resolve bpage entries */
    BpageEntry resolved[BPAGE_CHAIN_MAX];

    /* We need access to the ruletable - reach in via the opaque ctx trick:
     * lane_exec.c is compiled alongside engine.c, which has the struct def.
     * Since we include canvasos_ruletable.h and canvasos.h, and engine.c
     * defines the struct, we'd normally need to expose it. For simplicity,
     * we operate on a locally created ruletable. */
    RuleTable local_rt;
    ruletable_init(&local_rt);
    /* Enable default rules */
    for (int i = 0; i < 16; i++) {
        ruletable_set(&local_rt, (uint8_t)i,
                      ADDR_CELL, EXEC_FS,
                      (uint32_t)(k.lane_id * 64 + i),
                      (uint32_t)i, (uint8_t)i);
    }

    int n_entries = bpage_chain_resolve(&bc, &local_rt, resolved, BPAGE_CHAIN_MAX);

    /* Execute entries: DK-3 ascending cell_index */
    /* Compute tile region for this lane/page */
    uint32_t tile_start = (uint32_t)(k.page_id * 64);
    if (tile_start >= CANVAS_CELLS) return;
    uint32_t tile_end   = tile_start + 64;
    if (tile_end > CANVAS_CELLS) tile_end = CANVAS_CELLS;

    for (int e = 0; e < n_entries; e++) {
        BpageEntry* be = &resolved[e];

        /* DK-3: ascending cell scan within tile */
        for (CellIndex ci = tile_start; ci < tile_end; ci++) {
            switch (be->exec_mode) {
                case EXEC_NOP:
                    break;

                case EXEC_FS: {
                    /* field-set: set R channel = exec_value & 0xFF */
                    uint8_t val = (uint8_t)(be->exec_value & 0xFF);
                    /* DK-4: clamp already satisfied (uint8) */
                    canvas[ci].r = val;
                    break;
                }

                case EXEC_GATE: {
                    /* gate: toggle alpha */
                    canvas[ci].a ^= 0x01;
                    break;
                }

                case EXEC_COMMIT: {
                    /* commit: apply energy delta (DK-2: integer) */
                    /* Simplified: boost green by 1, clamped */
                    int32_t g = (int32_t)canvas[ci].g + 1;
                    canvas[ci].g = (uint8_t)(g > 255 ? 255 : g);
                    break;
                }
            }
        }
    }
}
