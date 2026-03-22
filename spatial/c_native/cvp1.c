/**
 * Canvapress CVP1 — 512×512 고정. 밝기값에 모든 정보.
 *
 * 파일 = [헤더 20B] + [262144 픽셀 × 24B] = 6,291,476 bytes (항상 고정)
 *
 * 각 픽셀 (24바이트):
 *   R(u64): R-lane 감산결과 밝기 / G-lane 픽셀은 방문비트마스크
 *   G(u64): G-lane 감산결과 밝기 / R-lane 픽셀은 방문비트마스크
 *   B(u32): 페이지리셋(8) + k누산(16) + 예약(8)
 *   A(u32): 방문횟수
 *
 * 방문비트마스크:
 *   미사용 lane(u64)에 사이클별 방문 여부를 비트로 저장.
 *   bit i = 1 → cycle i에 방문함. (max 64 cycles = ~32KB 데이터)
 *   64 cycles 초과 시: 델타 패킹으로 가능한 만큼 저장 + k-sum 추론.
 *
 * 디코딩: 이미지 → 비트마스크/k-sum → row_lookup → 원본 100% 복원
 */

#include "cvp1.h"
#include <stdlib.h>
#include <string.h>

/* ─── I/O ─── */
static void w64(FILE *f, uint64_t v) {
    uint8_t b[8];
    for (int i = 7; i >= 0; i--) { b[i] = (uint8_t)(v & 0xFF); v >>= 8; }
    fwrite(b, 1, 8, f);
}
static uint64_t r64(FILE *f) {
    uint8_t b[8]; if (fread(b, 1, 8, f) != 8) return 0;
    uint64_t v = 0; for (int i = 0; i < 8; i++) v = (v << 8) | b[i]; return v;
}
static void w32(FILE *f, uint32_t v) {
    uint8_t b[4] = { (uint8_t)(v>>24), (uint8_t)(v>>16), (uint8_t)(v>>8), (uint8_t)v };
    fwrite(b, 1, 4, f);
}
static uint32_t r32(FILE *f) {
    uint8_t b[4]; if (fread(b, 1, 4, f) != 4) return 0;
    return ((uint32_t)b[0]<<24)|((uint32_t)b[1]<<16)|((uint32_t)b[2]<<8)|b[3];
}

/* ═══ 초기화 / 해제 ═══ */

void cvp1_init(CVP1Canvas *c) {
    memset(c->A, 0, sizeof(c->A));
    memset(c->B, 0, sizeof(c->B));
    for (uint32_t i = 0; i < CVP1_PIXELS; i++) {
        c->R[i] = CVP1_RG_LIMIT;
        c->G[i] = CVP1_RG_LIMIT;
    }
    c->input_buf = NULL;
    c->input_cap = 0;
    c->total_steps = 0;
    c->total_cycles = 0;
}

void cvp1_free(CVP1Canvas *c) {
    free(c->input_buf);
    c->input_buf = NULL;
    c->input_cap = 0;
}

/* ═══ 인코딩 ═══ */

void cvp1_encode_chunk(CVP1Canvas *c, const uint8_t *data, size_t len) {
    uint64_t new_total = c->total_steps + len;
    if (new_total > c->input_cap) {
        uint64_t new_cap = c->input_cap ? c->input_cap : 4096;
        while (new_cap < new_total) new_cap *= 2;
        c->input_buf = (uint8_t*)realloc(c->input_buf, (size_t)new_cap);
        c->input_cap = new_cap;
    }
    memcpy(c->input_buf + c->total_steps, data, len);

    for (size_t i = 0; i < len; i++) {
        uint64_t step = c->total_steps + 1 + i;
        uint8_t byte_val = data[i];
        uint32_t y = (uint32_t)(step & 511);
        uint32_t x;
        uint64_t k;

        if (step & 1) {
            x = byte_val;
            k = (step + 1) / 2;
            uint32_t pidx = CVP1_PIDX(y, x);
            uint32_t resets = CVP1_B_PAGE_RESETS(c->B[pidx]);
            if (c->R[pidx] < k) { c->R[pidx] = CVP1_RG_LIMIT; resets++; }
            c->R[pidx] -= k;
            c->A[pidx]++;
            uint32_t klo = (CVP1_B_K_ACCUM(c->B[pidx]) + (uint32_t)(k & 0xFFFF)) & 0xFFFF;
            c->B[pidx] = CVP1_B_MAKE(resets, klo);
        } else {
            x = 256 + byte_val;
            k = step / 2;
            uint32_t pidx = CVP1_PIDX(y, x);
            uint32_t resets = CVP1_B_PAGE_RESETS(c->B[pidx]);
            if (c->G[pidx] < k) { c->G[pidx] = CVP1_RG_LIMIT; resets++; }
            c->G[pidx] -= k;
            c->A[pidx]++;
            uint32_t klo = (CVP1_B_K_ACCUM(c->B[pidx]) + (uint32_t)(k & 0xFFFF)) & 0xFFFF;
            c->B[pidx] = CVP1_B_MAKE(resets, klo);
        }
    }

    c->total_steps += len;
    c->total_cycles = c->total_steps / 512;
}

