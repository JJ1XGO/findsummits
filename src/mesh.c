/*
 * mesh.c - 国土地理院メッシュコードとタイル座標の変換
 *
 * SOTA日本支部 未登録サミット候補探索ツール
 * Copyright (C) 2026 JJ1XGO
 * GPL-3.0
 */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "mesh.h"

#define PI 3.14159265358979323846

/*
 * 緯度経度からタイル座標を求める
 */
void latlon_to_tile(double lat, double lon, int z,
                    int *tx, int *ty)
{
    double x = (lon + 180.0) / 360.0 * pow(2.0, z);
    double sinlat = sin(lat * PI / 180.0);
    double y = (1.0 - log((sinlat + 1.0) /
                cos(lat * PI / 180.0)) / PI) / 2.0 * pow(2.0, z);
    *tx = (int)x;
    *ty = (int)y;
}

/*
 * タイル座標から緯度経度を求める(タイルの北西端)
 */
void tile_to_latlon(int z, int tx, int ty,
                    double *lat, double *lon)
{
    double n = pow(2.0, z);
    *lon = tx / n * 360.0 - 180.0;
    double mapy = PI - 2.0 * PI * ty / n;
    *lat = 180.0 / PI * atan(sinh(mapy));
}

/*
 * 1次メッシュコードからタイル座標範囲を求める
 *
 * 1次メッシュコード(4桁): 上2桁=緯度コード 下2桁=経度コード
 *   緯度 = コード * 2/3 度
 *   経度 = コード + 100 度
 *   範囲 = 緯度2/3度 × 経度1度
 *
 * 戻り値: 0=成功 / -1=エラー
 */
int mesh_to_tile_range(int meshcode, int z, MeshTileRange *range)
{
    if (meshcode < 1000 || meshcode > 9999) {
        fprintf(stderr, "mesh_to_tile_range: 無効なメッシュコード: %d\n",
                meshcode);
        return -1;
    }

    int lat_code = meshcode / 100;
    int lon_code = meshcode % 100;

    /* 1次メッシュの北西端・南東端の緯度経度 */
    double lat_north = (lat_code + 1) * 2.0 / 3.0;  /* 北端 */
    double lat_south =  lat_code      * 2.0 / 3.0;  /* 南端 */
    double lon_west  =  lon_code + 100.0;            /* 西端 */
    double lon_east  =  lon_code + 101.0;            /* 東端 */

    printf("メッシュ%d: 北%.4f° 南%.4f° 西%.4f° 東%.4f°\n",
           meshcode, lat_north, lat_south, lon_west, lon_east);

    /* タイル座標に変換 */
    int tx_west, ty_north, tx_east, ty_south;
    latlon_to_tile(lat_north, lon_west,  z, &tx_west,  &ty_north);
    latlon_to_tile(lat_south, lon_east,  z, &tx_east,  &ty_south);

    range->z     = z;
    range->x_min = tx_west;
    range->x_max = tx_east;
    range->y_min = ty_north;
    range->y_max = ty_south;
    range->tile_w = tx_east  - tx_west  + 1;
    range->tile_h = ty_south - ty_north + 1;

    return 0;
}
