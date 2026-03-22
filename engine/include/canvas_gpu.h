#pragma once

/*
 * Canvas OS — GPU Interface (Phase 7+ target)
 * Currently: CPU fallback stub only (SJ_GPU=0)
 * DK-2 compliant: integer-only, no float/double
 */

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* GPU backend selector */
#ifndef SJ_GPU
#define SJ_GPU 0   /* 0 = CPU stub, 1 = OpenCL, 2 = Vulkan */
#endif

typedef struct GpuCtx GpuCtx;

/* ─── Lifecycle ─────────────────────────────────────────── */
GpuCtx* gpu_init(void);
void    gpu_shutdown(GpuCtx* g);
bool    gpu_is_available(void);   /* false on stub */

/* ─── Tile upload ────────────────────────────────────────── */
/* Tiles are integer ABGR packed cells */
bool gpu_upload_tiles(GpuCtx* g, const uint32_t* tiles, int w, int h);

/* ─── Active set scan ───────────────────────────────────── */
/* Fills out[] with cell_indices of active (non-zero) cells */
int  gpu_scan_active_set(GpuCtx* g, uint32_t* out, int cap);

/* ─── BlackHole summarise ───────────────────────────────── */
/* Compress idle tile regions into BH summary */
typedef struct {
    uint32_t region_start;
    uint32_t region_len;
    uint64_t hash;        /* FNV-1a of region (64-bit, DK-5) */
} BhSummary;

int  gpu_bh_summarize_idle(GpuCtx* g, BhSummary* out, int cap);

/* ─── Delta tile merge ──────────────────────────────────── */
bool gpu_merge_delta_tiles(GpuCtx* g, const uint32_t* delta, int n);

/* ─── Stats ──────────────────────────────────────────────── */
typedef struct {
    int     backend;       /* 0=CPU, 1=OpenCL, 2=Vulkan */
    int     tiles_uploaded;
    int     active_cells;
    uint64_t scan_cycles;
} GpuStats;

GpuStats gpu_get_stats(const GpuCtx* g);