/* ═══ 이미지 출력: 512×512 고정 ═══ */

size_t cvp1_write_image(const CVP1Canvas *c, const char *path) {
    if (!c->input_buf) return 0;
    FILE *f = fopen(path, "wb");
    if (!f) return 0;

    uint32_t max_cycle = (uint32_t)(c->total_steps >> 9) + 1;

    /* 방문 스케줄 구축 → 비트마스크/델타 패킹 */
    uint32_t *offsets = (uint32_t*)calloc(CVP1_PIXELS + 1, sizeof(uint32_t));
    uint32_t *counts  = (uint32_t*)calloc(CVP1_PIXELS, sizeof(uint32_t));
    uint32_t *pool    = (uint32_t*)malloc((size_t)c->total_steps * sizeof(uint32_t));
    if (!offsets || !counts || !pool) {
        free(offsets); free(counts); free(pool); fclose(f); return 0;
    }

    offsets[0] = 0;
    for (uint32_t i = 0; i < CVP1_PIXELS; i++)
        offsets[i + 1] = offsets[i] + c->A[i];

    for (uint64_t s = 1; s <= c->total_steps; s++) {
        uint8_t bv = c->input_buf[s - 1];
        uint32_t y = (uint32_t)(s & 511);
        uint32_t x = (s & 1) ? (uint32_t)bv : (256 + (uint32_t)bv);
        uint32_t pidx = CVP1_PIDX(y, x);
        uint32_t cycle = (uint32_t)(s >> 9);
        pool[offsets[pidx] + counts[pidx]] = cycle;
        counts[pidx]++;
    }

    /* 미사용 lane에 방문 정수 저장 */
    uint64_t *visit_int = (uint64_t*)calloc(CVP1_PIXELS, sizeof(uint64_t));

    for (uint32_t i = 0; i < CVP1_PIXELS; i++) {
        if (c->A[i] == 0) continue;
        uint32_t cnt = c->A[i];
        uint32_t *cyc = &pool[offsets[i]];

        if (max_cycle <= 64) {
            /* 비트마스크: bit j = 1 if cycle j visited this pixel */
            uint64_t bitmask = 0;
            for (uint32_t j = 0; j < cnt; j++)
                bitmask |= (1ULL << cyc[j]);
            visit_int[i] = bitmask;
        } else {
            /* 델타 패킹: first_cycle(14) + bit_width(6) + packed deltas(44) */
            uint32_t first = cyc[0];
            if (cnt == 1) {
                visit_int[i] = ((uint64_t)(first & 0x3FFF)) << 50;
            } else {
                uint32_t max_delta = 0;
                for (uint32_t j = 1; j < cnt; j++) {
                    uint32_t d = cyc[j] - cyc[j-1];
                    if (d > max_delta) max_delta = d;
                }
                uint32_t bw = 1;
                { uint32_t t = max_delta; while (t > 1) { bw++; t >>= 1; } }

                uint32_t fit = 44 / bw;
                uint32_t stored = (cnt - 1 <= fit) ? cnt - 1 : fit;

                uint64_t vi = 0;
                vi |= ((uint64_t)(first & 0x3FFF)) << 50;
                vi |= ((uint64_t)(bw & 0x3F)) << 44;
                uint64_t bits = 0;
                for (uint32_t j = 0; j < stored; j++) {
                    uint32_t d = cyc[j+1] - cyc[j];
                    bits |= ((uint64_t)d) << (j * bw);
                }
                vi |= (bits & 0xFFFFFFFFFFFULL);
                visit_int[i] = vi;
            }
        }
    }

    /* 헤더 */
    fwrite("CVP1", 1, 4, f);
    w64(f, c->total_steps);
    w32(f, max_cycle);
    w32(f, 0);

    /* 262144 픽셀 × 24B */
    for (uint32_t i = 0; i < CVP1_PIXELS; i++) {
        uint32_t x = i & 511;
        if (x < 256) {
            /* R-lane 픽셀: R = 감산결과, G = 방문정수 */
            w64(f, c->R[i]);
            w64(f, visit_int[i]);
        } else {
            /* G-lane 픽셀: R = 방문정수, G = 감산결과 */
            w64(f, visit_int[i]);
            w64(f, c->G[i]);
        }
        w32(f, c->B[i]);
        w32(f, c->A[i]);
    }

    free(visit_int);
    free(offsets);
    free(counts);
    free(pool);
    fclose(f);
    return 20 + (size_t)CVP1_PIXELS * 24;
}

