/**
 * Canvapress CVP1 — 통합 고정 명세 v1.1
 * ========================================
 * 512×512 고정. 양발 커널.
 * 모든 정보(바이트, 순서, k, 페이지, 리셋, 방문집합)가
 * RGBA 밝기값으로 녹아있다.
 *
 * 채널:
 *   R : u64 — R lane 밝기 (FULL에서 감산)
 *   G : u64 — G lane 밝기 (FULL에서 감산)
 *   B : u32 — 페이지리셋 + k누산
 *   A : u32 — 방문 횟수 (정수화 기반)
 *
 * 양발 커널:
 *   홀수 step → R lane, x = byte,       k = (step+1)/2
 *   짝수 step → G lane, x = 256 + byte, k = step/2
 *   y = step & 511,  pidx = (y << 9) + x
 *
 * 정수화:
 *   방문 사이클 집합 → 정렬 → 델타 → 정수 → 밝기값
 *   디코딩: 밝기값 → 정수 → 델타 → 집합 → 원본 복원
 *   패턴/주기 구분 없음. 통일된 정수화 규칙.
 */

#ifndef CVP1_H
#define CVP1_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ─── 캔버스 상수 ─── */
#define CVP1_H_SIZE       512
#define CVP1_W_SIZE       512
#define CVP1_PIXELS       (CVP1_H_SIZE * CVP1_W_SIZE)  /* 262144 */
#define CVP1_RG_LIMIT     (UINT64_MAX / 32)

/* ─── pidx ─── */
#define CVP1_PIDX(y, x)   (((uint32_t)(y) << 9) + (uint32_t)(x))

/* ─── B 채널 ─── */
#define CVP1_B_PAGE_RESETS(b)    (((b) >> 24) & 0xFF)
#define CVP1_B_K_ACCUM(b)       ((b) & 0xFFFF)
#define CVP1_B_MAKE(resets, klo) ((((uint32_t)(resets) & 0xFF) << 24) | ((uint32_t)(klo) & 0xFFFF))

/* ─── 캔버스 ─── */
typedef struct {
    uint64_t R[CVP1_PIXELS];
    uint64_t G[CVP1_PIXELS];
    uint32_t B[CVP1_PIXELS];
    uint32_t A[CVP1_PIXELS];    /* 방문 횟수 (인코딩 중) */

    uint8_t *input_buf;          /* 입력 버퍼 (방문 스케줄 구축용) */
    uint64_t input_cap;

    uint64_t total_steps;
    uint64_t total_cycles;
} CVP1Canvas;

/* ─── 헤더 ─── */
typedef struct {
    char     magic[4];           /* "CVP1" */
    uint64_t total_steps;
    uint32_t active_pixels;
    uint32_t reserved;           /* 예약 */
} CVP1Header;

/* ─── 통계 ─── */
typedef struct {
    uint64_t original_bytes;
    uint64_t image_bytes;
    double   compression_ratio;
    double   saving_pct;
    uint32_t active_pixels;
    uint32_t total_pixels;
    uint32_t single_visit;       /* 단일방문 픽셀 */
    uint32_t multi_visit;        /* 다중방문 픽셀 */
    uint32_t bh_loop_count;      /* 미사용 (호환) */
    uint32_t bh_raw_count;       /* 미사용 (호환) */
    double   sparsity;
    uint64_t total_cycles;
    uint32_t max_visit_count;
} CVP1Stats;

/* ═══ API ═══ */

void      cvp1_init(CVP1Canvas *c);
void      cvp1_free(CVP1Canvas *c);
void      cvp1_encode_chunk(CVP1Canvas *c, const uint8_t *data, size_t len);
size_t    cvp1_write_image(const CVP1Canvas *c, const char *path);
int       cvp1_read_and_decode(const char *path, uint8_t *out, size_t out_cap,
                               size_t *actual);
CVP1Stats cvp1_stats(const CVP1Canvas *c, const char *image_path);
void      cvp1_print_stats(const CVP1Stats *s);

#ifdef __cplusplus
}
#endif

#endif /* CVP1_H */
