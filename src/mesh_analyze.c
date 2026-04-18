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
 *
 * bigタイルの座標系:
 *   x=0, y=0                          : 外周ボーダー（海面=0、calloc済み）
 *   x=1〜tile_w*TILE_PIX, y=1〜...    : メッシュデータ本体
 *   x=W-1, y=H-1                      : 外周ボーダー（海面=0、calloc済み）
 *
 * 外周ボーダーはcallocで0埋め済みのため追加処理不要。
 * Union-Findはボーダーの0標高をメッシュ外の「海面」として扱い、
 * メッシュ端のピークのプロミネンス計算に利用する。
 */
static ElevTile *load_mesh_tile(const char *tile_dir,
                                 const MeshTileRange *range)
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
    big->data   = calloc(W * H, sizeof(float));  /* 外周ボーダー=0で初期化 */
    if (!big->data) { free(big); return NULL; }

    int total = range->tile_w * range->tile_h;
    int done  = 0;

    for (int ty = range->y_min; ty <= range->y_max; ty++) {
        for (int tx = range->x_min; tx <= range->x_max; tx++) {
            /* メッシュ内グリッド位置（0始まり）から bigタイル上の書き込み先を計算 */
            /* +1 は外周ボーダー1ピクセル分のオフセット */
            int dst_x = (tx - range->x_min) * TILE_PIX + 1;
            int dst_y = (ty - range->y_min) * TILE_PIX + 1;

            TileCoord tc = {15, tx, ty};
            ElevTile *tile = elev_load_with_overlap_8dir(tile_dir, tc);

            if (tile) {
                /* 中央256×256部分のみをコピー（オーバーラップ分を除く） */
                /* elev_load_with_overlap_8dir は258×258を返し、
                 * メインタイルは (1,1)〜(256,256) に配置される */
                for (uint32_t py = 0; py < TILE_PIX; py++) {
                    for (uint32_t px = 0; px < TILE_PIX; px++) {
                        float elev = elev_get(tile, px + 1, py + 1);
                        if (elev == ELEV_NODATA || elev < 0.0f)
                            elev = 0.0f;
                        big->data[(dst_y + py) * W + (dst_x + px)] = elev;
                    }
                }
                /*
                 * [修正] 0リセットブロックを削除。
                 *
                 * 旧コードはここで dst_y-1 行と dst_x-1 列を 0 にリセットして
                 * いたが、これは直前のタイルが書き込んだデータを破壊するバグ。
                 * 例: タイル(gx,gy)のリセットが dst_y-1 = gy*TILE_PIX 行を
                 * 0にすると、タイル(gx,gy-1)の最終行が消える。
                 * 外周ボーダーは calloc で既に 0 になっているため、
                 * リセット処理は不要。
                 */
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
 *
 * px, py は bigタイル上の座標（外周ボーダーが x=0, y=0）。
 * メッシュデータは x=1, y=1 から始まるため、グローバルタイル座標への
 * 変換では -1 のオフセットが必要。
 */
static void pixel_to_latlon(const MeshTileRange *range,
                              int px, int py,
                              double *lat, double *lon)
{
    /* [修正] bigタイルの外周ボーダー分（+1オフセット）を差し引く */
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
    /* [注] メッシュデータは bigタイルの (1,1) から始まるため +1 オフセット */
    float max_elev = 0.0f;
    int valid_pixels = 0;
    for (uint32_t y = 1; y <= (uint32_t)(TILE_PIX * range.tile_h); y++) {
        for (uint32_t x = 1; x <= (uint32_t)(TILE_PIX * range.tile_w); x++) {
            float e = big->data[y * big->width + x];
            if (e > 10.0f) {
                valid_pixels++;
                if (e > max_elev) max_elev = e;
            }
        }
    }
    printf("  巨大イメージ有効ピクセル: %d / %u   最高標高: %.1fm\n",
           valid_pixels,
           (uint32_t)TILE_PIX * range.tile_w * TILE_PIX * range.tile_h,
           max_elev);
    /* ==================================================== */

    /* Union-Find解析 */
    printf("  Union-Find解析開始...\n");
    clock_gettime(CLOCK_MONOTONIC, &ts_start);

    /*
     * [修正] ピーク報告範囲を正しく設定。
     *
     * bigタイルは (tile_w*TILE_PIX + 2) × (tile_h*TILE_PIX + 2)。
     * analyze_tile_data は p->x >= main_w または p->y >= main_h の
     * ピークを除外する（上限チェックのみ）。
     *
     * メッシュデータが x=1〜tile_w*TILE_PIX に存在するため:
     *   main_w = tile_w*TILE_PIX + 1  → x=tile_w*TILE_PIX まで含む
     *   main_h = tile_h*TILE_PIX + 1
     *
     * x=0 の外周ボーダーは calloc で 0 のためピーク検出されない。
     * 旧コードの -16（端から8ピクセル削り）は不要。
     */
    uint32_t effective_w = TILE_PIX * range.tile_w + 1;
    uint32_t effective_h = TILE_PIX * range.tile_h + 1;

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