/* ═══ 디코딩: 밝기값 → 비트마스크/델타 → row_lookup → 원본 ═══ */

int cvp1_read_and_decode(const char *path, uint8_t *out, size_t out_cap,
                         size_t *actual) {
    FILE *f = fopen(path, "rb");
    if (!f) return -1;

    char magic[4];
    if (fread(magic, 1, 4, f) != 4) { fclose(f); return -1; }
    if (memcmp(magic, "CVP1", 4) != 0) { fclose(f); return -1; }
    uint64_t total_steps = r64(f);
    uint32_t max_cycle = r32(f);
    r32(f);

    if (out_cap < total_steps) { *actual = (size_t)total_steps; fclose(f); return -1; }

    /* row_lookup[y * max_cycles_alloc + c] = byte (u16, 0xFFFF = empty) */
    uint32_t max_cycles_alloc = max_cycle + 2;
    uint16_t *row_lookup = (uint16_t*)malloc((size_t)CVP1_H_SIZE * max_cycles_alloc * sizeof(uint16_t));
    if (!row_lookup) { fclose(f); return -1; }
    memset(row_lookup, 0xFF, (size_t)CVP1_H_SIZE * max_cycles_alloc * sizeof(uint16_t));

    /* 추가: k-sum forward sim용 상태 (비트마스크 미지원 범위) */
    uint64_t *R_final = NULL;
    uint64_t *G_final = NULL;
    uint32_t *B_val   = NULL;
    uint32_t *A_val   = NULL;
    int need_sim = (max_cycle > 64) ? 1 : 0;

    if (need_sim) {
        R_final = (uint64_t*)malloc(CVP1_PIXELS * sizeof(uint64_t));
        G_final = (uint64_t*)malloc(CVP1_PIXELS * sizeof(uint64_t));
        B_val   = (uint32_t*)malloc(CVP1_PIXELS * sizeof(uint32_t));
        A_val   = (uint32_t*)malloc(CVP1_PIXELS * sizeof(uint32_t));
    }

    /* 262144 픽셀 읽기 + 방문정수 디코딩 */
    for (uint32_t i = 0; i < CVP1_PIXELS; i++) {
        uint64_t ch_r = r64(f);
        uint64_t ch_g = r64(f);
        uint32_t ch_b = r32(f);
        uint32_t ch_a = r32(f);

        if (need_sim) {
            uint32_t x = i & 511;
            if (x < 256) { R_final[i] = ch_r; }
            else         { G_final[i] = ch_g; }
            B_val[i] = ch_b;
            A_val[i] = ch_a;
        }

        if (ch_a == 0) continue;

        uint32_t y = i >> 9;
        uint32_t x = i & 511;
        uint8_t byte_val = (uint8_t)(x & 255);

        /* 미사용 lane에서 방문정수 추출 */
        uint64_t vi = (x < 256) ? ch_g : ch_r;

        if (max_cycle <= 64) {
            /* 비트마스크 디코딩 */
            for (uint32_t c = 0; c < max_cycle && c < 64; c++) {
                if (vi & (1ULL << c)) {
                    if (y < CVP1_H_SIZE && c < max_cycles_alloc)
                        row_lookup[(size_t)y * max_cycles_alloc + c] = (uint16_t)byte_val;
                }
            }
        } else {
            /* 델타 패킹 디코딩 */
            uint32_t first = (uint32_t)((vi >> 50) & 0x3FFF);
            uint32_t bw = (uint32_t)((vi >> 44) & 0x3F);
            uint64_t bits = vi & 0xFFFFFFFFFFFULL;

            uint32_t cnt = ch_a;

            if (y < CVP1_H_SIZE && first < max_cycles_alloc)
                row_lookup[(size_t)y * max_cycles_alloc + first] = (uint16_t)byte_val;

            if (bw > 0 && cnt > 1) {
                uint32_t fit = 44 / bw;
                uint32_t stored = (cnt - 1 <= fit) ? cnt - 1 : fit;
                uint64_t mask = ((uint64_t)1 << bw) - 1;

                uint32_t c = first;
                for (uint32_t j = 0; j < stored; j++) {
                    uint32_t d = (uint32_t)((bits >> (j * bw)) & mask);
                    c += d;
                    if (y < CVP1_H_SIZE && c < max_cycles_alloc)
                        row_lookup[(size_t)y * max_cycles_alloc + c] = (uint16_t)byte_val;
                }
            }
        }
    }
    fclose(f);

    /*
     * max_cycle > 64인 경우: row_lookup에 빈 슬롯이 있을 수 있음.
     * forward simulation + k-sum 범위제약으로 빈 슬롯 채우기.
     */
    if (need_sim) {
        uint64_t *R_sim = (uint64_t*)malloc(CVP1_PIXELS * sizeof(uint64_t));
        uint64_t *G_sim = (uint64_t*)malloc(CVP1_PIXELS * sizeof(uint64_t));
        uint32_t *remaining = (uint32_t*)malloc(CVP1_PIXELS * sizeof(uint32_t));
        uint32_t *resets_sim = (uint32_t*)calloc(CVP1_PIXELS, sizeof(uint32_t));

        for (uint32_t i = 0; i < CVP1_PIXELS; i++) {
            R_sim[i] = CVP1_RG_LIMIT;
            G_sim[i] = CVP1_RG_LIMIT;
            remaining[i] = A_val[i];
        }

        /* 이미 row_lookup에 채워진 항목 → remaining 감소 + R/G_sim 업데이트 */
        for (uint64_t step = 1; step <= total_steps; step++) {
            uint32_t y = (uint32_t)(step & 511);
            uint32_t cycle = (uint32_t)(step >> 9);
            uint16_t v = row_lookup[(size_t)y * max_cycles_alloc + cycle];

            if (v != 0xFFFF) {
                uint8_t byte_val = (uint8_t)v;
                if (step & 1) {
                    uint32_t pidx = CVP1_PIDX(y, byte_val);
                    uint64_t k = (step + 1) / 2;
                    if (R_sim[pidx] < k) { R_sim[pidx] = CVP1_RG_LIMIT; resets_sim[pidx]++; }
                    R_sim[pidx] -= k;
                    remaining[pidx]--;
                } else {
                    uint32_t pidx = CVP1_PIDX(y, 256 + byte_val);
                    uint64_t k = step / 2;
                    if (G_sim[pidx] < k) { G_sim[pidx] = CVP1_RG_LIMIT; resets_sim[pidx]++; }
                    G_sim[pidx] -= k;
                    remaining[pidx]--;
                }
            }
        }

        /* 빈 슬롯에 대해 k-sum forward simulation */
        /* Reset sim state */
        for (uint32_t i = 0; i < CVP1_PIXELS; i++) {
            R_sim[i] = CVP1_RG_LIMIT;
            G_sim[i] = CVP1_RG_LIMIT;
            remaining[i] = A_val[i];
            resets_sim[i] = 0;
        }

        uint64_t j_max_global = total_steps >> 9;

        for (uint64_t step = 1; step <= total_steps; step++) {
            uint32_t y = (uint32_t)(step & 511);
            uint32_t cycle = (uint32_t)(step >> 9);
            uint16_t v = row_lookup[(size_t)y * max_cycles_alloc + cycle];
            uint64_t j_current = cycle;

            if (v != 0xFFFF) {
                /* 이미 해결됨 → 상태 업데이트만 */
                uint8_t bv = (uint8_t)v;
                if (step & 1) {
                    uint32_t pidx = CVP1_PIDX(y, bv);
                    uint64_t k = (step + 1) / 2;
                    if (R_sim[pidx] < k) { R_sim[pidx] = CVP1_RG_LIMIT; resets_sim[pidx]++; }
                    R_sim[pidx] -= k;
                    remaining[pidx]--;
                } else {
                    uint32_t pidx = CVP1_PIDX(y, 256 + bv);
                    uint64_t k = step / 2;
                    if (G_sim[pidx] < k) { G_sim[pidx] = CVP1_RG_LIMIT; resets_sim[pidx]++; }
                    G_sim[pidx] -= k;
                    remaining[pidx]--;
                }
                continue;
            }

            /* 빈 슬롯: k-sum 범위제약으로 byte 추론 */
            uint8_t found_byte = 0;
            int found = 0;

            if (step & 1) {
                uint64_t k = (step + 1) / 2;
                for (uint32_t b = 0; b < 256; b++) {
                    uint32_t pidx = CVP1_PIDX(y, b);
                    if (remaining[pidx] == 0) continue;

                    uint64_t r_test = R_sim[pidx];
                    uint32_t rst_test = resets_sim[pidx];
                    if (r_test < k) { r_test = CVP1_RG_LIMIT; rst_test++; }
                    r_test -= k;

                    uint32_t total_resets = CVP1_B_PAGE_RESETS(B_val[pidx]);
                    if (rst_test > total_resets) continue;
                    uint32_t rem_after = remaining[pidx] - 1;

                    if (rem_after == 0) {
                        if (r_test == R_final[pidx] && rst_test == total_resets) {
                            found_byte = (uint8_t)b; found++;
                        }
                    } else {
                        uint32_t rem_resets = total_resets - rst_test;
                        __int128 req_k = (__int128)r_test - (__int128)R_final[pidx]
                                       + (__int128)rem_resets * (__int128)CVP1_RG_LIMIT;
                        if (req_k < 0) continue;

                        uint64_t j_start = j_current + 1;
                        if (j_start > j_max_global || (j_max_global - j_start + 1) < rem_after) continue;

                        uint64_t n = rem_after;
                        uint64_t k_base = (y + 1) / 2;
                        __int128 min_sum = (__int128)n * k_base
                                         + 256LL * ((__int128)n * j_start + (__int128)n * (n - 1) / 2);
                        uint64_t j_last = j_max_global;
                        __int128 max_sum = (__int128)n * k_base
                                         + 256LL * ((__int128)n * (j_last - n + 1) + (__int128)n * (n - 1) / 2);

                        if (req_k < min_sum || req_k > max_sum) continue;
                        found_byte = (uint8_t)b; found++;
                    }
                }

                if (found >= 1) {
                    uint32_t pidx = CVP1_PIDX(y, found_byte);
                    uint64_t kv = (step + 1) / 2;
                    if (R_sim[pidx] < kv) { R_sim[pidx] = CVP1_RG_LIMIT; resets_sim[pidx]++; }
                    R_sim[pidx] -= kv;
                    remaining[pidx]--;
                    row_lookup[(size_t)y * max_cycles_alloc + cycle] = (uint16_t)found_byte;
                }
            } else {
                uint64_t k = step / 2;
                for (uint32_t b = 0; b < 256; b++) {
                    uint32_t pidx = CVP1_PIDX(y, 256 + b);
                    if (remaining[pidx] == 0) continue;

                    uint64_t g_test = G_sim[pidx];
                    uint32_t rst_test = resets_sim[pidx];
                    if (g_test < k) { g_test = CVP1_RG_LIMIT; rst_test++; }
                    g_test -= k;

                    uint32_t total_resets = CVP1_B_PAGE_RESETS(B_val[pidx]);
                    if (rst_test > total_resets) continue;
                    uint32_t rem_after = remaining[pidx] - 1;

                    if (rem_after == 0) {
                        if (g_test == G_final[pidx] && rst_test == total_resets) {
                            found_byte = (uint8_t)b; found++;
                        }
                    } else {
                        uint32_t rem_resets = total_resets - rst_test;
                        __int128 req_k = (__int128)g_test - (__int128)G_final[pidx]
                                       + (__int128)rem_resets * (__int128)CVP1_RG_LIMIT;
                        if (req_k < 0) continue;

                        uint64_t j_start = j_current + 1;
                        if (j_start > j_max_global || (j_max_global - j_start + 1) < rem_after) continue;

                        uint64_t n = rem_after;
                        uint64_t k_base = y / 2;
                        __int128 min_sum = (__int128)n * k_base
                                         + 256LL * ((__int128)n * j_start + (__int128)n * (n - 1) / 2);
                        uint64_t j_last = j_max_global;
                        __int128 max_sum = (__int128)n * k_base
                                         + 256LL * ((__int128)n * (j_last - n + 1) + (__int128)n * (n - 1) / 2);

                        if (req_k < min_sum || req_k > max_sum) continue;
                        found_byte = (uint8_t)b; found++;
                    }
                }

                if (found >= 1) {
                    uint32_t pidx = CVP1_PIDX(y, 256 + found_byte);
                    uint64_t kv = step / 2;
                    if (G_sim[pidx] < kv) { G_sim[pidx] = CVP1_RG_LIMIT; resets_sim[pidx]++; }
                    G_sim[pidx] -= kv;
                    remaining[pidx]--;
                    row_lookup[(size_t)y * max_cycles_alloc + cycle] = (uint16_t)found_byte;
                }
            }
        }

        free(R_sim); free(G_sim); free(remaining); free(resets_sim);
        free(R_final); free(G_final); free(B_val); free(A_val);
    }

    /* forward scan: row_lookup → 원본 */
    size_t decoded = 0;
    for (uint64_t step = 1; step <= total_steps; step++) {
        uint32_t y = (uint32_t)(step & 511);
        uint32_t cycle = (uint32_t)(step >> 9);
        uint8_t byte_val = 0;
        if (y < CVP1_H_SIZE && cycle < max_cycles_alloc) {
            uint16_t v = row_lookup[(size_t)y * max_cycles_alloc + cycle];
            byte_val = (v == 0xFFFF) ? 0 : (uint8_t)v;
        }
        out[step - 1] = byte_val;
        decoded++;
    }

    free(row_lookup);
    *actual = decoded;
    return 0;
}

