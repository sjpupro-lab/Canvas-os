/*
 * Canvas OS — WhiteHole: 65536-record circular event log
 */

#include "canvasos_wh.h"
#include <string.h>
#include <stdio.h>

void wh_init(WhiteHole* wh) {
    if (!wh) return;
    memset(wh, 0, sizeof(*wh));
}

void wh_reset(WhiteHole* wh) {
    if (!wh) return;
    wh->head  = 0;
    wh->total = 0;
    memset(wh->records, 0, sizeof(wh->records));
}

void wh_log(WhiteHole* wh, uint64_t tick, WhEventType type,
            uint32_t cell_index, uint32_t value, const char* msg) {
    if (!wh) return;
    WhRecord* r = &wh->records[wh->head % WH_CAPACITY];
    r->tick       = tick;
    r->type       = type;
    r->cell_index = cell_index;
    r->value      = value;
    if (msg) {
        strncpy(r->msg, msg, WH_MSG_LEN - 1);
        r->msg[WH_MSG_LEN - 1] = '\0';
    } else {
        r->msg[0] = '\0';
    }
    wh->head = (wh->head + 1) % WH_CAPACITY;
    wh->total++;
}

uint32_t wh_count(const WhiteHole* wh) {
    if (!wh) return 0;
    uint32_t n = wh->total;
    return (n < WH_CAPACITY) ? n : WH_CAPACITY;
}

bool wh_read(const WhiteHole* wh, uint32_t offset, WhRecord* out) {
    if (!wh || !out) return false;
    uint32_t cnt = wh_count(wh);
    if (offset >= cnt) return false;

    /* newest is at (head - 1), offset 0 = newest */
    uint32_t idx;
    if (wh->total <= WH_CAPACITY) {
        /* buffer not wrapped: entries are 0..head-1, newest = head-1 */
        if (wh->head == 0) return false;
        idx = (wh->head - 1 + WH_CAPACITY - offset) % WH_CAPACITY;
    } else {
        /* buffer wrapped: newest = head-1 (mod CAP) */
        idx = (wh->head - 1 + WH_CAPACITY - offset) % WH_CAPACITY;
    }
    *out = wh->records[idx];
    return true;
}

int wh_to_json(const WhiteHole* wh, int last_n, char* buf, int cap) {
    if (!wh || !buf || cap <= 0) return 0;
    uint32_t cnt = wh_count(wh);
    if ((uint32_t)last_n > cnt) last_n = (int)cnt;

    int written = 0;
    written += snprintf(buf + written, (size_t)(cap - written), "[");

    for (int i = last_n - 1; i >= 0 && written < cap - 2; i--) {
        WhRecord r;
        if (!wh_read(wh, (uint32_t)i, &r)) continue;
        int n = snprintf(buf + written, (size_t)(cap - written),
            "%s{\"tick\":%llu,\"type\":%d,\"cell\":%u,\"val\":%u,\"msg\":\"%s\"}",
            (i == last_n - 1) ? "" : ",",
            (unsigned long long)r.tick,
            (int)r.type,
            r.cell_index,
            r.value,
            r.msg);
        if (n < 0 || written + n >= cap - 2) break;
        written += n;
    }

    if (written < cap - 1) {
        buf[written++] = ']';
        buf[written]   = '\0';
    }
    return written;
}
