/*
 * Canvas OS — Scheduler (PROC_MAX=64)
 */

#include "canvasos_sched.h"
#include "canvasos_json.h"
#include <string.h>
#include <stdio.h>

void sched_init(Scheduler* s) {
    if (!s) return;
    memset(s, 0, sizeof(*s));
    s->next_pid = 1;
    /* Mark all slots as free */
    for (int i = 0; i < PROC_MAX; i++) {
        s->table[i].state = PROC_STATE_FREE;
        s->table[i].pipe_in  = -1;
        s->table[i].pipe_out = -1;
    }
}

void sched_reset(Scheduler* s) {
    sched_init(s);
}

ProcId sched_spawn(Scheduler* s, ProcId parent, const char* name) {
    if (!s) return 0;
    if (s->count >= PROC_MAX) return 0;  /* PROC_MAX=64 bottleneck */

    /* Find free slot */
    for (int i = 0; i < PROC_MAX; i++) {
        if (s->table[i].state == PROC_STATE_FREE) {
            ProcEntry* p = &s->table[i];
            p->pid        = s->next_pid++;
            p->parent_pid = parent;
            p->state      = PROC_STATE_RUNNING;
            p->ticks_alive = 0;
            p->exit_code  = 0;
            p->pipe_in    = -1;
            p->pipe_out   = -1;
            if (name) {
                strncpy(p->name, name, PROC_NAME_LEN - 1);
                p->name[PROC_NAME_LEN - 1] = '\0';
            } else {
                p->name[0] = '\0';
            }
            s->count++;
            return p->pid;
        }
    }
    return 0;
}

bool sched_kill(Scheduler* s, ProcId pid) {
    if (!s || pid == 0) return false;
    for (int i = 0; i < PROC_MAX; i++) {
        if (s->table[i].state != PROC_STATE_FREE &&
            s->table[i].pid == pid) {
            s->table[i].state     = PROC_STATE_ZOMBIE;
            s->table[i].exit_code = 1;
            return true;
        }
    }
    return false;
}

bool sched_kill_name(Scheduler* s, const char* name) {
    if (!s || !name) return false;
    bool killed = false;
    for (int i = 0; i < PROC_MAX; i++) {
        if (s->table[i].state == PROC_STATE_RUNNING &&
            strncmp(s->table[i].name, name, PROC_NAME_LEN) == 0) {
            s->table[i].state     = PROC_STATE_ZOMBIE;
            s->table[i].exit_code = 1;
            killed = true;
        }
    }
    return killed;
}

void sched_reap(Scheduler* s) {
    if (!s) return;
    for (int i = 0; i < PROC_MAX; i++) {
        if (s->table[i].state == PROC_STATE_ZOMBIE) {
            s->table[i].state = PROC_STATE_FREE;
            if (s->count > 0) s->count--;
        }
    }
}

void sched_tick_all(Scheduler* s) {
    if (!s) return;
    for (int i = 0; i < PROC_MAX; i++) {
        if (s->table[i].state == PROC_STATE_RUNNING) {
            s->table[i].ticks_alive++;
        }
    }
}

ProcEntry* sched_find(Scheduler* s, ProcId pid) {
    if (!s) return NULL;
    for (int i = 0; i < PROC_MAX; i++) {
        if (s->table[i].state != PROC_STATE_FREE &&
            s->table[i].pid == pid) {
            return &s->table[i];
        }
    }
    return NULL;
}

ProcEntry* sched_find_name(Scheduler* s, const char* name) {
    if (!s || !name) return NULL;
    for (int i = 0; i < PROC_MAX; i++) {
        if (s->table[i].state != PROC_STATE_FREE &&
            strncmp(s->table[i].name, name, PROC_NAME_LEN) == 0) {
            return &s->table[i];
        }
    }
    return NULL;
}

int sched_count(const Scheduler* s) {
    if (!s) return 0;
    return s->count;
}

int sched_list(const Scheduler* s, ProcEntry* out, int cap) {
    if (!s || !out || cap <= 0) return 0;
    int n = 0;
    for (int i = 0; i < PROC_MAX && n < cap; i++) {
        if (s->table[i].state != PROC_STATE_FREE) {
            out[n++] = s->table[i];
        }
    }
    return n;
}

int sched_to_json(const Scheduler* s, char* buf, int cap) {
    if (!s || !buf || cap <= 0) return 0;
    int written = snprintf(buf, (size_t)cap, "[");
    bool first = true;
    for (int i = 0; i < PROC_MAX && written < cap - 4; i++) {
        const ProcEntry* p = &s->table[i];
        if (p->state == PROC_STATE_FREE) continue;
        char esc_name[PROC_NAME_LEN * 2];
        json_escape_str(p->name, esc_name, sizeof(esc_name));
        int n = snprintf(buf + written, (size_t)(cap - written),
            "%s{\"pid\":%u,\"name\":\"%s\",\"state\":%d,\"ticks\":%llu}",
            first ? "" : ",",
            p->pid, esc_name, (int)p->state,
            (unsigned long long)p->ticks_alive);
        if (n < 0 || written + n >= cap - 4) break;
        written += n;
        first = false;
    }
    if (written < cap - 1) {
        buf[written++] = ']';
        buf[written]   = '\0';
    }
    return written;
}
