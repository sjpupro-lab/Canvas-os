#pragma once

/*
 * Canvas OS — Scheduler Header
 * PROC_MAX=64: Static process limit (bottleneck, future: dynamic expansion)
 */

#include <stdint.h>
#include <stdbool.h>

#define PROC_MAX        64
#define PROC_NAME_LEN   32

typedef uint32_t ProcId;

typedef enum {
    PROC_STATE_FREE    = 0,
    PROC_STATE_RUNNING = 1,
    PROC_STATE_BLOCKED = 2,
    PROC_STATE_ZOMBIE  = 3
} ProcState;

typedef struct {
    ProcId     pid;
    ProcId     parent_pid;
    ProcState  state;
    char       name[PROC_NAME_LEN];
    uint64_t   ticks_alive;
    int        exit_code;
    int        pipe_in;    /* fd: read  side of stdin  pipe */
    int        pipe_out;   /* fd: write side of stdout pipe */
} ProcEntry;

typedef struct {
    ProcEntry  table[PROC_MAX];
    int        count;       /* active process count */
    ProcId     next_pid;
} Scheduler;

/* Lifecycle */
void    sched_init(Scheduler* s);
void    sched_reset(Scheduler* s);

/* Process management */
ProcId  sched_spawn(Scheduler* s, ProcId parent, const char* name);
bool    sched_kill(Scheduler* s, ProcId pid);
bool    sched_kill_name(Scheduler* s, const char* name);
void    sched_reap(Scheduler* s);              /* collect zombies */
void    sched_tick_all(Scheduler* s);          /* increment ticks_alive */

/* Query */
ProcEntry* sched_find(Scheduler* s, ProcId pid);
ProcEntry* sched_find_name(Scheduler* s, const char* name);
int        sched_count(const Scheduler* s);
int        sched_list(const Scheduler* s, ProcEntry* out, int cap);

/* Serialise to JSON array */
int        sched_to_json(const Scheduler* s, char* buf, int cap);
