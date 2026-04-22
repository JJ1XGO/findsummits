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
 * root → peak_id のオープンアドレス法ハッシュマップ
 * キー: root ピクセルインデックス(int32_t, 常に >= 0)
 * 値  : peak_id(int32_t, 常に >= 0)
 * 空セルマーカー: key = -1
 */
typedef struct {
    int32_t *keys;
    int32_t *vals;
    int      cap;   /* 常に 2 のべき乗 */
    int      cnt;
} PeakMap;

/*
 * Union-Find 本体
 */
typedef struct {
    int32_t *parent;    /* 親のインデックス */
    int8_t  *rank;      /* 木の深さ（経路圧縮で上限に達しないが 127 でガード） */
    size_t   size;      /* 全ピクセル数 */

    PeakMap  pm;        /* root → peak_id マッピング */

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

/* ハッシュマップ参照（analyze.c からも使用） */
int        peakmap_get(const PeakMap *pm, int32_t key);

#endif /* UNIONFIND_H */
