/*
 * elevation.c - 標高タイルの読み込みと標高デコード
 *
 * SOTA日本支部 未登録サミット候補探索ツール
 * Copyright (C) 2026 JJ1XGO
 * GPL-3.0
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <png.h>
#include "elevation.h"

static float rgb2elev(uint8_t r, uint8_t g, uint8_t b)
{
    uint32_t x = ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
    if (x == 0x800000) return ELEV_NODATA;
    if (x > 0x800000)  return (float)((int32_t)x - (1 << 24)) / 100.0f;
    return (float)x / 100.0f;
}

static uint8_t *load_png_rgb(const char *path,
                              uint32_t *out_w, uint32_t *out_h,
                              uint32_t *out_channels)
{
    FILE *fp = fopen(path, "rb");
    if (!fp) return NULL;

    uint8_t sig[8];
    if (fread(sig, 1, 8, fp) != 8 || png_sig_cmp(sig, 0, 8)) {
        fclose(fp); return NULL;
    }

    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING,
                                              NULL, NULL, NULL);
    if (!png) { fclose(fp); return NULL; }

    png_infop info = png_create_info_struct(png);
    if (!info) {
        png_destroy_read_struct(&png, NULL, NULL);
        fclose(fp); return NULL;
    }

    if (setjmp(png_jmpbuf(png))) {
        png_destroy_read_struct(&png, &info, NULL);
        fclose(fp); return NULL;
    }

    png_init_io(png, fp);
    png_set_sig_bytes(png, 8);
    png_read_info(png, info);

    uint32_t width    = png_get_image_width(png, info);
    uint32_t height   = png_get_image_height(png, info);
    int color_type    = png_get_color_type(png, info);
    int bit_depth     = png_get_bit_depth(png, info);

    if (bit_depth == 16) png_set_strip_16(png);
    if (color_type == PNG_COLOR_TYPE_PALETTE) png_set_palette_to_rgb(png);
    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
        png_set_expand_gray_1_2_4_to_8(png);
    if (png_get_valid(png, info, PNG_INFO_tRNS)) png_set_tRNS_to_alpha(png);
    if (color_type == PNG_COLOR_TYPE_GRAY ||
        color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_gray_to_rgb(png);
    png_read_update_info(png, info);

    uint32_t rowbytes = png_get_rowbytes(png, info);
    uint8_t *imgbuf   = malloc(rowbytes * height);
    png_bytep *rows   = malloc(sizeof(png_bytep) * height);
    if (!imgbuf || !rows) {
        free(imgbuf); free(rows);
        png_destroy_read_struct(&png, &info, NULL);
        fclose(fp); return NULL;
    }
    for (uint32_t y = 0; y < height; y++)
        rows[y] = imgbuf + y * rowbytes;

    png_read_image(png, rows);
    png_destroy_read_struct(&png, &info, NULL);
    fclose(fp);
    free(rows);

    *out_w        = width;
    *out_h        = height;
    *out_channels = rowbytes / width;
    return imgbuf;
}

static void copy_rgb_to_tile(ElevTile *dst,
                              int dst_x, int dst_y,
                              const uint8_t *rgb,
                              uint32_t src_w, uint32_t channels,
                              int src_x0, int src_y0,
                              int src_x1, int src_y1)
{
    for (int sy = src_y0; sy < src_y1; sy++) {
        for (int sx = src_x0; sx < src_x1; sx++) {
            const uint8_t *px = rgb + sy * src_w * channels + sx * channels;
            float elev = rgb2elev(px[0], px[1], px[2]);
            if (elev == ELEV_NODATA || elev < ELEV_SEA)
                elev = ELEV_SEA;
            int dx = dst_x + (sx - src_x0);
            int dy = dst_y + (sy - src_y0);
            dst->data[dy * dst->width + dx] = elev;
        }
    }
}

ElevTile *elev_load_png(const char *path)
{
    uint32_t w, h, ch;
    uint8_t *rgb = load_png_rgb(path, &w, &h, &ch);
    if (!rgb) {

        return NULL;
    }

    ElevTile *tile = malloc(sizeof(ElevTile));
    tile->width    = w;
    tile->height   = h;
    tile->data     = malloc(sizeof(float) * w * h);
    if (!tile->data) { free(rgb); free(tile); return NULL; }

    copy_rgb_to_tile(tile, 0, 0, rgb, w, ch, 0, 0, w, h);
    free(rgb);
    return tile;
}

/*
 * タイルパスを組み立てる
 * 形式: {tile_dir}/{x}/{y}_{dem}.png
 */
static void make_tile_path(char *buf, size_t bufsize,
                            const char *tile_dir,
                            int x, int y, const char *dem)
{
    snprintf(buf, bufsize, "%s/%d/%d_%s.png", tile_dir, x, y, dem);
}

/*
 * タイルを読み込んでElevTileの指定領域にコピーする
 * 失敗しても0埋めのまま処理を続ける
 */
static void load_tile_into(ElevTile *dst,
                            int dst_x, int dst_y,
                            const char *tile_dir,
                            int tx, int ty,
                            int src_x0, int src_y0,
                            int src_x1, int src_y1)
{
    char path[512];
    uint32_t w, h, ch;
    uint8_t *rgb = NULL;

    /* dem5a/b/cの優先順で試す */
    const char *dems[] = {"a", "b", "c"};
    for (int d = 0; d < 3 && !rgb; d++) {
        make_tile_path(path, sizeof(path), tile_dir, tx, ty, dems[d]);
        rgb = load_png_rgb(path, &w, &h, &ch);
    }

    if (!rgb) return;  /* タイルなし→0埋めのまま */

    copy_rgb_to_tile(dst, dst_x, dst_y, rgb, w, ch,
                     src_x0, src_y0, src_x1, src_y1);
    free(rgb);
}

ElevTile *elev_load_with_overlap(const char *tile_dir, TileCoord tc)
{
    uint32_t W = TILE_PIX + 1;  /* 257 */
    uint32_t H = TILE_PIX + 1;  /* 257 */

    ElevTile *tile = malloc(sizeof(ElevTile));
    tile->width    = W;
    tile->height   = H;
    tile->data     = calloc(W * H, sizeof(float));
    if (!tile->data) { free(tile); return NULL; }

    /* メインタイル: 256×256を(0,0)に配置 */
    load_tile_into(tile, 0, 0,
                   tile_dir, tc.x, tc.y,
                   0, 0, TILE_PIX, TILE_PIX);

    /* 右隣タイル: 左端1列を(256,0)に配置 */
    load_tile_into(tile, TILE_PIX, 0,
                   tile_dir, tc.x+1, tc.y,
                   0, 0, 1, TILE_PIX);

    /* 下隣タイル: 上端1行を(0,256)に配置 */
    load_tile_into(tile, 0, TILE_PIX,
                   tile_dir, tc.x, tc.y+1,
                   0, 0, TILE_PIX, 1);

    /* 右下隣タイル: 左上1ピクセルを(256,256)に配置 */
    load_tile_into(tile, TILE_PIX, TILE_PIX,
                   tile_dir, tc.x+1, tc.y+1,
                   0, 0, 1, 1);

    return tile;
}

void elev_destroy(ElevTile *tile)
{
    if (!tile) return;
    free(tile->data);
    free(tile);
}

float elev_get(const ElevTile *tile, int x, int y)
{
    return tile->data[y * tile->width + x];
}
