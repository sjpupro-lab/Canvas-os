/*
 * Canvas OS — Core Engine (engine.c)
 * DK-1: Tick boundary
 * DK-2: Integer-only arithmetic
 * DK-3: cell_index ascending order
 * DK-4: Clamp
 * DK-5: FNV-1a hash
 */

#include "canvasos.h"
#include "canvasos_sched.h"
#include "canvasos_ruletable.h"
#include "canvasos_wh.h"
#include "canvasos_bh.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>

/* ─── Internal Engine State ─────────────────────────────── */
struct EngineContext {
    /* DK-1: tick counter */
    Tick        tick;

    /* Canvas memory (DK-3: indexed ascending) */
    Cell        canvas[CANVAS_CELLS];

    /* Energy map (DK-2: integer) */
    int32_t     energy[CANVAS_CELLS];

    /* Branch state */
    BranchId    branch_id;

    /* Vis mode */
    VisMode     vis_mode;

    /* Gate bitmap: 64 gates, 1 bit each */
    uint64_t    gate_mask;

    /* Viewport */
    Viewport    viewport;

    /* WhiteHole event log */
    WhiteHole   wh;

    /* BlackHole compression */
    BlackHole   bh;

    /* Rule Table */
    RuleTable   ruletable;

    /* Scheduler */
    Scheduler   sched;

    /* data directory */
    char        data_dir[256];

    /* dirty flag for render */
    bool        dirty;

    /* initialised flag */
    bool        initialised;
};

/* ─── DK-5: FNV-1a hash ─────────────────────────────────── */
#define FNV_OFFSET_BASIS UINT64_C(14695981039346656037)
#define FNV_PRIME        UINT64_C(1099511628211)

uint64_t dk_fnv1a(const uint8_t* data, size_t len) {
    uint64_t h = FNV_OFFSET_BASIS;
    for (size_t i = 0; i < len; i++) {
        h ^= (uint64_t)data[i];
        h *= FNV_PRIME;
    }
    return h;
}

uint64_t dk_canvas_hash(const EngineContext* ctx) {
    if (!ctx) return 0;
    /* DK-3: scan ascending cell_index order */
    return dk_fnv1a((const uint8_t*)ctx->canvas,
                    sizeof(Cell) * CANVAS_CELLS);
}

/* ─── Lifecycle ─────────────────────────────────────────── */
EngineContext* engctx_create(void) {
    EngineContext* ctx = (EngineContext*)calloc(1, sizeof(EngineContext));
    if (!ctx) return NULL;

    ctx->tick         = 0;
    ctx->branch_id    = 0;
    ctx->vis_mode     = VIS_ABGR;
    ctx->gate_mask    = 0;
    ctx->dirty        = false;
    ctx->initialised  = false;

    memset(ctx->canvas, 0, sizeof(ctx->canvas));
    memset(ctx->energy, 0, sizeof(ctx->energy));

    wh_init(&ctx->wh);
    bh_init(&ctx->bh);
    ruletable_init(&ctx->ruletable);
    sched_init(&ctx->sched);

    ctx->viewport.x       = 0;
    ctx->viewport.y       = 0;
    ctx->viewport.w       = 256;
    ctx->viewport.h       = 256;
    ctx->viewport.cell_px = 2;

    return ctx;
}

void engctx_destroy(EngineContext* ctx) {
    if (!ctx) return;
    free(ctx);
}

bool engctx_init(EngineContext* ctx, const char* data_dir) {
    if (!ctx) return false;
    if (data_dir) {
        strncpy(ctx->data_dir, data_dir, sizeof(ctx->data_dir) - 1);
        ctx->data_dir[sizeof(ctx->data_dir) - 1] = '\0';
    }
    ctx->initialised = true;
    /* Log init event */
    wh_log(&ctx->wh, 0, WH_EVT_TICK, 0, 0, "init");
    return true;
}

void engctx_shutdown(EngineContext* ctx) {
    if (!ctx) return;
    ctx->initialised = false;
}

/* ─── DK-1: Tick ────────────────────────────────────────── */
void engctx_tick(EngineContext* ctx) {
    if (!ctx) return;
    ctx->tick++;
    ctx->dirty = true;

    /* DK-3: ascending energy decay across all cells */
    for (CellIndex i = 0; i < CANVAS_CELLS; i++) {
        if (ctx->energy[i] > 0) {
            ctx->energy[i]--;  /* DK-2: integer only, DK-4: clamp implicit */
        }
    }

    sched_tick_all(&ctx->sched);
    wh_log(&ctx->wh, ctx->tick, WH_EVT_TICK, 0, (uint32_t)ctx->tick, "tick");
}

Tick engctx_tick_n(EngineContext* ctx, uint32_t n) {
    if (!ctx) return 0;
    for (uint32_t i = 0; i < n; i++) {
        engctx_tick(ctx);
    }
    return ctx->tick;
}

Tick engctx_get_tick(const EngineContext* ctx) {
    if (!ctx) return 0;
    return ctx->tick;
}

/* ─── Cell Access ───────────────────────────────────────── */
Cell engctx_get_cell(const EngineContext* ctx, int x, int y) {
    Cell zero = {0, 0, 0, 0};
    if (!ctx) return zero;
    if (x < 0 || x >= CANVAS_W || y < 0 || y >= CANVAS_H) return zero;
    return ctx->canvas[cell_idx(x, y)];
}

