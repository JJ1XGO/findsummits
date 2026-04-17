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
 * 1次メッシュの全タイルを1枚の巨大ElevTileに展開する（8方向オーバーラップ対応）
 */
static ElevTile *load_mesh_tile(const char *tile_dir,
                                 const MeshTileRange *range)
{
    uint32_t W = (uint32_t)range->tile_w * TILE_PIX + 2;
    uint32_t H = (uint32_t)range->tile_h * TILE_PIX + 2;

    printf("  イメージサイズ: %u x %u ピクセル (8方向オーバーラップ込み)\n", W, H);
    printf("  メモリ: %.1f MB\n",
           (double)W * H * sizeof(float) / 1024 / 1024);

    ElevTile *big = malloc(sizeof(ElevTile));
    if (!big) return NULL;
    big->width  = W;
    big->height = H;
    big->data   = calloc(W * H, sizeof(float));
    if (!big->data) { free(big); return NULL; }

    int total = range->tile_w * range->tile_h;
    int done  = 0;

    for (int ty = range->y_min; ty <= range->y_max; ty++) {
        for (int tx = range->x_min; tx <= range->x_max; tx++) {
            int dst_x = (tx - range->x_min) * TILE_PIX + 1;
            int dst_y = (ty - range->y_min) * TILE_PIX + 1;

            TileCoord tc = {15, tx, ty};
            ElevTile *tile = elev_load_with_overlap_8dir(tile_dir, tc);

            if (tile) {
                /* 中央256×256だけをコピーし、オーバーラップ領域は明確に0にしておく */
                for (uint32_t py = 0; py < TILE_PIX; py++) {
                    for (uint32_t px = 0; px < TILE_PIX; px++) {
                        float elev = elev_get(tile, px + 1, py + 1);
                        if (elev == ELEV_NODATA || elev < 0.0f) {
                            elev = 0.0f;
                        }
                        big->data[(dst_y + py) * W + (dst_x + px)] = elev;
                    }
                }
                /* オーバーラップ領域（端の1ピクセル分）は明確に0にする */
                /* 上端と下端 */
                for (uint32_t px = 0; px < W; px++) {
                    big->data[(dst_y - 1) * W + px] = 0.0f;
                    big->data[(dst_y + TILE_PIX) * W + px] = 0.0f;
                }
                /* 左端と右端 */
                for (uint32_t py = 0; py < H; py++) {
                    big->data[py * W + (dst_x - 1)] = 0.0f;
                    big->data[py * W + (dst_x + TILE_PIX)] = 0.0f;
                }
                elev_destroy(tile);
            }

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

    /* === デバッグ：巨大イメージに標高が入っているか確認 === */
    float max_elev = 0.0f;
    int valid_pixels = 0;
    for (uint32_t y = 0; y < TILE_PIX * range.tile_h; y++) {
        for (uint32_t x = 0; x < TILE_PIX * range.tile_w; x++) {
            float e = big->data[y * big->width + x];
            if (e > 10.0f) {
                valid_pixels++;
                if (e > max_elev) max_elev = e;
            }
        }
    }
    printf("  巨大イメージ有効ピクセル: %d / %u   最高標高: %.1fm\n",
           valid_pixels, TILE_PIX * range.tile_w * TILE_PIX * range.tile_h, max_elev);
    /* ==================================================== */

    /* Union-Find解析 */
    printf("  Union-Find解析開始...\n");
    clock_gettime(CLOCK_MONOTONIC, &ts_start);

    /* オーバーラップ領域を避けるため、端から8ピクセル内側に絞る */
    uint32_t effective_w = TILE_PIX * range.tile_w - 16;
    uint32_t effective_h = TILE_PIX * range.tile_h - 16;

    AnalyzeResult *result = analyze_tile_data(big,
                                               effective_w,
                                               effective_h,
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
