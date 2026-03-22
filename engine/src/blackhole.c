/*
 * Canvas OS — BlackHole: pattern compression
 * DK-2: integer-only arithmetic
 * DK-5: FNV-1a for region hashing
 */

#include "canvasos_bh.h"
#include "canvasos.h"  /* for dk_fnv1a */
#include <string.h>
#include <stdio.h>

void bh_init(BlackHole* bh) {
    if (!bh) return;
    memset(bh, 0, sizeof(*bh));
}

void bh_reset(BlackHole* bh) {
    if (!bh) return;
    memset(bh, 0, sizeof(*bh));
}

int bh_compress(BlackHole* bh, const uint32_t* cells, uint32_t n_cells,
                uint32_t base_idx) {
    if (!bh || !cells || n_cells == 0) return 0;
    int added = 0;

    uint32_t i = 0;
    while (i < n_cells) {
        /* Find run of identical cells */
        uint32_t run_start = i;
        uint32_t run_val   = cells[i];
        uint32_t j = i + 1;
        while (j < n_cells && cells[j] == run_val) j++;
        uint32_t run_len = j - run_start;

        if (run_len >= BH_MIN_RUN && bh->count < BH_MAX_ENTRIES) {
            BhEntry* e = &bh->entries[bh->count];
            e->start_idx = base_idx + run_start;
            e->length    = run_len;
            e->pattern   = run_val;
            e->hash      = dk_fnv1a((const uint8_t*)&cells[run_start],
                                    (size_t)run_len * sizeof(uint32_t));
            e->active    = true;
            bh->count++;
            bh->total_cells_compressed += run_len;
            added++;
        }
        i = j;
    }
    return added;
}

bool bh_decompress(const BlackHole* bh, int entry_id,
                   uint32_t* out, int cap) {
    if (!bh || !out || entry_id < 0 || entry_id >= bh->count) return false;
    const BhEntry* e = &bh->entries[entry_id];
    if (!e->active) return false;
    int fill = (int)e->length;
    if (fill > cap) fill = cap;
    for (int i = 0; i < fill; i++) out[i] = e->pattern;
    return true;
}

int bh_find(const BlackHole* bh, uint32_t cell_idx) {
    if (!bh) return -1;
    for (int i = 0; i < bh->count; i++) {
        const BhEntry* e = &bh->entries[i];
        if (!e->active) continue;
        if (cell_idx >= e->start_idx &&
            cell_idx < e->start_idx + e->length) {
            return i;
        }
    }
    return -1;
}

int bh_count(const BlackHole* bh) {
    if (!bh) return 0;
    return bh->count;
}

uint64_t bh_total_compressed(const BlackHole* bh) {
    if (!bh) return 0;
    return bh->total_cells_compressed;
}

int bh_to_json(const BlackHole* bh, char* buf, int cap) {
    if (!bh || !buf || cap <= 0) return 0;
    int written = 0;
    written += snprintf(buf + written, (size_t)(cap - written),
        "{\"count\":%d,\"total_compressed\":%llu,\"entries\":[",
        bh->count, (unsigned long long)bh->total_cells_compressed);

    for (int i = 0; i < bh->count && written < cap - 4; i++) {
        const BhEntry* e = &bh->entries[i];
        int n = snprintf(buf + written, (size_t)(cap - written),
            "%s{\"start\":%u,\"len\":%u,\"pat\":%u,\"hash\":\"%016llx\"}",
            i ? "," : "",
            e->start_idx, e->length, e->pattern,
            (unsigned long long)e->hash);
        if (n < 0 || written + n >= cap - 4) break;
        written += n;
    }

    if (written < cap - 2) {
        buf[written++] = ']';
        buf[written++] = '}';
        buf[written]   = '\0';
    }
    return written;
}
