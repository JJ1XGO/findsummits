/*
 * test_mesh_analyze.c - mesh_analyze.cの動作確認 + 巨大イメージ視覚化機能
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <png.h>
#include <stdint.h>
#include "mesh_analyze.h"
#include "elevation.h"
#include "mesh.h"

static void save_terrain_rgb_image(const ElevTile *big, const char *filename);

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "使い方: %s <1次メッシュコード> [--save-image]\n", argv[0]);
        return 1;
    }

    int meshcode = atoi(argv[1]);
    int save_image = (argc >= 3 && strcmp(argv[2], "--save-image") == 0);

    MeshAnalyzeConfig cfg = {
        .tile_dir       = "/mnt/findsummits/tiles",
        .result_dir     = "/mnt/findsummits/results/csv",
        .min_prominence = 150.0f,
    };

    printf("=== SOTA未登録サミット候補探索ツール (テスト) ===\n");
    printf("対象メッシュ: %d\n\n", meshcode);

    /* --save-image: 中心メッシュのbig imageをTerrain-RGB出力 */
    if (save_image) {
        MeshTileRange center_range;
        if (mesh_to_tile_range(meshcode, 15, &center_range) == 0) {
            ElevTile *big = load_mesh_tile(cfg.tile_dir, &center_range);
            if (big) {
                char img_path[512];
                snprintf(img_path, sizeof(img_path),
                         "/mnt/findsummits/images/%d_terrain.png", meshcode);
                save_terrain_rgb_image(big, img_path);
                elev_destroy(big);
            }
        }
    }

    /* mesh_analyze で3×3解析・CSV出力 */
    struct timespec ts_start, ts_end;
    clock_gettime(CLOCK_MONOTONIC, &ts_start);

    int ret = mesh_analyze(&cfg, meshcode);

    clock_gettime(CLOCK_MONOTONIC, &ts_end);
    double elapsed = (ts_end.tv_sec - ts_start.tv_sec) +
                     (ts_end.tv_nsec - ts_start.tv_nsec) / 1e9;

    if (ret == 0)
        printf("\n解析完了 (%.1f秒)\n", elapsed);
    else
        fprintf(stderr, "解析失敗\n");

    return ret;
}

/*
 * 標高データを Terrain-RGB 形式 PNG で出力する (長辺6000px縮小)
 *
 * 国土地理院標高タイル互換エンコード:
 *   x = round(h * 100)
 *   x >= 0: R=x>>16, G=(x>>8)&0xFF, B=x&0xFF
 *   x < 0:  x += 2^24, 同様に分解
 *   NODATA: R=128, G=0, B=0
 */
static void save_terrain_rgb_image(const ElevTile *big, const char *filename)
{
    printf("Terrain-RGB PNG 出力中: %s\n", filename);

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

    FILE *fp = fopen(filename, "wb");
    if (!fp) { fprintf(stderr, "画像ファイル作成失敗: %s\n", filename); return; }

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
            uint32_t sx = (uint32_t)((double)x * src_w / dst_w);
            uint32_t sy = (uint32_t)((double)y * src_h / dst_h);
            float elev = big->data[sy * src_w + sx];

            uint8_t r, g, b;
            if (elev < -9000.0f) {
                r = 128; g = 0; b = 0;  /* NODATA */
            } else {
                int32_t val = (int32_t)(elev * 100.0f + 0.5f);
                uint32_t x24 = (val < 0)
                    ? (uint32_t)(val + (1 << 24))
                    : (uint32_t)val;
                r = (x24 >> 16) & 0xFF;
                g = (x24 >>  8) & 0xFF;
                b =  x24        & 0xFF;
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