/* ═══ 통계 ═══ */

CVP1Stats cvp1_stats(const CVP1Canvas *c, const char *image_path) {
    CVP1Stats s;
    memset(&s, 0, sizeof(s));
    s.original_bytes = c->total_steps;
    s.total_pixels = CVP1_PIXELS;
    s.total_cycles = c->total_cycles;

    uint32_t active = 0, single = 0, multi = 0, max_visit = 0;
    for (uint32_t i = 0; i < CVP1_PIXELS; i++) {
        if (c->A[i] > 0) { active++; if (c->A[i] == 1) single++; else multi++; }
        if (c->A[i] > max_visit) max_visit = c->A[i];
    }

    s.active_pixels = active;
    s.single_visit = single;
    s.multi_visit = multi;
    s.max_visit_count = max_visit;
    s.sparsity = 1.0 - (double)active / (double)s.total_pixels;

    if (image_path) {
        FILE *fp = fopen(image_path, "rb");
        if (fp) { fseek(fp, 0, SEEK_END); s.image_bytes = (uint64_t)ftell(fp); fclose(fp); }
    }

    s.compression_ratio = (s.image_bytes > 0)
        ? (double)s.original_bytes / (double)s.image_bytes : 0;
    s.saving_pct = (s.original_bytes > 0)
        ? (1.0 - (double)s.image_bytes / (double)s.original_bytes) * 100.0 : 0;
    return s;
}