void engctx_set_cell(EngineContext* ctx, int x, int y, Cell c) {
    if (!ctx) return;
    if (x < 0 || x >= CANVAS_W || y < 0 || y >= CANVAS_H) return;
    CellIndex idx = cell_idx(x, y);
    ctx->canvas[idx] = c;
    ctx->energy[idx] = 255; /* DK-4: clamped to uint8 range */
    ctx->dirty = true;
    wh_log(&ctx->wh, ctx->tick, WH_EVT_CELL, idx, 0, "set");
}

Cell* engctx_canvas_ptr(EngineContext* ctx) {
    if (!ctx) return NULL;
    return ctx->canvas;
}

/* ─── Viewport ──────────────────────────────────────────── */
void engctx_set_viewport(EngineContext* ctx, int x, int y,
                         int w, int h, int cell_px) {
    if (!ctx) return;
    ctx->viewport.x       = x;
    ctx->viewport.y       = y;
    ctx->viewport.w       = w;
    ctx->viewport.h       = h;
    ctx->viewport.cell_px = cell_px;
}

/* ─── VisMode ────────────────────────────────────────────── */
VisMode engctx_get_vis_mode(const EngineContext* ctx) {
    if (!ctx) return VIS_ABGR;
    return ctx->vis_mode;
}
void engctx_set_vis_mode(EngineContext* ctx, VisMode m) {
    if (!ctx) return;
    ctx->vis_mode = m;
}

/* ─── Gate ───────────────────────────────────────────────── */
int engctx_get_open_gates(const EngineContext* ctx) {
    if (!ctx) return 0;
    /* popcount of gate_mask */
    uint64_t m = ctx->gate_mask;
    int cnt = 0;
    while (m) { cnt += (int)(m & 1); m >>= 1; }
    return cnt;
}

/* ─── Branch ─────────────────────────────────────────────── */
int engctx_get_branch_id(const EngineContext* ctx) {
    if (!ctx) return 0;
    return (int)ctx->branch_id;
}

/* ─── Status JSON ────────────────────────────────────────── */
int engctx_get_status(const EngineContext* ctx, char* buf, int cap) {
    if (!ctx || !buf || cap <= 0) return 0;
    uint64_t h = dk_canvas_hash(ctx);
    return snprintf(buf, (size_t)cap,
        "{\"tick\":%llu,\"hash\":\"%016llx\","
        "\"branchId\":%d,\"openGates\":%d,\"visMode\":%d}",
        (unsigned long long)ctx->tick,
        (unsigned long long)h,
        engctx_get_branch_id(ctx),
        engctx_get_open_gates(ctx),
        (int)ctx->vis_mode);
}

/* ─── BMP Export ─────────────────────────────────────────── */
bool engctx_export_bmp(const EngineContext* ctx, const char* path) {
    if (!ctx || !path) return false;

    /* BMP header constants (DK-2: integer only) */
    const int W = CANVAS_W;
    const int H = CANVAS_H;
    const int row_bytes = W * 3;
    const int pad = (4 - (row_bytes % 4)) % 4;
    const int stride = row_bytes + pad;
    const int pixel_data_size = stride * H;
    const int file_size = 54 + pixel_data_size;

    FILE* f = fopen(path, "wb");
    if (!f) return false;

    /* BMP file header (14 bytes) */
    uint8_t hdr[54];
    memset(hdr, 0, sizeof(hdr));
    hdr[0] = 'B'; hdr[1] = 'M';
    hdr[2] = (uint8_t)(file_size);
    hdr[3] = (uint8_t)(file_size >> 8);
    hdr[4] = (uint8_t)(file_size >> 16);
    hdr[5] = (uint8_t)(file_size >> 24);
    hdr[10] = 54;  /* pixel data offset */
    /* DIB header (40 bytes) */
    hdr[14] = 40;
    hdr[18] = (uint8_t)(W);
    hdr[19] = (uint8_t)(W >> 8);
    hdr[22] = (uint8_t)(H);
    hdr[23] = (uint8_t)(H >> 8);
    hdr[26] = 1;   /* colour planes */
    hdr[28] = 24;  /* bits per pixel */
    hdr[34] = (uint8_t)(pixel_data_size);
    hdr[35] = (uint8_t)(pixel_data_size >> 8);
    hdr[36] = (uint8_t)(pixel_data_size >> 16);
    hdr[37] = (uint8_t)(pixel_data_size >> 24);

    fwrite(hdr, 1, 54, f);

    /* Pixel data: BMP is bottom-up */
    uint8_t row_buf[CANVAS_W * 3 + 4];
    memset(row_buf, 0, sizeof(row_buf));
    for (int y = H - 1; y >= 0; y--) {
        for (int x = 0; x < W; x++) {
            Cell c = ctx->canvas[cell_idx(x, y)];
            row_buf[x * 3 + 0] = c.b;
            row_buf[x * 3 + 1] = c.g;
            row_buf[x * 3 + 2] = c.r;
        }
        fwrite(row_buf, 1, (size_t)stride, f);
    }
    fclose(f);
    return true;
}
