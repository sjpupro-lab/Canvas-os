/*
 * Canvas OS — CanvasFS file I/O bridge
 */

#include "canvasos.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* Virtual path prefix */
#define CANVAS_FS_PREFIX "/dev/canvas"

/* Read a virtual canvas file:
 * /dev/canvas/cell/X/Y  -> JSON of cell
 * /dev/canvas/status    -> engine status JSON
 * /dev/canvas/hash      -> hex hash
 */
int canvas_fs_read(EngineContext* ctx, const char* path,
                   char* buf, int cap) {
    if (!ctx || !path || !buf || cap <= 0) return -1;

    /* /dev/canvas/status */
    if (strcmp(path, "/dev/canvas/status") == 0) {
        return engctx_get_status(ctx, buf, cap);
    }

    /* /dev/canvas/hash */
    if (strcmp(path, "/dev/canvas/hash") == 0) {
        uint64_t h = dk_canvas_hash(ctx);
        return snprintf(buf, (size_t)cap, "%016llx", (unsigned long long)h);
    }

    /* /dev/canvas/cell/X/Y */
    const char* cell_prefix = "/dev/canvas/cell/";
    if (strncmp(path, cell_prefix, strlen(cell_prefix)) == 0) {
        const char* rest = path + strlen(cell_prefix);
        int x = 0, y = 0;
        if (sscanf(rest, "%d/%d", &x, &y) == 2) {
            Cell c = engctx_get_cell(ctx, x, y);
            return snprintf(buf, (size_t)cap,
                            "{\"x\":%d,\"y\":%d,\"r\":%d,\"g\":%d,\"b\":%d,\"a\":%d}",
                            x, y, c.r, c.g, c.b, c.a);
        }
    }

    snprintf(buf, (size_t)cap, "error: path not found: %s", path);
    return -1;
}

/* Write to virtual canvas:
 * /dev/null            -> discard
 * /dev/canvas/cell/X/Y -> write cell (format: "r g b a")
 */
int canvas_fs_write(EngineContext* ctx, const char* path,
                    const char* data, int data_len) {
    if (!ctx || !path || !data) return -1;
    (void)data_len;

    if (strcmp(path, "/dev/null") == 0) return 0;

    const char* cell_prefix = "/dev/canvas/cell/";
    if (strncmp(path, cell_prefix, strlen(cell_prefix)) == 0) {
        const char* rest = path + strlen(cell_prefix);
        int x = 0, y = 0;
        if (sscanf(rest, "%d/%d", &x, &y) == 2) {
            int r = 0, g = 0, b = 0, a = 0;
            sscanf(data, "%d %d %d %d", &r, &g, &b, &a);
            Cell c = {
                (uint8_t)dk_clamp(a, 0, 255),
                (uint8_t)dk_clamp(b, 0, 255),
                (uint8_t)dk_clamp(g, 0, 255),
                (uint8_t)dk_clamp(r, 0, 255)
            };
            engctx_set_cell(ctx, x, y, c);
            return 0;
        }
    }
    return -1;
}
