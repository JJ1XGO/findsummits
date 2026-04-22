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

#define PEAK_INIT_CAP    1024
#define PEAKMAP_INIT_CAP 262144   /* 全ピーク見込み 200K × load factor 0.5 */
#define PEAKMAP_EMPTY    (-1)

/* ---- PeakMap 内部実装 ---- */

static int peakmap_init(PeakMap *pm, int cap)
{
    pm->keys = malloc(sizeof(int32_t) * cap);
    pm->vals = malloc(sizeof(int32_t) * cap);
    if (!pm->keys || !pm->vals) {
        free(pm->keys);
        free(pm->vals);
        pm->keys = NULL;
        pm->vals = NULL;
        return -1;
    }
    pm->cap = cap;
    pm->cnt = 0;
    memset(pm->keys, 0xff, sizeof(int32_t) * cap);  /* 全セルを -1 に初期化 */
    return 0;
}

static void peakmap_free_internal(PeakMap *pm)
{
    free(pm->keys);
    free(pm->vals);
    pm->keys = NULL;
    pm->vals = NULL;
    pm->cap  = 0;
    pm->cnt  = 0;
}

/* grow/put の相互呼び出しを避けるための生挿入ヘルパ（リサイズなし） */
static void peakmap_insert_raw(PeakMap *pm, int32_t key, int32_t val)
{
    int slot = (uint32_t)key & (uint32_t)(pm->cap - 1);
    while (pm->keys[slot] != PEAKMAP_EMPTY && pm->keys[slot] != key)
        slot = (slot + 1) & (pm->cap - 1);
    if (pm->keys[slot] == PEAKMAP_EMPTY) pm->cnt++;
    pm->keys[slot] = key;
    pm->vals[slot] = val;
}

static int peakmap_grow(PeakMap *pm)
{
    int new_cap    = pm->cap * 2;
    int32_t *old_k = pm->keys;
    int32_t *old_v = pm->vals;
    int old_cap    = pm->cap;

    pm->keys = malloc(sizeof(int32_t) * new_cap);
    pm->vals = malloc(sizeof(int32_t) * new_cap);
    if (!pm->keys || !pm->vals) {
        free(pm->keys);
        free(pm->vals);
        pm->keys = old_k;
        pm->vals = old_v;
        return -1;
    }
    pm->cap = new_cap;
    pm->cnt = 0;
    memset(pm->keys, 0xff, sizeof(int32_t) * new_cap);

    for (int i = 0; i < old_cap; i++) {
        if (old_k[i] != PEAKMAP_EMPTY)
            peakmap_insert_raw(pm, old_k[i], old_v[i]);
    }
    free(old_k);
    free(old_v);
    return 0;
}

static int peakmap_put(PeakMap *pm, int32_t key, int32_t val)
{
    if (pm->cnt * 2 >= pm->cap) {
        if (peakmap_grow(pm) != 0) return -1;
    }
    peakmap_insert_raw(pm, key, val);
    return 0;
}

int peakmap_get(const PeakMap *pm, int32_t key)
{
    int slot = (uint32_t)key & (uint32_t)(pm->cap - 1);
    while (pm->keys[slot] != PEAKMAP_EMPTY) {
        if (pm->keys[slot] == key) return pm->vals[slot];
        slot = (slot + 1) & (pm->cap - 1);
    }
    return -1;
}

/* ---- UnionFind 実装 ---- */

UnionFind *uf_create(size_t size)
{
    UnionFind *uf = malloc(sizeof(UnionFind));
    if (!uf) return NULL;

    uf->parent = malloc(sizeof(int32_t) * size);
    uf->rank   = malloc(sizeof(int8_t)  * size);
    uf->peaks  = malloc(sizeof(Peak)    * PEAK_INIT_CAP);
    uf->size     = size;
    uf->peak_cnt = 0;
    uf->peak_cap = PEAK_INIT_CAP;

    if (!uf->parent || !uf->rank || !uf->peaks) {
        uf_destroy(uf);
        return NULL;
    }

    if (peakmap_init(&uf->pm, PEAKMAP_INIT_CAP) != 0) {
        uf_destroy(uf);
        return NULL;
    }

    for (size_t i = 0; i < size; i++) {
        uf->parent[i] = (int32_t)i;
        uf->rank[i]   = 0;
    }
    return uf;
}

void uf_destroy(UnionFind *uf)
{
    if (!uf) return;
    free(uf->parent);
    free(uf->rank);
    peakmap_free_internal(&uf->pm);
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

    peakmap_put(&uf->pm, i, pid);
    return pid;
}

/*
 * aとbのグループを合体する
 *
 * [修正] peak_id < 0 のノードが渡された場合は早期リターン。
 * analyze.c 側でフィルタしているが念のため防御する。
 */
int uf_union(UnionFind *uf, int32_t a, int32_t b,
             float col_elev, int32_t col_x, int32_t col_y)
{
    int32_t ra = uf_find(uf, a);
    int32_t rb = uf_find(uf, b);

    if (ra == rb) return -1;

    int pid_a = peakmap_get(&uf->pm, ra);
    int pid_b = peakmap_get(&uf->pm, rb);

    /* どちらかが海面コンポーネント（peak_id=-1）なら処理しない */
    if (pid_a < 0 || pid_b < 0) return -1;

    int pid_winner, pid_loser;
    int32_t r_winner, r_loser;

    if (uf->peaks[pid_a].elev > uf->peaks[pid_b].elev) {
        pid_winner = pid_a; r_winner = ra;
        pid_loser  = pid_b; r_loser  = rb;
        if (col_elev > uf->peaks[pid_loser].col_elev) {
            uf->peaks[pid_loser].col_elev = col_elev;
            uf->peaks[pid_loser].col_x    = col_x;
            uf->peaks[pid_loser].col_y    = col_y;
        }
    } else if (uf->peaks[pid_b].elev > uf->peaks[pid_a].elev) {
        pid_winner = pid_b; r_winner = rb;
        pid_loser  = pid_a; r_loser  = ra;
        if (col_elev > uf->peaks[pid_loser].col_elev) {
            uf->peaks[pid_loser].col_elev = col_elev;
            uf->peaks[pid_loser].col_x    = col_x;
            uf->peaks[pid_loser].col_y    = col_y;
        }
    } else {
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

    uf->parent[r_loser] = r_winner;
    if (uf->rank[r_winner] == uf->rank[r_loser] && uf->rank[r_winner] < 127)
        uf->rank[r_winner]++;

    peakmap_put(&uf->pm, r_winner, pid_winner);

    return 0;
}
