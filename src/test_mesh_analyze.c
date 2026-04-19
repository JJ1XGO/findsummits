/*
 * test_mesh_analyze.c - mesh_analyze.cの動作確認 + 巨大イメージ視覚化機能
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>        // clock_gettime 用
#include <sys/stat.h>    // mkdir 用
#include <png.h>
#include <stdint.h>
#include "mesh_analyze.h"
#include "elevation.h"
#include "mesh.h"        // MeshTileRange 用

static void save_elevation_image(const ElevTile *big, const char *filename);

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "使い方: %s <1次メッシュコード> [--save-image]\n", argv[0]);
        return 1;
    }

    int meshcode = atoi(argv[1]);
    int save_image = 0;

    if (argc >= 3 && strcmp(argv[2], "--save-image") == 0) {
        save_image = 1;
    }

    MeshAnalyzeConfig cfg = {
        .tile_dir      = "/mnt/findsummits/tiles/15",
        .result_dir    = "/mnt/findsummits/results",
        .min_prominence = 150.0f,
    };

    printf("=== SOTA未登録サミット候補探索ツール ===\n");
    printf("対象メッシュ: %d\n\n", meshcode);

    /* タイル範囲を取得 */
    MeshTileRange range;
    if (mesh_to_tile_range(meshcode, 15, &range) != 0) {
        fprintf(stderr, "メッシュ範囲取得失敗\n");
        return 1;
    }

    /* 巨大イメージを作成 */
    ElevTile *big = load_mesh_tile(cfg.tile_dir, &range);
    if (!big) {
        fprintf(stderr, "イメージ作成失敗\n");
        return 1;
    }

    /* --save-image が指定されていれば、ここで即座に出力 */
    if (save_image) {
        char img_path[512];
        snprintf(img_path, sizeof(img_path), "/mnt/findsummits/images/%d_elev.png", meshcode);
        save_elevation_image(big, img_path);
    }

    /* Union-Find解析を実行 */
    printf("  Union-Find解析開始...\n");
    struct timespec ts_start, ts_now;
    clock_gettime(CLOCK_MONOTONIC, &ts_start);

    uint32_t effective_w = TILE_PIX * range.tile_w + 1;
    uint32_t effective_h = TILE_PIX * range.tile_h + 1;

    AnalyzeResult *result = analyze_tile_data(big, effective_w, effective_h, cfg.min_prominence);

    clock_gettime(CLOCK_MONOTONIC, &ts_now);
    printf("  解析完了: %.1f秒 / 検出ピーク数: %d\n",
           (ts_now.tv_sec - ts_start.tv_sec) + (ts_now.tv_nsec - ts_start.tv_nsec)/1e9,
           result ? result->peak_cnt : 0);

    /* 結果をCSVに保存 */
    mkdir(cfg.result_dir, 0755);
    char result_path[512];
    snprintf(result_path, sizeof(result_path), "%s/%d.csv", cfg.result_dir, meshcode);

    if (result) {
        FILE *fp = fopen(result_path, "w");
        if (fp) {
            fprintf(fp, "peak_lat,peak_lon,peak_elev,col_lat,col_lon,col_elev,prominence,is_tile_top\n");
            for (int i = 0; i < result->peak_cnt; i++) {
                PeakResult *p = &result->peaks[i];
                double peak_lat, peak_lon;
                pixel_to_latlon(&range, p->peak_x, p->peak_y, &peak_lat, &peak_lon);
                double col_lat = 0.0, col_lon = 0.0;
                if (!p->is_tile_top)
                    pixel_to_latlon(&range, p->col_x, p->col_y, &col_lat, &col_lon);

                fprintf(fp, "%.8f,%.8f,%.2f,%.8f,%.8f,%.2f,%.2f,%d\n",
                        peak_lat, peak_lon, p->peak_elev,
                        col_lat, col_lon, p->col_elev,
                        p->prominence, p->is_tile_top);
            }
            fclose(fp);
            printf("  結果保存: %s\n", result_path);
        }
        analyze_result_destroy(result);
    }

    elev_destroy(big);

    printf("メッシュ%d 解析完了\n", meshcode);
    return 0;
}

/* ===================================================== */
/* 巨大イメージを標高色付きPNGとして出力（長辺6000px縮小） */
/* terrain系自然な地形色を使用 */
/* ===================================================== */
static void save_elevation_image(const ElevTile *big, const char *filename)
{
    printf("巨大イメージをPNG出力中: %s (長辺6000pxに縮小)\n", filename);

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

    /* まず標高の最大値を調べる（色分けを動的に調整するため） */
    float max_elev = 1.0f;
    for (uint32_t i = 0; i < src_w * src_h; i++) {
        if (big->data[i] > max_elev) max_elev = big->data[i];
    }
    printf("  最高標高: %.1fm\n", max_elev);

    /* libpngで出力 */
    FILE *fp = fopen(filename, "wb");
    if (!fp) { fprintf(stderr, "画像ファイル作成失敗\n"); return; }

    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING,
                                              NULL, NULL, NULL);
    png_infop info = png_create_info_struct(png);
    if (setjmp(png_jmpbuf(png))) {
        png_destroy_write_struct(&png, &info);
        fclose(fp); return;
    }

    png_init_io(png, fp);
    png_set_IHDR(png, info, dst_w, dst_h, 8,
                 PNG_COLOR_TYPE_RGB,
                 PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT,
                 PNG_FILTER_TYPE_DEFAULT);
    png_write_info(png, info);

    uint8_t *row = malloc(dst_w * 3);

    for (uint32_t y = 0; y < dst_h; y++) {
        for (uint32_t x = 0; x < dst_w; x++) {
            uint32_t src_x = (uint32_t)((double)x * src_w / dst_w);
            uint32_t src_y = (uint32_t)((double)y * src_h / dst_h);
            float elev = big->data[src_y * src_w + src_x];

            uint8_t r, g, b;

            if (elev < 0.0f) {
                /* 無効値: 明るいピンク */
                r = 255; g = 100; b = 200;
            } else if (elev < 1.0f) {
                /* 海・0m付近: 濃い青 */
                r = 20; g = 60; b = 150;
            } else if (elev < 50.0f) {
                /* 低平地: 水色 */
                r = 100; g = 180; b = 220;
            } else if (elev < 150.0f) {
                /* 平地: 薄い黄緑 */
                r = 200; g = 230; b = 150;
            } else if (elev < 300.0f) {
                /* 丘陵: 緑 */
                r = 120; g = 200; b = 100;
            } else if (elev < 600.0f) {
                /* 低山: 濃い緑 */
                r = 60; g = 150; b = 60;
            } else if (elev < 1000.0f) {
                /* 中山: 黄茶 */
                r = 180; g = 140; b = 80;
            } else if (elev < 1500.0f) {
                /* 高山: 茶 */
                r = 150; g = 100; b = 50;
            } else if (elev < 2000.0f) {
                /* 高峰: 灰茶 */
                r = 180; g = 160; b = 140;
            } else {
                /* 3000m級: 白 */
                r = 240; g = 240; b = 250;
            }

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
    printf("  → PNG出力完了: %s\n", filename);
}
