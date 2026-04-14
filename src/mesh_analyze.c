/*
 * mesh_analyze.c - 1次メッシュ単位の解析
 *
 * SOTA日本支部 未登録サミット候補探索ツール
 * Copyright (C) 2026 JJ1XGO
 * GPL-3.0
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <math.h>
#include "mesh_analyze.h"
#include "mesh.h"
#include "elevation.h"
#include "analyze.h"
#include "unionfind.h"
#include "fetch.h"

/*
 * 1次メッシュの全タイルを1枚の巨大ElevTileに展開する
 */
static ElevTile *load_mesh_tile(const char *tile_dir,
                                 const MeshTileRange *range)
{
    uint32_t W = (uint32_t)range->tile_w * TILE_PIX;
    uint32_t H = (uint32_t)range->tile_h * TILE_PIX;

    printf("  イメージサイズ: %u x %u ピクセル\n", W, H);
    printf("  メモリ: %.1f MB\n",
           (double)W * H * sizeof(float) / 1024 / 1024);

    ElevTile *big = malloc(sizeof(ElevTile));
    if (!big) return NULL;
    big->width  = W;
    big->height = H;
    big->data   = calloc(W * H, sizeof(float));
    if (!big->data) { free(big); return NULL; }

    /* 各タイルを読み込んで配置 */
    int total = range->tile_w * range->tile_h;
    int done  = 0;

    for (int ty = range->y_min; ty <= range->y_max; ty++) {
        for (int tx = range->x_min; tx <= range->x_max; tx++) {
            /* タイルの配置先オフセット */
            int dst_x = (tx - range->x_min) * TILE_PIX;
            int dst_y = (ty - range->y_min) * TILE_PIX;

            /* タイルを読み込む(dem5a/b/c優先順) */
            char path[512];
            const char *dems[] = {"a", "b", "c"};
            ElevTile *tile = NULL;

            for (int d = 0; d < 3 && !tile; d++) {
                make_tile_cache_path(path, sizeof(path),
                                     tile_dir, tx, ty, dems[d]);
                tile = elev_load_png(path);
            }

            if (tile) {
                /* タイルデータを巨大イメージにコピー */
                for (uint32_t py = 0; py < tile->height; py++)
                    for (uint32_t px = 0; px < tile->width; px++)
                        big->data[(dst_y + py) * W + (dst_x + px)]
                            = elev_get(tile, px, py);
                elev_destroy(tile);
            }
            /* タイルなし(海・範囲外)は0のまま */

            done++;
            if (done % 500 == 0 || done == total) {
                printf("  タイル読み込み: %d/%d\r", done, total);
                fflush(stdout);
            }
        }
    }
    printf("\n");
    return big;
}

/*
 * ピクセル座標から緯度経度を計算する
 */
static void pixel_to_latlon(const MeshTileRange *range,
                              int px, int py,
                              double *lat, double *lon)
{
    /* ピクセル座標をグローバルタイル座標に変換 */
    double global_px = range->x_min * TILE_PIX + px;
    double global_py = range->y_min * TILE_PIX + py;
    int z = range->z;

    /* タイル座標から緯度経度 */
    double n = pow(2.0, z);
    *lon = global_px / (n * TILE_PIX) * 360.0 - 180.0;
    double mapy = 3.14159265358979 -
                  2.0 * 3.14159265358979 * global_py / (n * TILE_PIX);
    *lat = 180.0 / 3.14159265358979 * atan(sinh(mapy));
}

/*
 * 1次メッシュを解析してCSVに保存する
 */
int mesh_analyze(const MeshAnalyzeConfig *cfg, int meshcode)
{
    printf("メッシュ%d 解析開始\n", meshcode);

    struct timespec ts_start, ts_now;
    clock_gettime(CLOCK_MONOTONIC, &ts_start);

    /* タイル範囲を取得 */
    MeshTileRange range;
    if (mesh_to_tile_range(meshcode, 15, &range) != 0)
        return -1;

    printf("  タイル範囲: x=%d〜%d y=%d〜%d (%d×%d=%d枚)\n",
           range.x_min, range.x_max,
           range.y_min, range.y_max,
           range.tile_w, range.tile_h,
           range.tile_w * range.tile_h);

    /* 全タイルを結合して巨大イメージを作成 */
    ElevTile *big = load_mesh_tile(cfg->tile_dir, &range);
    if (!big) {
        fprintf(stderr, "イメージ作成失敗\n");
        return -1;
    }

    clock_gettime(CLOCK_MONOTONIC, &ts_now);
    printf("  タイル読み込み完了: %.1f秒\n",
           (ts_now.tv_sec - ts_start.tv_sec) +
           (ts_now.tv_nsec - ts_start.tv_nsec) / 1e9);

    /* Union-Find解析 */
    printf("  Union-Find解析開始...\n");
    clock_gettime(CLOCK_MONOTONIC, &ts_start);

    AnalyzeResult *result = analyze_tile_data(big,
                                               big->width, big->height,
                                               cfg->min_prominence);
    elev_destroy(big);

    if (!result) {
        fprintf(stderr, "解析失敗\n");
        return -1;
    }

    clock_gettime(CLOCK_MONOTONIC, &ts_now);
    printf("  解析完了: %.1f秒 / 検出ピーク数: %d\n",
           (ts_now.tv_sec - ts_start.tv_sec) +
           (ts_now.tv_nsec - ts_start.tv_nsec) / 1e9,
           result->peak_cnt);

    /* 結果をCSVに保存 */
    mkdir(cfg->result_dir, 0755);
    char result_path[512];
    snprintf(result_path, sizeof(result_path),
             "%s/%d.csv", cfg->result_dir, meshcode);

    FILE *fp = fopen(result_path, "w");
    if (!fp) {
        fprintf(stderr, "結果ファイルを開けません: %s\n", result_path);
        analyze_result_destroy(result);
        return -1;
    }

    /* ヘッダー */
    fprintf(fp, "peak_lat,peak_lon,peak_elev,"
                "col_lat,col_lon,col_elev,"
                "prominence,is_tile_top\n");

    for (int i = 0; i < result->peak_cnt; i++) {
        PeakResult *p = &result->peaks[i];

        double peak_lat, peak_lon;
        pixel_to_latlon(&range, p->peak_x, p->peak_y,
                        &peak_lat, &peak_lon);

        double col_lat = 0.0, col_lon = 0.0;
        if (!p->is_tile_top)
            pixel_to_latlon(&range, p->col_x, p->col_y,
                            &col_lat, &col_lon);

        fprintf(fp, "%.8f,%.8f,%.2f,%.8f,%.8f,%.2f,%.2f,%d\n",
                peak_lat, peak_lon, p->peak_elev,
                col_lat, col_lon, p->col_elev,
                p->prominence, p->is_tile_top);
    }

    fclose(fp);
    analyze_result_destroy(result);

    printf("  結果保存: %s\n", result_path);
    printf("メッシュ%d 解析完了\n", meshcode);
    return 0;
}
