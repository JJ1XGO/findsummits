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

static int cmp_int(const void *a, const void *b)
{
    return *(const int *)a - *(const int *)b;
}

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

/*
 * メッシュコードリストファイルを読み込んで MeshSet を構築する
 *
 * list_path: 1行1メッシュコード（4桁整数）のテキストファイル
 * 戻り値: 読み込んだ件数（>=0）/ -1=エラー
 */
int mesh_set_load(MeshSet *set, const char *list_path)
{
    FILE *fp = fopen(list_path, "r");
    if (!fp) {
        fprintf(stderr, "mesh_set_load: 開けない: %s\n", list_path);
        return -1;
    }

    int cap = 256;
    set->codes = malloc(cap * sizeof(int));
    if (!set->codes) {
        fclose(fp);
        return -1;
    }
    set->count = 0;

    char line[64];
    while (fgets(line, sizeof(line), fp)) {
        int code;
        if (sscanf(line, "%d", &code) != 1) continue;
        if (code < 1000 || code > 9999)     continue;
        if (set->count == cap) {
            cap *= 2;
            int *tmp = realloc(set->codes, cap * sizeof(int));
            if (!tmp) {
                free(set->codes);
                fclose(fp);
                set->codes = NULL;
                set->count = 0;
                return -1;
            }
            set->codes = tmp;
        }
        set->codes[set->count++] = code;
    }
    fclose(fp);

    qsort(set->codes, set->count, sizeof(int), cmp_int);
    return set->count;
}

/*
 * MeshSet に指定コードが含まれるか二分探索で調べる
 * 戻り値: 1=含む / 0=含まない
 */
int mesh_set_contains(const MeshSet *set, int code)
{
    int lo = 0, hi = set->count - 1;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        if (set->codes[mid] == code) return 1;
        if (set->codes[mid] <  code) lo = mid + 1;
        else                         hi = mid - 1;
    }
    return 0;
}

void mesh_set_destroy(MeshSet *set)
{
    free(set->codes);
    set->codes = NULL;
    set->count = 0;
}

/*
 * 隣接メッシュコードを計算する
 * dlat, dlon ∈ {-1, 0, 1}（-1=南/西, +1=北/東）
 * 有効範囲チェックなし
 */
int mesh_neighbor(int code, int dlat, int dlon)
{
    int lat_code = code / 100;
    int lon_code = code % 100;
    return (lat_code + dlat) * 100 + (lon_code + dlon);
}
