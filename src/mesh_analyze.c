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
#include <stdint.h>
#include <png.h>
#include "mesh_analyze.h"
#include "mesh.h"
#include "elevation.h"
#include "analyze.h"
#include "unionfind.h"

/*
 * 標高 → RGB変換（tests/terrain_viz.c の elevation_to_color() と同一ロジック）
 *
 * big->data では海面/NODATA が 0.0m に統一されているため、
 * elev <= 0.0 は stops[0](-200m) の色（濃紺）で表示して陸地と区別する。
 */
static void elev_to_rgb(float elev, uint8_t *r, uint8_t *g, uint8_t *b)
{
    static const double stops[] = {
        -200.0, 0.0, 150.0, 500.0, 650.0, 850.0, 1100.0, 1500.0, 3000.0, 3800.0
    };
    static const int colors[][3] = {
        { 20,  60, 150},  /* 深海：濃紺          */
        { 65, 150, 210},  /* 海岸線：水色        */
        {180, 220, 140},  /* 低地：黄緑          */
        {120, 185,  80},  /* 丘陵：緑            */
        {190, 160,  90},  /* 山麓：黄茶          */
        {160, 120,  60},  /* 中山：茶色          */
        {130,  90,  50},  /* 高山麓：濃茶        */
        {180, 170, 160},  /* 亜高山：灰色        */
        {220, 215, 210},  /* 高山帯：明るい灰    */
        {255, 255, 255},  /* 山頂付近：白        */
    };
    static const int n = 10;

    double e = (double)elev;

    /* NODATA(-9999) や海面(0m以下) は stops[0] の濃紺で統一 */
    if (e < -9000.0 || e <= 0.0) {
        *r = colors[0][0]; *g = colors[0][1]; *b = colors[0][2]; return;
    }
    if (e >= stops[n - 1]) {
        *r = colors[n-1][0]; *g = colors[n-1][1]; *b = colors[n-1][2]; return;
    }
    for (int i = 0; i < n - 1; i++) {
        if (e >= stops[i] && e < stops[i + 1]) {
            double t = (e - stops[i]) / (stops[i + 1] - stops[i]);
            *r = (uint8_t)(colors[i][0] + t * (colors[i+1][0] - colors[i][0]) + 0.5);
            *g = (uint8_t)(colors[i][1] + t * (colors[i+1][1] - colors[i][1]) + 0.5);
            *b = (uint8_t)(colors[i][2] + t * (colors[i+1][2] - colors[i][2]) + 0.5);
            return;
        }
    }
}

/*
 * 標高カラーマップ PNG を出力する（長辺6000px縮小）
 * 低地=緑 → 中地=黄茶 → 高山=白、NODATA/海=青
 */
