/*
 * Canvas OS — GPU Stub (CPU fallback)
 * SJ_GPU=0 — CPU-only, no OpenCL/Vulkan
 * DK-2: integer-only arithmetic
 */

#include "canvas_gpu.h"
#include "canvasos.h"
#include <stdlib.h>
#include <string.h>

struct GpuCtx {
    int      backend;      /* always 0 (CPU stub) */
    int      tiles_w;
    int      tiles_h;
    uint32_t* tile_buf;
    int      tile_buf_n;
    int      tiles_uploaded;
    int      active_cells;
    uint64_t scan_cycles;
};

GpuCtx* gpu_init(void) {
    GpuCtx* g = (GpuCtx*)calloc(1, sizeof(GpuCtx));
    if (!g) return NULL;
    g->backend = 0; /* CPU stub */
    return g;
}

void gpu_shutdown(GpuCtx* g) {
    if (!g) return;
    if (g->tile_buf) free(g->tile_buf);
    free(g);
}

bool gpu_is_available(void) {
    return false; /* stub: no GPU */
}

bool gpu_upload_tiles(GpuCtx* g, const uint32_t* tiles, int w, int h) {
    if (!g || !tiles || w <= 0 || h <= 0) return false;
    int n = w * h;
    if (n != g->tile_buf_n) {
        free(g->tile_buf);
        g->tile_buf   = (uint32_t*)malloc((size_t)n * sizeof(uint32_t));
        g->tile_buf_n = n;
    }
    if (!g->tile_buf) return false;
    memcpy(g->tile_buf, tiles, (size_t)n * sizeof(uint32_t));
    g->tiles_w        = w;
    g->tiles_h        = h;
    g->tiles_uploaded = n;
    return true;
}

int gpu_scan_active_set(GpuCtx* g, uint32_t* out, int cap) {
    if (!g || !out || cap <= 0 || !g->tile_buf) return 0;
    int found = 0;
    /* DK-3: ascending scan order */
    for (int i = 0; i < g->tile_buf_n && found < cap; i++) {
        if (g->tile_buf[i] != 0) {
            out[found++] = (uint32_t)i;
        }
    }
    g->active_cells = found;
    g->scan_cycles += (uint64_t)g->tile_buf_n;
    return found;
}

int gpu_bh_summarize_idle(GpuCtx* g, BhSummary* out, int cap) {
    if (!g || !out || cap <= 0 || !g->tile_buf) return 0;
    int n = 0;
    /* Find runs of zero tiles */
    int i = 0;
    while (i < g->tile_buf_n && n < cap) {
        if (g->tile_buf[i] == 0) {
            int start = i;
            while (i < g->tile_buf_n && g->tile_buf[i] == 0) i++;
            int len = i - start;
            if (len >= 4) {
                out[n].region_start = (uint32_t)start;
                out[n].region_len   = (uint32_t)len;
                out[n].hash         = dk_fnv1a(
                    (const uint8_t*)(g->tile_buf + start),
                    (size_t)len * sizeof(uint32_t));
                n++;
            }
        } else {
            i++;
        }
    }
    return n;
}

bool gpu_merge_delta_tiles(GpuCtx* g, const uint32_t* delta, int n) {
    if (!g || !delta || n <= 0 || !g->tile_buf) return false;
    /* Apply delta (XOR merge, integer-only, DK-2) */
    for (int i = 0; i < n && i < g->tile_buf_n; i++) {
        g->tile_buf[i] ^= delta[i];
    }
    return true;
}

GpuStats gpu_get_stats(const GpuCtx* g) {
    GpuStats s = {0, 0, 0, 0};
    if (!g) return s;
    s.backend        = g->backend;
    s.tiles_uploaded = g->tiles_uploaded;
    s.active_cells   = g->active_cells;
    s.scan_cycles    = g->scan_cycles;
    return s;
}
