/*
 * elevation.c - 標高タイルの読み込みと標高デコード
 *
 * SOTA日本支部 未登録サミット候補探索ツール
 * Copyright (C) 2026 JJ1XGO
 * GPL-3.0
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <png.h>
#include "elevation.h"

/*
 * RGB値から標高(m)に変換する
 * 国土地理院標高タイル仕様に従う
 */
static float rgb2elev(uint8_t r, uint8_t g, uint8_t b)
{
    uint32_t x = ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;

    /* 無効値チェック */
    if (x == 0x800000)          /* 2^23 */
        return ELEV_NODATA;

    /* 負の標高 */
    if (x > 0x800000)           /* x > 2^23 */
        return (float)((int32_t)x - (1 << 24)) / 100.0f;

    /* 通常の標高 */
    return (float)x / 100.0f;
}

/*
 * PNGファイルから標高タイルを読み込む
 */
ElevTile *elev_load_png(const char *path)
{
    FILE *fp = fopen(path, "rb");
    if (!fp) {
        fprintf(stderr, "elev_load_png: ファイルを開けません: %s\n", path);
        return NULL;
    }

    /* PNGシグネチャ確認 */
    uint8_t sig[8];
    if (fread(sig, 1, 8, fp) != 8 || png_sig_cmp(sig, 0, 8)) {
        fprintf(stderr, "elev_load_png: PNGファイルではありません: %s\n", path);
        fclose(fp);
        return NULL;
    }

    /* libpng初期化 */
    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING,
                                              NULL, NULL, NULL);
    if (!png) { fclose(fp); return NULL; }

    png_infop info = png_create_info_struct(png);
    if (!info) {
        png_destroy_read_struct(&png, NULL, NULL);
        fclose(fp);
        return NULL;
    }

    /* エラー時のジャンプ先 */
    if (setjmp(png_jmpbuf(png))) {
        png_destroy_read_struct(&png, &info, NULL);
        fclose(fp);
        return NULL;
    }

    png_init_io(png, fp);
    png_set_sig_bytes(png, 8);  /* シグネチャ読み済みを通知 */
    png_read_info(png, info);

    uint32_t width  = png_get_image_width(png, info);
    uint32_t height = png_get_image_height(png, info);
    int color_type  = png_get_color_type(png, info);
    int bit_depth   = png_get_bit_depth(png, info);

    /* RGB/RGBAに正規化 */
    if (bit_depth == 16)
        png_set_strip_16(png);
    if (color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_palette_to_rgb(png);
    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
        png_set_expand_gray_1_2_4_to_8(png);
    if (png_get_valid(png, info, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha(png);
    if (color_type == PNG_COLOR_TYPE_GRAY ||
        color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
        png_set_gray_to_rgb(png);

    png_read_update_info(png, info);

    /* ピクセルデータ読み込み */
    uint32_t rowbytes = png_get_rowbytes(png, info);
    uint8_t *imgbuf   = malloc(rowbytes * height);
    png_bytep *rows   = malloc(sizeof(png_bytep) * height);
    if (!imgbuf || !rows) {
        free(imgbuf); free(rows);
        png_destroy_read_struct(&png, &info, NULL);
        fclose(fp);
        return NULL;
    }
    for (uint32_t y = 0; y < height; y++)
        rows[y] = imgbuf + y * rowbytes;

    png_read_image(png, rows);
    png_destroy_read_struct(&png, &info, NULL);
    fclose(fp);

    /* ElevTile作成 */
    ElevTile *tile = malloc(sizeof(ElevTile));
    tile->data     = malloc(sizeof(float) * width * height);
    tile->width    = width;
    tile->height   = height;
    if (!tile->data) {
        free(tile); free(imgbuf); free(rows);
        return NULL;
    }

    /* チャンネル数を確認(RGB=3, RGBA=4) */
    uint32_t channels = rowbytes / width;

    /* RGB→標高変換 */
    for (uint32_t y = 0; y < height; y++) {
        for (uint32_t x = 0; x < width; x++) {
            uint8_t *px = imgbuf + y * rowbytes + x * channels;
            float elev  = rgb2elev(px[0], px[1], px[2]);
            /* 無効値と海面以下は0扱い */
            if (elev == ELEV_NODATA || elev < ELEV_SEA)
                elev = ELEV_SEA;
            tile->data[y * width + x] = elev;
        }
    }

    free(imgbuf);
    free(rows);
    return tile;
}

/*
 * ElevTileを解放する
 */
void elev_destroy(ElevTile *tile)
{
    if (!tile) return;
    free(tile->data);
    free(tile);
}

/*
 * 標高値を取得する
 */
float elev_get(const ElevTile *tile, int x, int y)
{
    return tile->data[y * tile->width + x];
}