static void save_terrain_rgb_image(const ElevTile *big, const char *path)
{
    printf("  標高カラーマップ PNG 出力中: %s\n", path);

    uint32_t src_w = big->width;
    uint32_t src_h = big->height;
    uint32_t dst_w, dst_h;
    if (src_w > src_h) {
        dst_w = 6000;
        dst_h = (uint32_t)((double)src_h * 6000.0 / src_w + 0.5);
    } else {
        dst_h = 6000;
        dst_w = (uint32_t)((double)src_w * 6000.0 / src_h + 0.5);
    }
    printf("  元サイズ: %ux%u → 出力サイズ: %ux%u\n", src_w, src_h, dst_w, dst_h);

    FILE *fp = fopen(path, "wb");
    if (!fp) { fprintf(stderr, "  画像ファイル作成失敗: %s\n", path); return; }

    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    png_infop info  = png_create_info_struct(png);
    if (setjmp(png_jmpbuf(png))) {
        png_destroy_write_struct(&png, &info);
        fclose(fp); return;
    }
    png_init_io(png, fp);
    png_set_IHDR(png, info, dst_w, dst_h, 8,
                 PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_write_info(png, info);

    uint8_t *row = malloc(dst_w * 3);
    for (uint32_t y = 0; y < dst_h; y++) {
        for (uint32_t x = 0; x < dst_w; x++) {
            uint32_t sx = (uint32_t)((double)x * src_w / dst_w);
            uint32_t sy = (uint32_t)((double)y * src_h / dst_h);
            float elev = big->data[sy * src_w + sx];
            uint8_t r, g, b;
            elev_to_rgb(elev, &r, &g, &b);
            row[x * 3 + 0] = r;
            row[x * 3 + 1] = g;
            row[x * 3 + 2] = b;
        }
        png_write_row(png, row);
    }
    free(row);
    png_write_end(png, NULL);
    png_destroy_write_struct(&png, &info);
    fclose(fp);
    printf("  → PNG出力完了\n");
}

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
 * 3×3 最小矩形方式:
 *   中心メッシュ + 隣接メッシュが mesh_set に含まれる方向のみ拡張。
 *   全解析領域のピークを出力し、フィルタは merge.py 側で行う。
 *   タイルは prefetch_tiles.py で事前取得済みであること。
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
     * 3×3 最小矩形の計算:
     * 隣接メッシュが mesh_set に含まれる方向だけ解析範囲を拡張する。
     * 離島・海岸部では実際に存在する方向だけ拡張するため、
     * 常に固定9メッシュになるとは限らない（最小矩形の最適化）。
     */
    MeshTileRange combined = center_range;
    for (int dlat = -1; dlat <= 1; dlat++) {
        for (int dlon = -1; dlon <= 1; dlon++) {
            if (dlat == 0 && dlon == 0) continue;
            int nb = mesh_neighbor(meshcode, dlat, dlon);
            if (!cfg->mesh_set || !mesh_set_contains(cfg->mesh_set, nb))
                continue;
            MeshTileRange nb_range;
            if (mesh_to_tile_range(nb, 15, &nb_range) != 0) continue;
            if (nb_range.x_min < combined.x_min) combined.x_min = nb_range.x_min;
            if (nb_range.x_max > combined.x_max) combined.x_max = nb_range.x_max;
            if (nb_range.y_min < combined.y_min) combined.y_min = nb_range.y_min;
            if (nb_range.y_max > combined.y_max) combined.y_max = nb_range.y_max;
        }
    }
    combined.tile_w = combined.x_max - combined.x_min + 1;
    combined.tile_h = combined.y_max - combined.y_min + 1;

    printf("  解析範囲 (3×3最小矩形): x=%d〜%d y=%d〜%d (%d×%d=%d枚)\n",
           combined.x_min, combined.x_max,
           combined.y_min, combined.y_max,
           combined.tile_w, combined.tile_h,
           combined.tile_w * combined.tile_h);

    /* タイルを結合して巨大イメージを作成
     * ※タイルは prefetch_tiles.py で事前取得済みであること */
    ElevTile *big = load_mesh_tile(cfg->tile_dir, &combined);
    if (!big) {
        fprintf(stderr, "イメージ作成失敗\n");
        return -1;
    }

    clock_gettime(CLOCK_MONOTONIC, &ts_now);
    printf("  タイル読み込み完了: %.1f秒\n",
           (ts_now.tv_sec - ts_start.tv_sec) +
           (ts_now.tv_nsec - ts_start.tv_nsec) / 1e9);

    /* Terrain-RGB イメージ出力 */
    if (cfg->img_dir) {
        mkdir(cfg->img_dir, 0755);
        char img_path[512];
        snprintf(img_path, sizeof(img_path), "%s/%d_terrain.png", cfg->img_dir, meshcode);
        save_terrain_rgb_image(big, img_path);
    }

    /* 簡易統計（中心メッシュ範囲） */
    int cx_min = (center_range.x_min - combined.x_min) * TILE_PIX + 1;
    int cx_max = (center_range.x_max - combined.x_min + 1) * TILE_PIX;
    int cy_min = (center_range.y_min - combined.y_min) * TILE_PIX + 1;
    int cy_max = (center_range.y_max - combined.y_min + 1) * TILE_PIX;

    float max_elev = 0.0f;
    int valid_pixels = 0;
    for (int y = cy_min; y <= cy_max; y++) {
        for (int x = cx_min; x <= cx_max; x++) {
            float e = big->data[(uint32_t)y * big->width + (uint32_t)x];
            if (e > 10.0f) {
                valid_pixels++;
                if (e > max_elev) max_elev = e;
            }
        }
    }
    printf("  中心メッシュ有効ピクセル: %d   最高標高: %.1fm\n",
           valid_pixels, max_elev);

    /* Union-Find 解析（combined 全体） */
    printf("  Union-Find解析開始...\n");
    clock_gettime(CLOCK_MONOTONIC, &ts_start);

    AnalyzeResult *result = analyze_tile_data(big,
                                               big->width,
                                               big->height,
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

    /* 全ピークをCSVに保存（中心メッシュフィルタなし） */
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
                "prominence,is_tile_top,col_margin_px,center_mesh\n");

    for (int i = 0; i < result->peak_cnt; i++) {
        PeakResult *p = &result->peaks[i];

        double peak_lat, peak_lon;
        pixel_to_latlon(&combined, p->peak_x, p->peak_y,
                        &peak_lat, &peak_lon);

        double col_lat = 0.0, col_lon = 0.0;
        if (!p->is_tile_top)
            pixel_to_latlon(&combined, p->col_x, p->col_y,
                            &col_lat, &col_lon);

        fprintf(fp, "%.8f,%.8f,%.2f,%.8f,%.8f,%.2f,%.2f,%d,%d,%d\n",
                peak_lat, peak_lon, p->peak_elev,
                col_lat, col_lon, p->col_elev,
                p->prominence, p->is_tile_top,
                p->col_margin_px, meshcode);
    }

    int out_cnt = result->peak_cnt;
    fclose(fp);
    analyze_result_destroy(result);

    printf("  結果保存: %s (%d件)\n", result_path, out_cnt);
    printf("メッシュ%d 解析完了\n", meshcode);
    return 0;
}
