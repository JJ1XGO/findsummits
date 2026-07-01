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
#include <stdarg.h>
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

/* stdout と logfp（非NULL時）の両方に書き出す */
static void mlog(FILE *logfp, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
    if (logfp) {
        va_start(ap, fmt);
        vfprintf(logfp, fmt, ap);
        va_end(ap);
        fflush(logfp);
    }
}

/*
 * 標高 → RGB変換（Japan Topo カラースキーム）
 *
 * big->data では海面/NODATA が 0.0m に統一されているため、
 * elev <= 0.0 は -1m として扱い陸地と区別する。
 * 配色: docs/decisions/research/dem_colormap.html「Japan Topo」
 */
static void elev_to_rgb(float elev, uint8_t *r, uint8_t *g, uint8_t *b)
{
    static const double stops[] = {
        -6000.0,   /* NODATA センチネル */
           -1.0,   /* 海面（0m以下を一律-1mで表示） */
            0.0,   /* 海岸線（不連続）              */
          300.0,   /* 低地〜台地                   */
          900.0,   /* 山地                         */
         1800.0,   /* 亜高山帯                     */
         2700.0,   /* 高山帯                       */
         3800.0,   /* 頂上域（富士山頂付近）        */
    };
    static const int colors[][3] = {
        { 26,  79, 114},  /* -6000m: #1a4f72 海洋（NODATA） */
        { 91, 163, 201},  /*    -1m: #5ba3c9 沿岸           */
        {232, 245, 200},  /*     0m: #e8f5c8 低地           */
        {168, 208, 141},  /*   300m: #a8d08d 台地           */
        {106, 168,  79},  /*   900m: #6aa84f 山地           */
        {181, 101,  29},  /*  1800m: #b5651d 亜高山         */
        {139, 115,  85},  /*  2700m: #8b7355 高山           */
        {240, 235, 227},  /*  3800m: #f0ebe3 頂上           */
    };
    static const int n = 8;

    double e = (double)elev;

    /* NODATA(-9999) → 最深海色 */
    if (e < -9000.0) {
        *r = colors[0][0]; *g = colors[0][1]; *b = colors[0][2]; return;
    }
    /* big->data では海面・NODATA が 0.0m に変換済みのため
     * 0m以下は -1m（薄い青）として扱い陸地と区別する */
    if (e <= 0.0) e = -1.0;

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
void save_terrain_rgb_image(const ElevTile *big, const char *path, FILE *logfp)
{
    mlog(logfp, "  標高カラーマップ PNG 出力中: %s\n", path);

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
    mlog(logfp, "  元サイズ: %ux%u → 出力サイズ: %ux%u\n", src_w, src_h, dst_w, dst_h);

    FILE *fp = fopen(path, "wb");
    if (!fp) { mlog(logfp, "  画像ファイル作成失敗: %s\n", path); return; }

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
    mlog(logfp, "  → PNG出力完了\n");
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
ElevTile *load_mesh_tile(const char *tile_dir, const MeshTileRange *range, FILE *logfp)
{
    uint32_t W = (uint32_t)range->tile_w * TILE_PIX + 2;
    uint32_t H = (uint32_t)range->tile_h * TILE_PIX + 2;

    mlog(logfp, "  イメージサイズ: %u x %u ピクセル (外周ボーダー込み)\n", W, H);
    mlog(logfp, "  メモリ: %.1f MB\n",
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
    if (logfp) fprintf(logfp, "  タイル読み込み: %d/%d\n", done, total);

    /* dem10b補完 + NODATA/負値→SEA変換 */
    mlog(logfp, "  dem10b補完中...\n");
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
    FILE *logfp = NULL;
    if (cfg->log_dir) {
        mkdir(cfg->log_dir, 0755);
        char log_path[512];
        snprintf(log_path, sizeof(log_path), "%s/%d.log", cfg->log_dir, meshcode);
        logfp = fopen(log_path, "w");
    }

    mlog(logfp, "メッシュ%d 解析開始\n", meshcode);

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
    int lat_code_min = meshcode / 100;
    int lat_code_max = meshcode / 100;
    int lon_code_min = meshcode % 100;
    int lon_code_max = meshcode % 100;
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
            int nb_lat = nb / 100, nb_lon = nb % 100;
            if (nb_lat < lat_code_min) lat_code_min = nb_lat;
            if (nb_lat > lat_code_max) lat_code_max = nb_lat;
            if (nb_lon < lon_code_min) lon_code_min = nb_lon;
            if (nb_lon > lon_code_max) lon_code_max = nb_lon;
        }
    }
    combined.tile_w = combined.x_max - combined.x_min + 1;
    combined.tile_h = combined.y_max - combined.y_min + 1;

    mlog(logfp, "  解析範囲 (3×3最小矩形): x=%d〜%d y=%d〜%d (%d×%d=%d枚)\n",
         combined.x_min, combined.x_max,
         combined.y_min, combined.y_max,
         combined.tile_w, combined.tile_h,
         combined.tile_w * combined.tile_h);

    /* タイルを結合して巨大イメージを作成
     * ※タイルは prefetch_tiles.py で事前取得済みであること */
    ElevTile *big = load_mesh_tile(cfg->tile_dir, &combined, logfp);
    if (!big) {
        mlog(logfp, "イメージ作成失敗\n");
        if (logfp) fclose(logfp);
        return -1;
    }

    clock_gettime(CLOCK_MONOTONIC, &ts_now);
    mlog(logfp, "  タイル読み込み完了: %.1f秒\n",
         (ts_now.tv_sec - ts_start.tv_sec) +
         (ts_now.tv_nsec - ts_start.tv_nsec) / 1e9);

    /* Terrain-RGB イメージ出力 */
    if (cfg->img_dir) {
        mkdir(cfg->img_dir, 0755);
        char img_path[512];
        snprintf(img_path, sizeof(img_path), "%s/%d_terrain.png", cfg->img_dir, meshcode);
        save_terrain_rgb_image(big, img_path, logfp);
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
    mlog(logfp, "  中心メッシュ有効ピクセル: %d   最高標高: %.1fm\n",
         valid_pixels, max_elev);

    /* Union-Find 解析（combined 全体） */
    mlog(logfp, "  Union-Find解析開始...\n");
    clock_gettime(CLOCK_MONOTONIC, &ts_start);

    AnalyzeResult *result = analyze_tile_data(big,
                                               big->width,
                                               big->height,
                                               cfg->min_prominence);
    elev_destroy(big);

    if (!result) {
        mlog(logfp, "解析失敗\n");
        if (logfp) fclose(logfp);
        return -1;
    }

    clock_gettime(CLOCK_MONOTONIC, &ts_now);
    mlog(logfp, "  解析完了: %.1f秒 / 全ピーク数: %d\n",
         (ts_now.tv_sec - ts_start.tv_sec) +
         (ts_now.tv_nsec - ts_start.tv_nsec) / 1e9,
         result->peak_cnt);

    /* mesh_set の地理的範囲内のピークのみ CSV に保存
     * フリンジ（端タイルの境界外ピクセル）で検出されたピークを除外する。
     * フリンジデータ自体は Keyコル検出に必要なため Union-Find では保持し、
     * 出力段階（C側）でフィルタする。 */
    double geo_lat_south = lat_code_min       * 2.0 / 3.0;
    double geo_lat_north = (lat_code_max + 1) * 2.0 / 3.0;
    double geo_lon_west  =  lon_code_min + 100.0;
    double geo_lon_east  =  lon_code_max + 101.0;

    mkdir(cfg->result_dir, 0755);
    char result_path[512];
    snprintf(result_path, sizeof(result_path),
             "%s/%d.csv", cfg->result_dir, meshcode);

    FILE *fp = fopen(result_path, "w");
    if (!fp) {
        mlog(logfp, "結果ファイルを開けません: %s\n", result_path);
        analyze_result_destroy(result);
        if (logfp) fclose(logfp);
        return -1;
    }

    fprintf(fp, "peak_lat,peak_lon,peak_elev,"
                "col_lat,col_lon,col_elev,"
                "prominence,is_tile_top,col_margin_px,center_mesh\n");

    int out_cnt = 0;
    for (int i = 0; i < result->peak_cnt; i++) {
        PeakResult *p = &result->peaks[i];

        double peak_lat, peak_lon;
        pixel_to_latlon(&combined, p->peak_x, p->peak_y,
                        &peak_lat, &peak_lon);

        /* mesh_set 地理範囲外のピークはスキップ */
        if (peak_lat <  geo_lat_south || peak_lat >= geo_lat_north ||
            peak_lon <  geo_lon_west  || peak_lon >= geo_lon_east)
            continue;

        double col_lat = 0.0, col_lon = 0.0;
        if (!p->is_tile_top)
            pixel_to_latlon(&combined, p->col_x, p->col_y,
                            &col_lat, &col_lon);

        fprintf(fp, "%.8f,%.8f,%.2f,%.8f,%.8f,%.2f,%.2f,%d,%d,%d\n",
                peak_lat, peak_lon, p->peak_elev,
                col_lat, col_lon, p->col_elev,
                p->prominence, p->is_tile_top,
                p->col_margin_px, meshcode);
        out_cnt++;
    }
    fclose(fp);
    analyze_result_destroy(result);

    mlog(logfp, "  結果保存: %s (%d件)\n", result_path, out_cnt);
    mlog(logfp, "メッシュ%d 解析完了\n", meshcode);
    if (logfp) fclose(logfp);
    return 0;
}
