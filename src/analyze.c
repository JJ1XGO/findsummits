/*
 * analyze.c - 標高タイル1枚のプロミネンス解析
 *
 * SOTA日本支部 未登録サミット候補探索ツール
 * Copyright (C) 2026 JJ1XGO
 * GPL-3.0
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "analyze.h"
#include "elevation.h"
#include "unionfind.h"

/* 近傍を8方向（上下左右＋斜め4方向）に拡張 */
static const int dx[] = { 0,  0, -1,  1, -1,  1, -1,  1 };
static const int dy[] = {-1,  1,  0,  0, -1, -1,  1,  1 };

/* インデックスソート用コンテキスト */
typedef struct { const float *data; uint32_t W; } SortCtx;

static int cmp_elev_desc(const void *a, const void *b, void *ctx)
{
    const SortCtx *c  = ctx;
    uint32_t ia = *(const uint32_t *)a;
    uint32_t ib = *(const uint32_t *)b;
    float    ea = c->data[ia], eb = c->data[ib];
    if (ea > eb) return -1;
    if (ea < eb) return  1;
    uint32_t xa = ia % c->W, xb = ib % c->W;  /* x 小=西 優先（決定論化） */
    if (xa < xb) return -1;
    if (xa > xb) return  1;
    uint32_t ya = ia / c->W, yb = ib / c->W;  /* y 小=北 優先 */
    if (ya < yb) return -1;
    if (ya > yb) return  1;
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
    size_t   N = (size_t)W * H;

    /* ピクセルインデックス配列を標高降順にソート（Pixel構造体より1/3のメモリ） */
    uint32_t *indices = malloc(sizeof(uint32_t) * N);
    if (!indices) return NULL;
    for (size_t i = 0; i < N; i++) indices[i] = (uint32_t)i;

    SortCtx sctx = { tile->data, W };
    qsort_r(indices, N, sizeof(uint32_t), cmp_elev_desc, &sctx);

    UnionFind *uf = uf_create(N);
    if (!uf) { free(indices); return NULL; }

    int8_t *processed = calloc(N, sizeof(int8_t));
    if (!processed) { uf_destroy(uf); free(indices); return NULL; }

    /* 高い順に1ピクセルずつ処理 */
    for (size_t pi = 0; pi < N; pi++) {
        uint32_t idx  = indices[pi];
        int32_t  x    = (int32_t)(idx % W);
        int32_t  y    = (int32_t)(idx / W);
        float    elev = tile->data[idx];
        uint32_t i    = (uint32_t)((uint64_t)y * W + x);

        processed[i] = 1;

        uint32_t neighbor_roots[8];
        int      neighbor_cnt = 0;

        for (int d = 0; d < 8; d++) {
            int nx = x + dx[d];
            int ny = y + dy[d];
            if (nx < 0 || nx >= (int)W || ny < 0 || ny >= (int)H) continue;
            uint32_t ni = (uint32_t)((uint64_t)ny * W + nx);
            if (!processed[ni]) continue;

            uint32_t root = uf_find(uf, ni);

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
            uint32_t root = neighbor_roots[0];
            if (peakmap_get(&uf->pm, root) >= 0) {
                /* 有効なピークコンポーネントに合流 */
                uf->parent[i] = root;
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
            int      max_root_valid = 0;
            uint32_t max_root = 0;
            for (int k = 0; k < neighbor_cnt; k++) {
                uint32_t root = neighbor_roots[k];
                if (peakmap_get(&uf->pm, root) < 0) continue;  /* 海面コンポーネントは除外 */
                if (!max_root_valid) {
                    max_root       = root;
                    max_root_valid = 1;
                } else {
                    int pid_k   = peakmap_get(&uf->pm, root);
                    int pid_max = peakmap_get(&uf->pm, max_root);
                    if (uf->peaks[pid_k].elev > uf->peaks[pid_max].elev)
                        max_root = root;
                }
            }

            if (!max_root_valid) {
                /* 全隣接が海面コンポーネント：このピクセルも海面扱い */
                if (elev > 0.0f)
                    uf_new_peak(uf, i, x, y, elev);
            } else {
                uf->parent[i] = max_root;

                for (int k = 0; k < neighbor_cnt; k++) {
                    uint32_t root = neighbor_roots[k];
                    if (root == max_root) continue;
                    if (peakmap_get(&uf->pm, root) < 0) continue;  /* 海面コンポーネントはスキップ */
                    uf_union(uf, i, root, elev, x, y);
                }
            }
        }
    }

    /* 結果を収集 */
    AnalyzeResult *result = malloc(sizeof(AnalyzeResult));
    result->peaks    = malloc(sizeof(PeakResult) * uf->peak_cnt);
    result->peak_cnt = 0;

    int32_t img_w = (int32_t)W;
    int32_t img_h = (int32_t)H;

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

        int32_t margin = -1;
        if (!is_top) {
            int32_t cx = p->col_x, cy = p->col_y;
            int32_t d0 = cx;
            int32_t d1 = cy;
            int32_t d2 = (img_w - 1) - cx;
            int32_t d3 = (img_h - 1) - cy;
            margin = d0;
            if (d1 < margin) margin = d1;
            if (d2 < margin) margin = d2;
            if (d3 < margin) margin = d3;
        }

        PeakResult *pr  = &result->peaks[result->peak_cnt++];
        pr->peak_x      = p->x;
        pr->peak_y      = p->y;
        pr->peak_elev   = p->elev;
        pr->col_x       = p->col_x;
        pr->col_y       = p->col_y;
        pr->col_elev    = p->col_elev;
        pr->prominence  = prom;
        pr->is_tile_top = is_top;
        pr->col_margin_px = margin;
    }

    free(processed);
    free(indices);
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
