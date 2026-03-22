#pragma once

/*
 * Canvas OS — BlackHole: pattern compression
 * Detects and compresses idle/repeating cell patterns
 * DK-2: integer-only arithmetic
 * DK-5: FNV-1a for region hashing
 */

#include <stdint.h>
#include <stdbool.h>

#define BH_MAX_ENTRIES   1024
#define BH_MIN_RUN       8     /* min run length to compress */

typedef struct {
    uint32_t start_idx;    /* DK-3: ascending cell index */
    uint32_t length;
    uint32_t pattern;      /* packed ABGR of repeating cell */
    uint64_t hash;         /* FNV-1a of uncompressed region */
    bool     active;
} BhEntry;

typedef struct {
    BhEntry  entries[BH_MAX_ENTRIES];
    int      count;
    uint64_t total_cells_compressed;
} BlackHole;

/* Lifecycle */
void bh_init(BlackHole* bh);
void bh_reset(BlackHole* bh);

/* Compression */
int  bh_compress(BlackHole* bh, const uint32_t* cells, uint32_t n_cells,
                 uint32_t base_idx);
bool bh_decompress(const BlackHole* bh, int entry_id,
                   uint32_t* out, int cap);

/* Query */
int  bh_find(const BlackHole* bh, uint32_t cell_idx);
int  bh_count(const BlackHole* bh);
uint64_t bh_total_compressed(const BlackHole* bh);

/* JSON */
int  bh_to_json(const BlackHole* bh, char* buf, int cap);
