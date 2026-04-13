/*
 * mesh.h - 国土地理院メッシュコードとタイル座標の変換
 *
 * SOTA日本支部 未登録サミット候補探索ツール
 * Copyright (C) 2026 JJ1XGO
 * GPL-3.0
 */
#ifndef MESH_H
#define MESH_H

#include <stdint.h>
#include "elevation.h"

/*
 * 1次メッシュの範囲に対応するタイル座標範囲
 */
typedef struct {
    int z;              /* ズームレベル */
    int x_min, x_max;  /* タイルX座標の範囲 */
    int y_min, y_max;  /* タイルY座標の範囲 */
    int tile_w;         /* タイル数(X方向) */
    int tile_h;         /* タイル数(Y方向) */
} MeshTileRange;

/* 関数プロトタイプ */
int  mesh_to_tile_range(int meshcode, int z, MeshTileRange *range);
void latlon_to_tile(double lat, double lon, int z,
                    int *tx, int *ty);
void tile_to_latlon(int z, int tx, int ty,
                    double *lat, double *lon);

#endif /* MESH_H */
