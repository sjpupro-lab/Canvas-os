/*
 * Canvas OS — PixelCode + 한글코드 parser/VM
 * DK-2: integer-only arithmetic
 */

#include "canvasos.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

/* ─── Opcode table ──────────────────────────────────────── */
typedef enum {
    PC_NOP   = 0x00,
    PC_SET   = 0x01,   /* SET x y r g b a   */
    PC_MOV   = 0x02,   /* MOV x1 y1 x2 y2   */
    PC_ADD   = 0x03,   /* ADD x y dr dg db  */
    PC_HASH  = 0x04,   /* HASH (push hash)  */
    PC_JUMP  = 0x05,   /* JUMP offset       */
    PC_HALT  = 0xFF
} PcOpcode;

/* VM registers (DK-2: all integers) */
typedef struct {
    int32_t  regs[16];
    uint32_t pc;        /* program counter */
    bool     halted;
} PcVM;

typedef struct {
    uint8_t* code;
    int      len;
} PcProgram;

static void pcvm_init(PcVM* vm) {
    if (!vm) return;
    memset(vm, 0, sizeof(*vm));
}

static void pcvm_exec_step(PcVM* vm, const PcProgram* prog,
                            EngineContext* ctx) {
    if (!vm || !prog || !ctx || vm->halted) return;
    if (vm->pc >= (uint32_t)prog->len) { vm->halted = true; return; }

    uint8_t op = prog->code[vm->pc++];
    switch ((PcOpcode)op) {
        case PC_NOP:
            break;

        case PC_SET: {
            if (vm->pc + 5 >= (uint32_t)prog->len) { vm->halted = true; break; }
            int x = prog->code[vm->pc++];
            int y = prog->code[vm->pc++];
            Cell c;
            c.r = prog->code[vm->pc++];
            c.g = prog->code[vm->pc++];
            c.b = prog->code[vm->pc++];
            c.a = prog->code[vm->pc++];
            engctx_set_cell(ctx, x, y, c);
            break;
        }

        case PC_ADD: {
            if (vm->pc + 4 >= (uint32_t)prog->len) { vm->halted = true; break; }
            int x  = prog->code[vm->pc++];
            int y  = prog->code[vm->pc++];
            Cell c = engctx_get_cell(ctx, x, y);
            /* DK-4: clamp */
            c.r = dk_clamp8((int32_t)c.r + (int8_t)prog->code[vm->pc++]);
            c.g = dk_clamp8((int32_t)c.g + (int8_t)prog->code[vm->pc++]);
            c.b = dk_clamp8((int32_t)c.b + (int8_t)prog->code[vm->pc++]);
            engctx_set_cell(ctx, x, y, c);
            break;
        }

        case PC_HASH: {
            /* Compute hash and store in reg[0] (low 32 bits) */
            uint64_t h = dk_canvas_hash(ctx);
            vm->regs[0] = (int32_t)(h & 0xFFFFFFFF);
            vm->regs[1] = (int32_t)((h >> 32) & 0xFFFFFFFF);
            break;
        }

        case PC_HALT:
        default:
            vm->halted = true;
            break;
    }
}

/* Public API */
int pixelcode_exec(EngineContext* ctx, const uint8_t* code, int code_len,
                   char* out, int out_cap) {
    if (!ctx || !code || code_len <= 0) return -1;
    PcVM vm;
    pcvm_init(&vm);
    PcProgram prog = { (uint8_t*)code, code_len };

    int steps = 0;
    while (!vm.halted && steps < 10000) {
        pcvm_exec_step(&vm, &prog, ctx);
        steps++;
    }

    if (out && out_cap > 0) {
        snprintf(out, (size_t)out_cap,
                 "steps=%d halted=%d r0=%d r1=%d",
                 steps, vm.halted, vm.regs[0], vm.regs[1]);
    }
    return steps;
}

/* ─── 한글코드 parser ────────────────────────────────────── */
/* Map Hangul syllable blocks to PixelCode opcodes */
/* U+AC00 to U+D7A3 = 11172 syllables */

#define HANGUL_BASE 0xAC00

static uint8_t hangul_to_opcode(uint32_t cp) {
    if (cp < HANGUL_BASE) return PC_NOP;
    uint32_t idx = cp - HANGUL_BASE;
    return (uint8_t)(idx % 5); /* simple mapping */
}

/* Parse UTF-8 stream for 한글코드 */
int hangulcode_parse(const char* src, uint8_t* out, int out_cap) {
    if (!src || !out || out_cap <= 0) return 0;
    int n = 0;
    const unsigned char* p = (const unsigned char*)src;
    while (*p && n < out_cap) {
        /* 3-byte UTF-8 for Korean (0xE0..0xEF) */
        if ((p[0] & 0xF0) == 0xE0 && p[1] && p[2]) {
            uint32_t cp = ((uint32_t)(p[0] & 0x0F) << 12)
                        | ((uint32_t)(p[1] & 0x3F) << 6)
                        | ((uint32_t)(p[2] & 0x3F));
            out[n++] = hangul_to_opcode(cp);
            p += 3;
        } else if (*p < 0x80) {
            /* ASCII passthrough */
            out[n++] = *p++;
        } else {
            p++;
        }
    }
    return n;
}
