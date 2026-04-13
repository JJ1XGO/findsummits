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
    const char *tile_dir;
    int         max_parallel;   /* 並列数(推奨: 4〜8) */
    int         interval_ms;    /* DL成功時のウェイト(ms) */
} FetchConfig;

int  fetch_tile(const FetchConfig *cfg, int x, int y);
int  fetch_mesh(const FetchConfig *cfg, const MeshTileRange *range);
int  tile_is_cached(const char *tile_dir, int x, int y);
void make_tile_cache_path(char *buf, size_t bufsize,
                          const char *tile_dir,
                          int x, int y, const char *dem);

#endif /* FETCH_H */
