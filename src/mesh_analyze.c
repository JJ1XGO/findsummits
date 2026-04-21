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
 * メッシュ範囲に含まれる全タイルを1枚の巨大ElevTileに展開する
 *
 * bigタイルの座標系:
 *   x=0, y=0         : 外周ボーダー（海面=0）
 *   x=1〜, y=1〜     : メッシュデータ本体
 *
 * 外周ボーダーはcallocで0埋め済み。
 */
ElevTile *load_mesh_tile(const char *tile_dir, const MeshTileRange *range)
{
    uint32_t W = (uint32_t)range->tile_w * TILE_PIX + 2;
    uint32_t H = (uint32_t)range->tile_h * TILE_PIX + 2;

    printf("  イメージサイズ: %u x %u ピクセル (外周ボーダー込み)\n", W, H);
    printf("  メモリ: %.1f MB\n",
           (double)W * H * sizeof(float) / 1024 / 1024);

    ElevTile *big = malloc(sizeof(ElevTile));
    if (!big) return NULL;
    big->width  = W;
    big->height = H;
    big->data   = malloc(W * H * sizeof(float));
    if (!big->data) { free(big); return NULL; }

    /* NODATA で初期化 (elev_fill_nodata_dem10b で最後に SEA 変換) */
    for (uint32_t i = 0; i < W * H; i++)
        big->data[i] = ELEV_NODATA;

    int total = range->tile_w * range->tile_h;
    int done  = 0;

    for (int ty = range->y_min; ty <= range->y_max; ty++) {
        for (int tx = range->x_min; tx <= range->x_max; tx++) {
            int dst_x = (tx - range->x_min) * TILE_PIX + 1;
            int dst_y = (ty - range->y_min) * TILE_PIX + 1;

            elev_load_tile_into_big(big, dst_x, dst_y, tile_dir, tx, ty);

            done++;
            if (done % 500 == 0 || done == total) {
                printf("  タイル読み込み: %d/%d\r", done, total);
                fflush(stdout);
            }
        }
    }
    printf("\n");

    /* dem10b補完 + NODATA/負値→SEA変換 */
    printf("  dem10b補完中...\n");
    elev_fill_nodata_dem10b(big, tile_dir, range->x_min, range->y_min);

    return big;
}

/*
 * ピクセル座標から緯度経度を計算する
 *
 * combined_range に対する big タイル上の座標 (px, py) を入力。
 * 外周ボーダー 1px のオフセットを差し引いてグローバルタイル座標に変換する。
 */
void pixel_to_latlon(const MeshTileRange *range,
                     int px, int py,
                     double *lat, double *lon)
{
    double global_px = range->x_min * TILE_PIX + (px - 1);
    double global_py = range->y_min * TILE_PIX + (py - 1);
    int z = range->z;

    double n = pow(2.0, z);
    *lon = global_px / (n * TILE_PIX) * 360.0 - 180.0;
    double mapy = 3.14159265358979 -
                  2.0 * 3.14159265358979 * global_py / (n * TILE_PIX);
    *lat = 180.0 / 3.14159265358979 * atan(sinh(mapy));
}

/*
 * 1次メッシュを解析してCSVに保存する
 *
 * 中心メッシュ + BORDER_TILES タイルオーバーラップ方式:
 *   境界またぎのコル問題を解消しつつ、メモリ使用量を抑制する。
 *   Union-Find は拡張範囲全体で実行し、結果は中心メッシュ内のピークのみ出力。
 */
