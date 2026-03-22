#pragma once

/*
 * Canvas OS — JSON utility helpers
 * Shared across all engine modules that emit JSON.
 */

#include <stddef.h>

/*
 * json_escape_str — copies `src` into `dst` with JSON string escaping applied.
 * Escapes:  " → \"   \ → \\   control chars (0x00-0x1F) are dropped.
 * Returns the number of characters written (not counting the NUL terminator).
 * `dst_cap` must be > 0; output is always NUL-terminated.
 */
static inline int json_escape_str(const char* src, char* dst, int dst_cap) {
    int w = 0;
    for (const char* p = src; *p && w < dst_cap - 1; p++) {
        unsigned char c = (unsigned char)*p;
        if (c == '"' || c == '\\') {
            if (w + 2 >= dst_cap) break;
            dst[w++] = '\\';
            dst[w++] = (char)c;
        } else if (c < 0x20) {
            /* drop control characters */
        } else {
            dst[w++] = (char)c;
        }
    }
    if (w < dst_cap) dst[w] = '\0';
    return w;
}
