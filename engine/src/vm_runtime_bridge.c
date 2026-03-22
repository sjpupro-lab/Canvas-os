/*
 * Canvas OS — VM Runtime Bridge (IPC pipes + process spawn)
 */

#include "canvasos.h"
#include "canvasos_sched.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* Message types */
typedef enum {
    VM_MSG_SEND  = 1,
    VM_MSG_RECV  = 2,
    VM_MSG_SPAWN = 3,
    VM_MSG_KILL  = 4,
    VM_MSG_ACK   = 5
} VmMsgType;

typedef struct {
    VmMsgType type;
    ProcId    src;
    ProcId    dst;
    uint32_t  len;
    uint8_t   data[256];
} VmMessage;

/* Simple in-memory message queue */
#define VM_QUEUE_CAP 64

typedef struct {
    VmMessage queue[VM_QUEUE_CAP];
    int       head;
    int       tail;
    int       count;
} VmMsgQueue;

static VmMsgQueue g_vmq;

void vm_bridge_init(void) {
    memset(&g_vmq, 0, sizeof(g_vmq));
}

int vm_send(ProcId src, ProcId dst, const uint8_t* data, int len) {
    if (g_vmq.count >= VM_QUEUE_CAP) return -1;
    VmMessage* m = &g_vmq.queue[g_vmq.tail];
    m->type = VM_MSG_SEND;
    m->src  = src;
    m->dst  = dst;
    m->len  = (uint32_t)(len > 256 ? 256 : len);
    memcpy(m->data, data, m->len);
    g_vmq.tail = (g_vmq.tail + 1) % VM_QUEUE_CAP;
    g_vmq.count++;
    return 0;
}

int vm_recv(ProcId dst, uint8_t* out, int cap) {
    if (g_vmq.count == 0) return 0;
    /* Find first message for dst */
    for (int i = 0; i < VM_QUEUE_CAP; i++) {
        int idx = (g_vmq.head + i) % VM_QUEUE_CAP;
        VmMessage* m = &g_vmq.queue[idx];
        if (m->dst == dst) {
            int copy = (int)m->len;
            if (copy > cap) copy = cap;
            memcpy(out, m->data, (size_t)copy);
            /* Remove from queue (simplified: mark as consumed) */
            m->len = 0;
            m->type = 0;
            g_vmq.count--;
            return copy;
        }
    }
    return 0;
}

int vm_queue_depth(void) {
    return g_vmq.count;
}
