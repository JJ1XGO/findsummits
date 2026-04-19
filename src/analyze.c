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

/* 近傍を8方向（上下左右＋斜め4方向）に拡張 */
static const int dx[] = { 0,  0, -1,  1, -1,  1, -1,  1 };
static const int dy[] = {-1,  1,  0,  0, -1, -1,  1,  1 };

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
 * 標高タイルを解析してプロミネンスを計算する
 *
 * tile:          解析対象タイル(オーバーラップありなら257×257)
 * main_w/main_h: メインタイルの有効範囲(通常256×256)
 * min_prominence: 最小プロミネンス(m)
 */
AnalyzeResult *analyze_tile_data(const ElevTile *tile,
                                  uint32_t main_w, uint32_t main_h,
                                  float min_prominence)
{
    uint32_t W = tile->width;
    uint32_t H = tile->height;
    uint32_t N = W * H;

    /* 全ピクセルをリスト化してソート */
    Pixel *pixels = malloc(sizeof(Pixel) * N);
    if (!pixels) return NULL;

    for (uint32_t y = 0; y < H; y++)
        for (uint32_t x = 0; x < W; x++) {
            uint32_t i     = y * W + x;
            pixels[i].x    = x;
            pixels[i].y    = y;
            pixels[i].elev = elev_get(tile, x, y);
        }

    qsort(pixels, N, sizeof(Pixel), cmp_elev_desc);

    UnionFind *uf = uf_create(N);
    if (!uf) { free(pixels); return NULL; }

    int8_t *processed = calloc(N, sizeof(int8_t));
    if (!processed) { uf_destroy(uf); free(pixels); return NULL; }

    /* 高い順に1ピクセルずつ処理 */
    for (uint32_t pi = 0; pi < N; pi++) {
        int32_t x    = pixels[pi].x;
        int32_t y    = pixels[pi].y;
        float   elev = pixels[pi].elev;
        int32_t i    = y * W + x;

        processed[i] = 1;

        int32_t neighbor_roots[8];
        int     neighbor_cnt = 0;

        for (int d = 0; d < 8; d++) {
            int nx = x + dx[d];
            int ny = y + dy[d];
            if (nx < 0 || nx >= (int)W || ny < 0 || ny >= (int)H) continue;
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
            /* 海面・谷底ピクセルはピークを作らない */
            if (elev > 0.0f)
                uf_new_peak(uf, i, x, y, elev);

        } else if (neighbor_cnt == 1) {
            int32_t root = neighbor_roots[0];
            if (uf->peak_id[root] >= 0) {
                /* 有効なピークコンポーネントに合流 */
                uf->peak_id[i] = uf->peak_id[root];
                uf->parent[i]  = root;
            }
            /* peak_id < 0（海面コンポーネント）への合流は何もしない：
             * このピクセルは独立ピーク候補のまま残す。
             * ただし elev <= 0 なら海面なのでピークも作らない */
            else if (elev > 0.0f) {
                uf_new_peak(uf, i, x, y, elev);
            }

        } else {
            /* neighbor_cnt >= 2：コル発見。
             * peak_id >= 0 のrootだけを対象にwinner(最高峰)を選ぶ */
            int32_t max_root = -1;
            for (int k = 0; k < neighbor_cnt; k++) {
                int32_t root = neighbor_roots[k];
                if (uf->peak_id[root] < 0) continue;  /* 海面コンポーネントは除外 */
                if (max_root < 0) {
                    max_root = root;
                } else {
                    int pid_k   = uf->peak_id[root];
                    int pid_max = uf->peak_id[max_root];
                    if (uf->peaks[pid_k].elev > uf->peaks[pid_max].elev)
                        max_root = root;
                }
            }

            if (max_root < 0) {
                /* 全隣接が海面コンポーネント：このピクセルも海面扱い */
                if (elev > 0.0f)
                    uf_new_peak(uf, i, x, y, elev);
            } else {
                uf->peak_id[i] = uf->peak_id[max_root];
                uf->parent[i]  = max_root;

                for (int k = 0; k < neighbor_cnt; k++) {
                    int32_t root = neighbor_roots[k];
                    if (root == max_root) continue;
                    if (uf->peak_id[root] < 0) continue;  /* 海面コンポーネントはスキップ */
                    uf_union(uf, i, root, elev, x, y);
                }
            }
        }
    }

    /* 結果を収集 */
    AnalyzeResult *result = malloc(sizeof(AnalyzeResult));
    result->peaks    = malloc(sizeof(PeakResult) * uf->peak_cnt);
    result->peak_cnt = 0;

    for (int pid = 0; pid < uf->peak_cnt; pid++) {
        Peak *p = &uf->peaks[pid];
        if (!p->valid) continue;

        /* ピーク座標がメインタイル外ならスキップ */
        if (p->x >= (int32_t)main_w || p->y >= (int32_t)main_h)
            continue;

        float prom;
        int   is_top = (p->col_elev < -9998.0f);

        if (is_top) {
            prom = p->elev;
        } else {
            prom = p->elev - p->col_elev;
        }

        if (prom < min_prominence) continue;

        PeakResult *pr  = &result->peaks[result->peak_cnt++];
        pr->peak_x      = p->x;
        pr->peak_y      = p->y;
        pr->peak_elev   = p->elev;
        pr->col_x       = p->col_x;
        pr->col_y       = p->col_y;
        pr->col_elev    = p->col_elev;
        pr->prominence  = prom;
        pr->is_tile_top = is_top;
    }

    free(processed);
    free(pixels);
    uf_destroy(uf);

    return result;
}

/*
 * PNGファイルから直接解析する(オーバーラップなし・テスト用)
 */
AnalyzeResult *analyze_tile(const char *png_path, float min_prominence)
{
    ElevTile *tile = elev_load_png(png_path);
    if (!tile) return NULL;

    AnalyzeResult *result = analyze_tile_data(tile,
                                               tile->width, tile->height,
                                               min_prominence);
    elev_destroy(tile);
    return result;
}

/*
 * オーバーラップありで解析する
 */
AnalyzeResult *analyze_tile_overlap(const char *tile_dir,
                                     TileCoord tc,
                                     float min_prominence)
{
    ElevTile *tile = elev_load_with_overlap(tile_dir, tc);
    if (!tile) return NULL;

    /* メインタイルは256×256、オーバーラップ分は除外 */
    AnalyzeResult *result = analyze_tile_data(tile,
                                               TILE_PIX, TILE_PIX,
                                               min_prominence);
    elev_destroy(tile);
    return result;
}

void analyze_result_destroy(AnalyzeResult *result)
{
    if (!result) return;
    free(result->peaks);
    free(result);
}
