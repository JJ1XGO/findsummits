/*
 * unionfind.c - Union-Find データ構造の実装
 *
 * SOTA日本支部 未登録サミット候補探索ツール
 * Copyright (C) 2026 JJ1XGO
 * GPL-3.0
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "unionfind.h"

#define PEAK_INIT_CAP 1024

UnionFind *uf_create(size_t size)
{
    UnionFind *uf = malloc(sizeof(UnionFind));
    if (!uf) return NULL;

    uf->parent  = malloc(sizeof(int32_t) * size);
    uf->rank    = malloc(sizeof(int32_t) * size);
    uf->peak_id = malloc(sizeof(int32_t) * size);
    uf->peaks   = malloc(sizeof(Peak) * PEAK_INIT_CAP);
    uf->size     = size;
    uf->peak_cnt = 0;
    uf->peak_cap = PEAK_INIT_CAP;

    if (!uf->parent || !uf->rank || !uf->peak_id || !uf->peaks) {
        uf_destroy(uf);
        return NULL;
    }

    for (size_t i = 0; i < size; i++) {
        uf->parent[i]  = (int32_t)i;
        uf->rank[i]    = 0;
        uf->peak_id[i] = -1;
    }
    return uf;
}

void uf_destroy(UnionFind *uf)
{
    if (!uf) return;
    free(uf->parent);
    free(uf->rank);
    free(uf->peak_id);
    free(uf->peaks);
    free(uf);
}

int32_t uf_find(UnionFind *uf, int32_t i)
{
    if (uf->parent[i] != i)
        uf->parent[i] = uf_find(uf, uf->parent[i]);
    return uf->parent[i];
}

int uf_new_peak(UnionFind *uf, int32_t i, int32_t x, int32_t y, float elev)
{
    if (uf->peak_cnt >= uf->peak_cap) {
        int new_cap = uf->peak_cap * 2;
        Peak *tmp = realloc(uf->peaks, sizeof(Peak) * new_cap);
        if (!tmp) return -1;
        uf->peaks    = tmp;
        uf->peak_cap = new_cap;
    }

    int pid = uf->peak_cnt++;
    uf->peaks[pid].x        = x;
    uf->peaks[pid].y        = y;
    uf->peaks[pid].elev     = elev;
    uf->peaks[pid].col_elev = -9999.0f;
    uf->peaks[pid].col_x    = -1;
    uf->peaks[pid].col_y    = -1;
    uf->peaks[pid].valid    = 1;

    uf->peak_id[i] = pid;
    return pid;
}

/*
 * aとbのグループを合体する
 *
 * 同標高ピークの扱い:
 *   両方のコルを更新し、どちらかを代表にする
 */
int uf_union(UnionFind *uf, int32_t a, int32_t b,
             float col_elev, int32_t col_x, int32_t col_y)
{
    int32_t ra = uf_find(uf, a);
    int32_t rb = uf_find(uf, b);

    if (ra == rb) return -1;

    int pid_a = uf->peak_id[ra];
    int pid_b = uf->peak_id[rb];

    int pid_winner, pid_loser;
    int32_t r_winner, r_loser;

    if (uf->peaks[pid_a].elev > uf->peaks[pid_b].elev) {
        /* aの方が高い → aがwinner */
        pid_winner = pid_a; r_winner = ra;
        pid_loser  = pid_b; r_loser  = rb;
        /* loserのキーコルを更新 */
        if (col_elev > uf->peaks[pid_loser].col_elev) {
            uf->peaks[pid_loser].col_elev = col_elev;
            uf->peaks[pid_loser].col_x    = col_x;
            uf->peaks[pid_loser].col_y    = col_y;
        }
    } else if (uf->peaks[pid_b].elev > uf->peaks[pid_a].elev) {
        /* bの方が高い → bがwinner */
        pid_winner = pid_b; r_winner = rb;
        pid_loser  = pid_a; r_loser  = ra;
        /* loserのキーコルを更新 */
        if (col_elev > uf->peaks[pid_loser].col_elev) {
            uf->peaks[pid_loser].col_elev = col_elev;
            uf->peaks[pid_loser].col_x    = col_x;
            uf->peaks[pid_loser].col_y    = col_y;
        }
    } else {
        /* 同標高 → 両方のコルを更新してaをwinnerにする */
        pid_winner = pid_a; r_winner = ra;
        pid_loser  = pid_b; r_loser  = rb;
        if (col_elev > uf->peaks[pid_a].col_elev) {
            uf->peaks[pid_a].col_elev = col_elev;
            uf->peaks[pid_a].col_x    = col_x;
            uf->peaks[pid_a].col_y    = col_y;
        }
        if (col_elev > uf->peaks[pid_b].col_elev) {
            uf->peaks[pid_b].col_elev = col_elev;
            uf->peaks[pid_b].col_x    = col_x;
            uf->peaks[pid_b].col_y    = col_y;
        }
    }

    /* loserをwinnerの子にする */
    uf->parent[r_loser] = r_winner;
    if (uf->rank[r_winner] == uf->rank[r_loser])
        uf->rank[r_winner]++;

    uf->peak_id[r_winner] = pid_winner;

    return 0;
}