int mesh_analyze(const MeshAnalyzeConfig *cfg, int meshcode)
{
    printf("メッシュ%d 解析開始\n", meshcode);

    struct timespec ts_start, ts_now;
    clock_gettime(CLOCK_MONOTONIC, &ts_start);

    /* 中心メッシュのタイル範囲 */
    MeshTileRange center_range;
    if (mesh_to_tile_range(meshcode, 15, &center_range) != 0)
        return -1;

    /*
     * 解析範囲: 中心メッシュ + 周囲 BORDER_TILES タイル分のオーバーラップ
     *
     * 3×3フルメッシュは ~4B ピクセルになりメモリ不足になるため、
     * 固定タイル数のパディングで代替する。
     * BORDER_TILES = 8 ≈ 8 × 1.2km ≈ 10km。
     * 日本のSOTA 150m突出の場合、キーコルはほぼこの範囲に収まる。
     * 範囲外はSEA(0)として扱われ、is_tile_top=1 フラグで識別できる。
     */
    #define BORDER_TILES 8

    MeshTileRange combined = {
        .z      = 15,
        .x_min  = center_range.x_min - BORDER_TILES,
        .x_max  = center_range.x_max + BORDER_TILES,
        .y_min  = center_range.y_min - BORDER_TILES,
        .y_max  = center_range.y_max + BORDER_TILES,
    };
    combined.tile_w = combined.x_max - combined.x_min + 1;
    combined.tile_h = combined.y_max - combined.y_min + 1;

    printf("  解析範囲 (中心+%dタイル): x=%d〜%d y=%d〜%d (%d×%d=%d枚)\n",
           BORDER_TILES,
           combined.x_min, combined.x_max,
           combined.y_min, combined.y_max,
           combined.tile_w, combined.tile_h,
           combined.tile_w * combined.tile_h);

    /* 中心メッシュのピクセル境界 (combined big タイル内での座標) */
    int cx_min = (center_range.x_min - combined.x_min) * TILE_PIX + 1;
    int cx_max = (center_range.x_max - combined.x_min + 1) * TILE_PIX;
    int cy_min = (center_range.y_min - combined.y_min) * TILE_PIX + 1;
    int cy_max = (center_range.y_max - combined.y_min + 1) * TILE_PIX;

    /* タイルをダウンロード (未キャッシュ分のみ) */
    FetchConfig fetch_cfg = {
        .tile_dir    = cfg->tile_dir,
        .max_parallel = 4,
        .interval_ms  = 200,
    };
    fetch_mesh(&fetch_cfg, &combined);

    /* 全タイルを結合して巨大イメージを作成 */
    ElevTile *big = load_mesh_tile(cfg->tile_dir, &combined);
    if (!big) {
        fprintf(stderr, "イメージ作成失敗\n");
        return -1;
    }

    clock_gettime(CLOCK_MONOTONIC, &ts_now);
    printf("  タイル読み込み完了: %.1f秒\n",
           (ts_now.tv_sec - ts_start.tv_sec) +
           (ts_now.tv_nsec - ts_start.tv_nsec) / 1e9);

    /* 簡易統計 */
    float max_elev = 0.0f;
    int valid_pixels = 0;
    for (uint32_t y = (uint32_t)cy_min; y <= (uint32_t)cy_max; y++) {
        for (uint32_t x = (uint32_t)cx_min; x <= (uint32_t)cx_max; x++) {
            float e = big->data[y * big->width + x];
            if (e > 10.0f) {
                valid_pixels++;
                if (e > max_elev) max_elev = e;
            }
        }
    }
    printf("  中心メッシュ有効ピクセル: %d   最高標高: %.1fm\n",
           valid_pixels, max_elev);

    /* Union-Find 解析 (3×3 全体) */
    printf("  Union-Find解析開始...\n");
    clock_gettime(CLOCK_MONOTONIC, &ts_start);

    uint32_t effective_w = combined.tile_w * TILE_PIX + 1;
    uint32_t effective_h = combined.tile_h * TILE_PIX + 1;

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
    printf("  解析完了: %.1f秒 / 全ピーク数: %d\n",
           (ts_now.tv_sec - ts_start.tv_sec) +
           (ts_now.tv_nsec - ts_start.tv_nsec) / 1e9,
           result->peak_cnt);

    /* 中心メッシュ内のピークのみCSVに保存 */
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

    fprintf(fp, "peak_lat,peak_lon,peak_elev,"
                "col_lat,col_lon,col_elev,"
                "prominence,is_tile_top\n");

    int out_cnt = 0;
    for (int i = 0; i < result->peak_cnt; i++) {
        PeakResult *p = &result->peaks[i];

        /* 中心メッシュ外のピークは除外 */
        if (p->peak_x < cx_min || p->peak_x > cx_max ||
            p->peak_y < cy_min || p->peak_y > cy_max)
            continue;

        double peak_lat, peak_lon;
        pixel_to_latlon(&combined, p->peak_x, p->peak_y,
                        &peak_lat, &peak_lon);

        double col_lat = 0.0, col_lon = 0.0;
        if (!p->is_tile_top)
            pixel_to_latlon(&combined, p->col_x, p->col_y,
                            &col_lat, &col_lon);

        fprintf(fp, "%.8f,%.8f,%.2f,%.8f,%.8f,%.2f,%.2f,%d\n",
                peak_lat, peak_lon, p->peak_elev,
                col_lat, col_lon, p->col_elev,
                p->prominence, p->is_tile_top);
        out_cnt++;
    }

    fclose(fp);
    analyze_result_destroy(result);

    printf("  結果保存: %s (%d件)\n", result_path, out_cnt);
    printf("メッシュ%d 解析完了\n", meshcode);
    return 0;
}
