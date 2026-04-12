/*
 * test_unionfind.c - Union-Findの動作確認
 *
 * SOTA日本支部 未登録サミット候補探索ツール
 * Copyright (C) 2026 JJ1XGO
 * GPL-3.0
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "unionfind.h"

#define WIDTH  5
#define HEIGHT 5

static float test_elev[HEIGHT][WIDTH] = {
    {100, 50, 80, 50,  90},
    { 50, 40, 50, 40,  50},
    { 60, 50, 70, 50,  60},
    { 50, 40, 50, 40,  50},
    { 80, 50, 90, 50, 100},
};

static inline int32_t idx(int x, int y) { return y * WIDTH + x; }

typedef struct { int32_t x, y; float elev; } Pixel;

static int cmp_elev_desc(const void *a, const void *b)
{
    float fa = ((Pixel*)a)->elev;
    float fb = ((Pixel*)b)->elev;
    if (fa > fb) return -1;
    if (fa < fb) return  1;
    return 0;
}

int main(void)
{
    printf("=== Union-Find テスト ===\n\n");

    Pixel pixels[WIDTH * HEIGHT];
    for (int y = 0; y < HEIGHT; y++)
        for (int x = 0; x < WIDTH; x++) {
            pixels[idx(x,y)].x    = x;
            pixels[idx(x,y)].y    = y;
            pixels[idx(x,y)].elev = test_elev[y][x];
        }

    qsort(pixels, WIDTH * HEIGHT, sizeof(Pixel), cmp_elev_desc);

    UnionFind *uf = uf_create(WIDTH * HEIGHT);
    if (!uf) { fprintf(stderr, "メモリ確保失敗\n"); return 1; }

    int8_t processed[WIDTH * HEIGHT];
    memset(processed, 0, sizeof(processed));

    int dx[] = {0, 0, -1, 1};
    int dy[] = {-1, 1, 0, 0};

    printf("処理ログ:\n");

    for (int pi = 0; pi < WIDTH * HEIGHT; pi++) {
        int32_t x    = pixels[pi].x;
        int32_t y    = pixels[pi].y;
        float   elev = pixels[pi].elev;
        int32_t i    = idx(x, y);

        processed[i] = 1;

        /* 4近傍の処理済みグループを収集 */
        int32_t neighbor_roots[4];
        int     neighbor_cnt = 0;

        for (int d = 0; d < 4; d++) {
            int nx = x + dx[d];
            int ny = y + dy[d];
            if (nx < 0 || nx >= WIDTH || ny < 0 || ny >= HEIGHT) continue;
            int32_t ni = idx(nx, ny);
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
            printf("  (%d,%d) %.1fm: 新ピーク登録\n", x, y, elev);

        } else if (neighbor_cnt == 1) {
            /* 同グループに合流 */
            /* このピクセル自体はピークではないのでpeak_idを引き継ぐ */
            uf->peak_id[i] = uf->peak_id[neighbor_roots[0]];
            uf->parent[i]  = neighbor_roots[0];
            printf("  (%d,%d) %.1fm: グループに合流\n", x, y, elev);

        } else {
            /* コル発見 */
            printf("  (%d,%d) %.1fm: コル発見!\n", x, y, elev);

            /* 最高峰を探す */
            int32_t max_root = neighbor_roots[0];
            for (int k = 1; k < neighbor_cnt; k++) {
                int pid_k   = uf->peak_id[neighbor_roots[k]];
                int pid_max = uf->peak_id[max_root];
                if (uf->peaks[pid_k].elev > uf->peaks[pid_max].elev)
                    max_root = neighbor_roots[k];
            }

            /* このピクセルのpeak_idを最高峰グループから引き継ぐ */
            uf->peak_id[i] = uf->peak_id[max_root];
            uf->parent[i]  = max_root;

            /* 全グループを合体 */
            for (int k = 0; k < neighbor_cnt; k++) {
                int32_t root = neighbor_roots[k];
                if (root == max_root) continue;
                int pid = uf->peak_id[root];
                printf("    ピーク(%d,%d) %.1fm の暫定コル更新: %.1fm\n",
                       uf->peaks[pid].x, uf->peaks[pid].y,
                       uf->peaks[pid].elev, elev);
                uf_union(uf, i, root, elev, x, y);
            }
        }
    }

    /* 結果出力 */
    printf("\n=== プロミネンス計算結果 ===\n");
    printf("  %-12s %-8s %-12s %-8s %s\n",
           "ピーク座標","標高","コル座標","コル標高","プロミネンス");

    for (int pid = 0; pid < uf->peak_cnt; pid++) {
        Peak *p = &uf->peaks[pid];
        if (!p->valid) continue;
        if (p->col_elev < -9998.0f) {
            printf("  (%d,%d) %6.1fm  コル未発見(最高峰)\n",
                   p->x, p->y, p->elev);
            continue;
        }
        float prom = p->elev - p->col_elev;
        printf("  (%d,%d) %6.1fm  |  (%d,%d) %6.1fm  |  %6.1fm\n",
               p->x, p->y, p->elev,
               p->col_x, p->col_y, p->col_elev,
               prom);
    }

    uf_destroy(uf);
    printf("\n=== テスト完了 ===\n");
    return 0;
}
