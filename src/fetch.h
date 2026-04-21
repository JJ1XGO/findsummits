/*
 * fetch.h - 標高タイルのダウンロードとキャッシュ管理
 *
 * SOTA日本支部 未登録サミット候補探索ツール
 * Copyright (C) 2026 JJ1XGO
 * GPL-3.0
 */
#ifndef FETCH_H
#define FETCH_H

#include "mesh.h"

typedef struct {
    const char *tile_dir;    /* タイルキャッシュ基底ディレクトリ (zoom含まず) */
    int         max_parallel;
    int         interval_ms;
} FetchConfig;

/* パス形式: {base}/{z}/{x}/{y}_{dem}.png */
void make_tile_cache_path(char *buf, size_t bufsize,
                          const char *base, int z,
                          int x, int y, const char *dem);

/* dem5 (z=15) のキャッシュ確認 */
int tile_is_cached(const char *base, int x, int y);

/* dem10b (z=14) のキャッシュ確認 */
int tile_dem10b_is_cached(const char *base, int x14, int y14);

/* dem5 タイル取得 (z=15, a→b→c 優先順) */
int fetch_tile(const FetchConfig *cfg, int x, int y);

/* dem10b タイル取得 (z=14) */
int fetch_dem10b_tile(const FetchConfig *cfg, int x14, int y14);

/* 1次メッシュの全dem5タイルを並列ダウンロード */
int fetch_mesh(const FetchConfig *cfg, const MeshTileRange *range);

#endif /* FETCH_H */