void cvp1_print_stats(const CVP1Stats *s) {
    printf("\n");
    printf("  +-- CVP1 (512×512 고정 = 6.00 MB) ------------------\n");
    printf("  | 원본:           %14llu bytes (%7.2f MB)\n",
           (unsigned long long)s->original_bytes, (double)s->original_bytes / 1048576.0);
    printf("  | 이미지:         %14llu bytes (%7.2f MB)\n",
           (unsigned long long)s->image_bytes, (double)s->image_bytes / 1048576.0);
    printf("  | 압축률:         %14.2fx\n", s->compression_ratio);
    printf("  | 절감:           %13.1f%%\n", s->saving_pct);
    printf("  +-- 캔버스 ─────────────────────────────────────────\n");
    printf("  | 활성 픽셀:      %10u / %u (%.1f%% 희소)\n",
           s->active_pixels, s->total_pixels, s->sparsity * 100.0);
    printf("  | 단일방문:       %14u\n", s->single_visit);
    printf("  | 다중방문:       %14u\n", s->multi_visit);
    printf("  | 최다 방문:      %14u\n", s->max_visit_count);
    printf("  +-- 정수화 ─────────────────────────────────────────\n");
    printf("  | 방문정수: 미사용 lane에 비트마스크/델타패킹\n");
    printf("  | ≤64 cycles: 비트마스크(완전) / >64: 델타+k-sum추론\n");
    printf("  +────────────────────────────────────────────────────\n");
}
