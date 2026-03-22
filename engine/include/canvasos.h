#pragma once

/*
 * Canvas OS — Core Engine Header
 * DK-1: Tick Boundary
 * DK-2: Integer-only arithmetic (NO float/double)
 * DK-3: cell_index ascending order
 * DK-4: Clamp operations
 * DK-5: FNV-1a hashing
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* ─── Canvas Dimensions ─────────────────────────────────── */
#define CANVAS_W        1024
#define CANVAS_H        1024
#define CANVAS_CELLS    (CANVAS_W * CANVAS_H)

/* ─── DK-2: Integer types only ──────────────────────────── */
typedef uint64_t  Tick;
typedef uint32_t  CellIndex;   /* DK-3: ascending index */
typedef uint32_t  BranchId;
typedef uint32_t  GateId;

/* ─── Cell ──────────────────────────────────────────────── */
typedef struct {
    uint8_t a;   /* alpha / opcode */
    uint8_t b;   /* blue           */
    uint8_t g;   /* green          */
    uint8_t r;   /* red            */
} Cell;

static inline CellIndex cell_idx(int x, int y) {
    return (CellIndex)(y * CANVAS_W + x);
}

/* ─── Vis Mode ──────────────────────────────────────────── */
typedef enum {
    VIS_ABGR     = 0,
    VIS_ENERGY   = 1,
    VIS_OPCODE   = 2,
    VIS_LANE     = 3,
    VIS_ACTIVITY = 4
} VisMode;

/* ─── Engine Context ────────────────────────────────────── */
typedef struct EngineContext EngineContext;

/* Lifecycle */
EngineContext* engctx_create(void);
void           engctx_destroy(EngineContext* ctx);
bool           engctx_init(EngineContext* ctx, const char* data_dir);
void           engctx_shutdown(EngineContext* ctx);

/* DK-1: Tick */
void           engctx_tick(EngineContext* ctx);
Tick           engctx_tick_n(EngineContext* ctx, uint32_t n);
Tick           engctx_get_tick(const EngineContext* ctx);

/* DK-5: FNV-1a */
uint64_t       dk_fnv1a(const uint8_t* data, size_t len);
uint64_t       dk_canvas_hash(const EngineContext* ctx);

/* Cell access */
Cell           engctx_get_cell(const EngineContext* ctx, int x, int y);
void           engctx_set_cell(EngineContext* ctx, int x, int y, Cell c);
Cell*          engctx_canvas_ptr(EngineContext* ctx);

/* Status / JSON */
int            engctx_get_status(const EngineContext* ctx, char* buf, int cap);
int            engctx_get_branch_id(const EngineContext* ctx);
int            engctx_get_open_gates(const EngineContext* ctx);
VisMode        engctx_get_vis_mode(const EngineContext* ctx);
void           engctx_set_vis_mode(EngineContext* ctx, VisMode m);

/* BMP export */
bool           engctx_export_bmp(const EngineContext* ctx, const char* path);

/* ─── DK-4: Clamp ───────────────────────────────────────── */
static inline int32_t dk_clamp(int32_t v, int32_t lo, int32_t hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}
static inline uint8_t dk_clamp8(int32_t v) {
    return (uint8_t)dk_clamp(v, 0, 255);
}

/* ─── Viewport ──────────────────────────────────────────── */
typedef struct {
    int x, y, w, h;
    int cell_px;
} Viewport;

void engctx_set_viewport(EngineContext* ctx, int x, int y, int w, int h, int cell_px);
