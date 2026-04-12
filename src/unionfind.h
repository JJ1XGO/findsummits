/*
 * unionfind.h - Union-Find データ構造
 *
 * SOTA日本支部 未登録サミット候補探索ツール
 * Copyright (C) 2026 JJ1XGO
 * GPL-3.0
 */
#ifndef UNIONFIND_H
#define UNIONFIND_H

#include <stdint.h>
#include <stddef.h>

/*
 * ピーク情報
 */
typedef struct {
    int32_t x, y;          /* 山頂座標 */
    float   elev;          /* 山頂標高(m) */
    float   col_elev;      /* キーコル標高(初期値-9999) */
    int32_t col_x, col_y;  /* キーコル座標 */
    int     valid;         /* 有効フラグ */
} Peak;

/*
 * Union-Find 本体
 */
typedef struct {
    int32_t *parent;    /* 親のインデックス */
    int32_t *rank;      /* 木の深さ */
    int32_t *peak_id;   /* このグループのピークID */
    size_t   size;      /* 全ピクセル数 */

    Peak    *peaks;     /* ピーク情報リスト */
    int      peak_cnt;  /* 登録済みピーク数 */
    int      peak_cap;  /* ピークリストの容量 */
} UnionFind;

/* 関数プロトタイプ */
UnionFind *uf_create(size_t size);
void       uf_destroy(UnionFind *uf);
int32_t    uf_find(UnionFind *uf, int32_t i);
int        uf_union(UnionFind *uf, int32_t a, int32_t b,
                    float col_elev, int32_t col_x, int32_t col_y);
int        uf_new_peak(UnionFind *uf, int32_t i,
                       int32_t x, int32_t y, float elev);

#endif /* UNIONFIND_H */
