#pragma once

/*
 * Canvas OS — WhiteHole: 65536-record circular event log
 * WH records engine events in ascending tick order (DK-3 derived)
 */

#include <stdint.h>
#include <stdbool.h>

#define WH_CAPACITY     65536
#define WH_MSG_LEN      64

typedef enum {
    WH_EVT_TICK      = 0,
    WH_EVT_CELL      = 1,
    WH_EVT_BRANCH    = 2,
    WH_EVT_GATE      = 3,
    WH_EVT_SPAWN     = 4,
    WH_EVT_KILL      = 5,
    WH_EVT_SHELL     = 6,
    WH_EVT_TIMEWARP  = 7,
    WH_EVT_HASH      = 8
} WhEventType;

typedef struct {
    uint64_t    tick;
    WhEventType type;
    uint32_t    cell_index;   /* relevant cell, or 0 */
    uint32_t    value;        /* event-specific value */
    char        msg[WH_MSG_LEN];
} WhRecord;

typedef struct {
    WhRecord records[WH_CAPACITY];
    uint32_t head;    /* write pointer (wraps) */
    uint32_t total;   /* total records ever written */
} WhiteHole;

/* Lifecycle */
void wh_init(WhiteHole* wh);
void wh_reset(WhiteHole* wh);

/* Write */
void wh_log(WhiteHole* wh, uint64_t tick, WhEventType type,
            uint32_t cell_index, uint32_t value, const char* msg);

/* Read (relative from newest backwards) */
bool wh_read(const WhiteHole* wh, uint32_t offset, WhRecord* out);
uint32_t wh_count(const WhiteHole* wh);

/* JSON serialise (last N records) */
int wh_to_json(const WhiteHole* wh, int last_n, char* buf, int cap);
