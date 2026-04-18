/*
 * elevation.h - 標高タイルの読み込みと標高デコード
 *
 * 国土地理院標高タイル仕様:
 *   標高(m) = (R*2^16 + G*2^8 + B) / 100
 *   無効値: R=128, G=0, B=0 (= 2^23)
 *   負の標高: x > 2^23 の時 h = (x - 2^24) / 100
 *
 * SOTA日本支部 未登録サミット候補探索ツール
 * Copyright (C) 2026 JJ1XGO
 * GPL-3.0
 */
#ifndef ELEVATION_H
#define ELEVATION_H

#include <stdint.h>
#include <stddef.h>

#define ELEV_NODATA  -9999.0f   /* 無効値 */
#define ELEV_SEA      0.0f      /* 海面 */
#define TILE_PIX      256       /* タイル1辺のピクセル数 */

/*
 * 標高タイル画像
 * オーバーラップあり: (TILE_PIX+1) × (TILE_PIX+1)
 * オーバーラップなし: TILE_PIX × TILE_PIX
 */
typedef struct {
    float   *data;      /* 標高データ(m) [height][width] */
    uint32_t width;     /* 幅(ピクセル) */
    uint32_t height;    /* 高さ(ピクセル) */
} ElevTile;

/*
 * タイル座標
 */
typedef struct {
    int z, x, y;
} TileCoord;

/* 関数プロトタイプ */
ElevTile *elev_load_png(const char *path);
ElevTile *elev_load_with_overlap(const char *tile_dir,
                                  TileCoord tc);
void      elev_destroy(ElevTile *tile);
float     elev_get(const ElevTile *tile, int x, int y);

ElevTile *elev_load_with_overlap_8dir(const char *tile_dir, TileCoord tc);

#endif /* ELEVATION_H */
