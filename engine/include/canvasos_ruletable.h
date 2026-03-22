#pragma once

/*
 * Canvas OS — Rule Cell Engine
 * 256-entry RuleTable + BpageChain (up to 8 rules per cell)
 */

#include <stdint.h>
#include <stdbool.h>

/* ─── Addressing modes ──────────────────────────────────── */
typedef enum {
    ADDR_NONE = 0,
    ADDR_SLOT = 1,   /* register slot */
    ADDR_TILE = 2,   /* tile-relative */
    ADDR_CELL = 3    /* absolute cell  */
} AddrMode;

/* ─── Execution modes ───────────────────────────────────── */
typedef enum {
    EXEC_NOP    = 0,
    EXEC_FS     = 1,   /* field-set  */
    EXEC_GATE   = 2,   /* gate op    */
    EXEC_COMMIT = 3    /* commit delta */
} ExecMode;

/* ─── Rule Entry ─────────────────────────────────────────── */
#define RULE_TABLE_SIZE 256

typedef struct {
    uint8_t  rule_id;
    AddrMode addr_mode;
    ExecMode exec_mode;
    uint32_t addr_arg;   /* tile/cell/slot arg */
    uint32_t exec_arg;   /* value arg           */
    uint8_t  priority;   /* lower = higher prio */
    bool     enabled;
} RuleEntry;

typedef struct {
    RuleEntry entries[RULE_TABLE_SIZE];
} RuleTable;

/* ─── BpageChain ─────────────────────────────────────────── */
#define BPAGE_CHAIN_MAX 8   /* max rules per bpage */

typedef struct {
    uint8_t rule_ids[BPAGE_CHAIN_MAX];
    int     count;
} BpageChain;

typedef struct {
    uint8_t  rule_id;
    AddrMode addr_mode;
    ExecMode exec_mode;
    uint32_t addr_resolved;
    uint32_t exec_value;
} BpageEntry;

/* ─── Lane Execution Key ────────────────────────────────── */
typedef struct {
    uint16_t lane_id;
    uint16_t page_id;
} LaneExecKey;

/* ─── Rule Table API ────────────────────────────────────── */
void       ruletable_init(RuleTable* rt);
void       ruletable_set(RuleTable* rt, uint8_t id, AddrMode am, ExecMode em,
                         uint32_t addr_arg, uint32_t exec_arg, uint8_t prio);
void       ruletable_enable(RuleTable* rt, uint8_t id, bool en);
RuleEntry* ruletable_get(RuleTable* rt, uint8_t id);

/* ─── BpageChain API ────────────────────────────────────── */
void bpage_chain_init(BpageChain* bc);
bool bpage_chain_add(BpageChain* bc, uint8_t rule_id);
void bpage_chain_clear(BpageChain* bc);
int  bpage_chain_resolve(const BpageChain* bc, const RuleTable* rt,
                         BpageEntry* out, int cap);

/* ─── Engine Context forward decl ───────────────────────── */
typedef struct EngineContext EngineContext;

/* Lane exec */
void lane_exec_tick(EngineContext* ctx, LaneExecKey k);

/* Delta buffer (integer-only) */
typedef struct {
    uint32_t cell_index;   /* DK-3: ascending */
    uint32_t delta_a;
    uint32_t delta_g;
    uint32_t delta_b;
    uint32_t delta_r;
    int32_t  energy_mod;
} CellDelta;
