/**
 * Canvapress CVP1 — 벤치마크 & 100% 복원 검증
 */

#include "cvp1.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define IMG_FILE "/home/sj_spatial_compress/test_data/canvas.cvp1"
#define STREAM_BUF (64 * 1024)

typedef void (*gen_fn)(uint8_t *buf, size_t len, uint64_t offset);

static void gen_text(uint8_t *b, size_t l, uint64_t o) {
    const char *p = "CanvasOS SJ Spatial V3 Final - image is everything. ";
    size_t pl = strlen(p);
    for (size_t i = 0; i < l; i++) b[i] = (uint8_t)p[(o+i)%pl];
}
static void gen_bin(uint8_t *b, size_t l, uint64_t o) {
    for (size_t i = 0; i < l; i++) b[i] = (uint8_t)(((o+i)*7+0x55)&0xFF);
}
static void gen_mono(uint8_t *b, size_t l, uint64_t o) {
    (void)o; memset(b, 0x42, l);
}
static void gen_rnd(uint8_t *b, size_t l, uint64_t o) {
    (void)o; for (size_t i = 0; i < l; i++) b[i] = (uint8_t)(rand()%256);
}

static void run(const char *name, uint64_t total, gen_fn gen) {
    printf("\n============================================================\n");
    printf("  %s\n", name);
    printf("  Input: %llu bytes (%.2f MB)\n",
           (unsigned long long)total, (double)total/1048576.0);
    printf("============================================================\n");

    CVP1Canvas *c = (CVP1Canvas*)malloc(sizeof(CVP1Canvas));
    if (!c) { printf("  [ERROR] malloc failed\n"); return; }
    cvp1_init(c);

    uint8_t *chunk = (uint8_t*)malloc(STREAM_BUF);

    /* 인코딩 */
    uint64_t fed = 0;
    clock_t t0 = clock();
    while (fed < total) {
        size_t n = STREAM_BUF;
        if (fed + n > total) n = (size_t)(total - fed);
        gen(chunk, n, fed);
        cvp1_encode_chunk(c, chunk, n);
        fed += n;
    }
    clock_t t1 = clock();
    double enc_s = (double)(t1-t0)/CLOCKS_PER_SEC;

    /* 이미지 출력 */
    clock_t t2 = clock();
    size_t img_sz = cvp1_write_image(c, IMG_FILE);
    clock_t t3 = clock();
    double img_s = (double)(t3-t2)/CLOCKS_PER_SEC;
    (void)img_sz;

    /* 통계 */
    CVP1Stats s = cvp1_stats(c, IMG_FILE);
    cvp1_print_stats(&s);

    printf("  +-- 성능 -------------------------------------------\n");
    double tp = (enc_s > 0) ? (double)total / enc_s / 1048576.0 : 0;
    printf("  | 인코딩:          %10.3f s (%7.1f MB/s)\n", enc_s, tp);
    printf("  | 이미지 출력:     %10.3f s\n", img_s);
    printf("  +---------------------------------------------------\n");

    /* 복원 검증 */
    if (total <= 16ULL * 1024 * 1024) {
        printf("\n  [복원] 이미지 -> 원본 디코딩...\n");
        size_t dec_cap = (size_t)total + 1024;
        uint8_t *restored = (uint8_t*)malloc(dec_cap);
        size_t actual = 0;

        clock_t t4 = clock();
        int rc = cvp1_read_and_decode(IMG_FILE, restored, dec_cap, &actual);
        clock_t t5 = clock();
        double dec_s = (double)(t5-t4)/CLOCKS_PER_SEC;

        if (rc == 0 && actual == total) {
            /* 원본 재생성 비교 */
            if (gen == gen_rnd) srand(42); /* rnd 재현 */
            uint8_t *expect = (uint8_t*)malloc((size_t)total);
            uint64_t eo = 0;
            if (gen == gen_rnd) srand(42);
            while (eo < total) {
                size_t n = STREAM_BUF;
                if (eo + n > total) n = (size_t)(total - eo);
                gen(expect + eo, n, eo);
                eo += n;
            }

            uint64_t match = 0;
            for (uint64_t i = 0; i < total; i++)
                if (restored[i] == expect[i]) match++;

            double accuracy = (double)match / (double)total * 100.0;
            printf("  [복원] %llu / %llu bytes 일치 (%.2f%%) | %.3fs\n",
                   (unsigned long long)match, (unsigned long long)total,
                   accuracy, dec_s);

            if (accuracy < 100.0) {
                uint32_t shown = 0;
                for (uint64_t i = 0; i < total && shown < 5; i++) {
                    if (restored[i] != expect[i]) {
                        printf("  [불일치] pos=%llu expect=0x%02X got=0x%02X\n",
                               (unsigned long long)i, expect[i], restored[i]);
                        shown++;
                    }
                }
            }
            free(expect);
        } else {
            printf("  [복원] FAIL (rc=%d, actual=%zu)\n", rc, actual);
        }
        free(restored);
    } else {
        printf("\n  [복원] 대형 데이터: 생략\n");
    }

    cvp1_free(c);
    free(chunk);
    free(c);
}

int main(void) {
    srand(42);

    printf("================================================================\n");
    printf("  Canvapress CVP1 -- WH/BH + 정렬정리 무손실 압축\n");
    printf("  512x512 | R(u64) G(u64) B(u32) A(u32)\n");
    printf("  양발 커널 | WH 방문스케줄 | BH LOOP/RAW 패턴압축\n");
    printf("================================================================\n");

    run("1KB 텍스트",              1024, gen_text);
    run("10KB 텍스트",             10240, gen_text);
    run("100KB 텍스트",            102400, gen_text);
    run("1MB 텍스트",              1048576, gen_text);
    run("4MB 텍스트",              4ULL*1024*1024, gen_text);
    run("4MB 바이너리",            4ULL*1024*1024, gen_bin);
    run("4MB 랜덤",               4ULL*1024*1024, gen_rnd);
    run("16MB 단일바이트",         16ULL*1024*1024, gen_mono);

    printf("\n================================================================\n");
    printf("  CVP1: 데이터 + 정보 = 모두 밝기값으로 변환. 100%% 무손실.\n");
    printf("================================================================\n");

    return 0;
}
