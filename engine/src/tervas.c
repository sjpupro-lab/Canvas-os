/*
 * Canvas OS — Tervas ANSI Canvas Terminal Renderer
 * DK-2: integer-only, DK-3: ascending scan
 */

#include "canvasos.h"
#include <string.h>
#include <stdio.h>

#define TERVAS_MAX_W 256
#define TERVAS_MAX_H 64

/* Render viewport to ANSI escape string */
int tervas_render(const EngineContext* ctx, const Viewport* vp,
                  char* out, int out_cap) {
    if (!ctx || !out || out_cap <= 0) return 0;

    /* Use provided vp or a default */
    int vx = vp ? vp->x : 0;
    int vy = vp ? vp->y : 0;
    int vw = vp ? vp->w : 80;
    int vh = vp ? vp->h : 24;

    if (vw > TERVAS_MAX_W) vw = TERVAS_MAX_W;
    if (vh > TERVAS_MAX_H) vh = TERVAS_MAX_H;

    int written = 0;
    /* DK-3: scan rows then cols ascending */
    for (int row = 0; row < vh && written < out_cap - 32; row++) {
        for (int col = 0; col < vw && written < out_cap - 32; col++) {
            int cx = vx + col;
            int cy = vy + row;
            Cell c = engctx_get_cell(ctx, cx, cy);

            /* ANSI 24-bit colour background */
            int n = snprintf(out + written, (size_t)(out_cap - written),
                             "\x1b[48;2;%d;%d;%dm ",
                             (int)c.r, (int)c.g, (int)c.b);
            if (n > 0) written += n;
        }
        /* Reset + newline */
        int n = snprintf(out + written, (size_t)(out_cap - written), "\x1b[0m\n");
        if (n > 0) written += n;
    }

    /* Reset at end */
    if (written < out_cap - 4) {
        written += snprintf(out + written, (size_t)(out_cap - written), "\x1b[0m");
    }
    return written;
}

/* ASCII art render (no escape codes) for testing */
int tervas_render_ascii(const EngineContext* ctx, int x, int y,
                        int w, int h, char* out, int out_cap) {
    if (!ctx || !out || out_cap <= 0) return 0;
    int written = 0;
    /* DK-3: ascending scan */
    for (int row = 0; row < h && written < out_cap - 4; row++) {
        for (int col = 0; col < w && written < out_cap - 4; col++) {
            Cell c = engctx_get_cell(ctx, x + col, y + row);
            /* Map energy/R channel to ASCII density */
            int brightness = (int)c.r + (int)c.g + (int)c.b;
            char ch;
            if (brightness == 0)        ch = '.';
            else if (brightness < 128)  ch = '+';
            else if (brightness < 384)  ch = '*';
            else if (brightness < 512)  ch = '#';
            else                        ch = '@';
            out[written++] = ch;
        }
        out[written++] = '\n';
    }
    if (written < out_cap) out[written] = '\0';
    return written;
}
