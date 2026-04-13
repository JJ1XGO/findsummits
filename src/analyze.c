/*
 * analyze.c - 標高タイル1枚のプロミネンス解析
 *
 * SOTA日本支部 未登録サミット候補探索ツール
 * Copyright (C) 2026 JJ1XGO
 * GPL-3.0
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "analyze.h"
#include "elevation.h"
#include "unionfind.h"

/* 4近傍 */
static const int dx[] = {0, 0, -1, 1};
static const int dy[] = {-1, 1, 0, 0};

/* ソート用ピクセル */
typedef struct {
    int32_t x, y;
    float   elev;
} Pixel;

static int cmp_elev_desc(const void *a, const void *b)
{
    float fa = ((Pixel*)a)->elev;
    float fb = ((Pixel*)b)->elev;
    if (fa > fb) return -1;
    if (fa < fb) return  1;
    return 0;
}

/*
 * 標高タイル1枚を解析してプロミネンスを計算する
 *
 * 戻り値: AnalyzeResult / NULLはエラー
 */
AnalyzeResult *analyze_tile(const char *png_path, float min_prominence)
{
    /* 1. PNGを読み込む */
    ElevTile *tile = elev_load_png(png_path);
    if (!tile) return NULL;

    uint32_t W = tile->width;
    uint32_t H = tile->height;
    uint32_t N = W * H;

    /* 2. 全ピクセルをリスト化してソート */
    Pixel *pixels = malloc(sizeof(Pixel) * N);
    if (!pixels) { elev_destroy(tile); return NULL; }

    for (uint32_t y = 0; y < H; y++)
        for (uint32_t x = 0; x < W; x++) {
            uint32_t i    = y * W + x;
            pixels[i].x   = x;
            pixels[i].y   = y;
            pixels[i].elev = elev_get(tile, x, y);
        }

    qsort(pixels, N, sizeof(Pixel), cmp_elev_desc);

    /* 3. Union-Find初期化 */
    UnionFind *uf = uf_create(N);
    if (!uf) { free(pixels); elev_destroy(tile); return NULL; }

    int8_t *processed = calloc(N, sizeof(int8_t));
    if (!processed) {
        uf_destroy(uf); free(pixels); elev_destroy(tile);
        return NULL;
    }

    /* 4. 高い順に1ピクセルずつ処理 */
    for (uint32_t pi = 0; pi < N; pi++) {
        int32_t x    = pixels[pi].x;
        int32_t y    = pixels[pi].y;
        float   elev = pixels[pi].elev;
        int32_t i    = (int32_t)(y * W + x);

        /* 海面・無効値はスキップ */
        if (elev <= 0.0f) continue;

        processed[i] = 1;

        /* 4近傍の処理済みグループを収集 */
        int32_t neighbor_roots[4];
        int     neighbor_cnt = 0;

        for (int d = 0; d < 4; d++) {
            int nx = x + dx[d];
            int ny = y + dy[d];
            if (nx < 0 || nx >= (int)W || ny < 0 || ny >= (int)H)
                continue;
            int32_t ni = ny * W + nx;
            if (!processed[ni]) continue;

            int32_t root = uf_find(uf, ni);
            int already = 0;
            for (int k = 0; k < neighbor_cnt; k++)
                if (neighbor_roots[k] == root) { already = 1; break; }
            if (!already)
                neighbor_roots[neighbor_cnt++] = root;
        }

        if (neighbor_cnt == 0) {
            /* 新ピーク */
            uf_new_peak(uf, i, x, y, elev);

        } else if (neighbor_cnt == 1) {
            /* 同グループに合流 */
            uf->peak_id[i] = uf->peak_id[neighbor_roots[0]];
            uf->parent[i]  = neighbor_roots[0];

        } else {
            /* コル発見 */
            /* 最高峰グループを探す */
            int32_t max_root = neighbor_roots[0];
            for (int k = 1; k < neighbor_cnt; k++) {
                int pid_k   = uf->peak_id[neighbor_roots[k]];
                int pid_max = uf->peak_id[max_root];
                if (pid_k >= 0 && pid_max >= 0 &&
                    uf->peaks[pid_k].elev > uf->peaks[pid_max].elev)
                    max_root = neighbor_roots[k];
            }

            uf->peak_id[i] = uf->peak_id[max_root];
            uf->parent[i]  = max_root;

            /* 最高峰以外をloserとして合体 */
            for (int k = 0; k < neighbor_cnt; k++) {
                int32_t root = neighbor_roots[k];
                if (root == max_root) continue;
                uf_union(uf, i, root, elev, x, y);
            }
        }
    }

    /* 5. 結果を収集 */
    AnalyzeResult *result = malloc(sizeof(AnalyzeResult));
    result->peaks     = malloc(sizeof(PeakResult) * uf->peak_cnt);
    result->peak_cnt  = 0;

    for (int pid = 0; pid < uf->peak_cnt; pid++) {
        Peak *p = &uf->peaks[pid];
        if (!p->valid) continue;

        float prom;
        if (p->col_elev < -9998.0f) {
            /* コル未発見=このタイルの最高峰 */
            prom = p->elev;  /* タイル内最高峰はプロミネンス=標高 */
        } else {
            prom = p->elev - p->col_elev;
        }

        /* min_prominence未満はスキップ */
        if (prom < min_prominence) continue;

        PeakResult *pr    = &result->peaks[result->peak_cnt++];
        pr->peak_x        = p->x;
        pr->peak_y        = p->y;
        pr->peak_elev     = p->elev;
        pr->col_x         = p->col_x;
        pr->col_y         = p->col_y;
        pr->col_elev      = p->col_elev;
        pr->prominence    = prom;
        pr->is_tile_top   = (p->col_elev < -9998.0f);
    }

    free(processed);
    free(pixels);
    uf_destroy(uf);
    elev_destroy(tile);

    return result;
}

void analyze_result_destroy(AnalyzeResult *result)
{
    if (!result) return;
    free(result->peaks);
    free(result);
}
