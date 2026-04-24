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
#include <sys/stat.h>
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

/* パス形式: {base}/{z}/{x}/{y}_{dem}.png */
static void make_tile_path(char *buf, size_t bufsize,
                            const char *base, int z,
                            int x, int y, const char *dem)
{
    snprintf(buf, bufsize, "%s/%d/%d/%d_%s.png", base, z, x, y, dem);
}

/* dem10b (z=14) がキャッシュ済みか確認する */
static int tile_dem10b_is_cached(const char *base, int x14, int y14)
{
    char path[512];
    struct stat st;
    make_tile_path(path, sizeof(path), base, 14, x14, y14, "b");
    return stat(path, &st) == 0;
}

/*
 * タイルを読み込んでElevTileの指定領域にコピーする
 * dem5 a/b/c をピクセル単位で優先順に試し、NODATA は ELEV_NODATA のまま残す
 */
static void load_tile_into(ElevTile *dst,
                            int dst_x, int dst_y,
                            const char *base,
                            int tx, int ty,
                            int src_x0, int src_y0,
                            int src_x1, int src_y1)
{
    char path[512];
    uint32_t w[3] = {0}, h[3] = {0}, ch[3] = {0};
    uint8_t *rgb[3] = {NULL, NULL, NULL};

    const char *dems[] = {"a", "b", "c"};
    for (int d = 0; d < 3; d++) {
        make_tile_path(path, sizeof(path), base, 15, tx, ty, dems[d]);
        rgb[d] = load_png_rgb(path, &w[d], &h[d], &ch[d]);
    }

    for (int sy = src_y0; sy < src_y1; sy++) {
        for (int sx = src_x0; sx < src_x1; sx++) {
            float elev = ELEV_NODATA;
            for (int d = 0; d < 3 && elev == ELEV_NODATA; d++) {
                if (rgb[d]) {
                    const uint8_t *px =
                        rgb[d] + sy * w[d] * ch[d] + sx * ch[d];
                    float e = rgb2elev(px[0], px[1], px[2]);
                    if (e != ELEV_NODATA) elev = e;
                }
            }
            int dx = dst_x + (sx - src_x0);
            int dy = dst_y + (sy - src_y0);
            dst->data[dy * dst->width + dx] = elev;  /* NODATA は -9999 のまま */
        }
    }

    for (int d = 0; d < 3; d++) free(rgb[d]);
}

ElevTile *elev_load_png(const char *path)
{
    uint32_t w, h, ch;
    uint8_t *rgb = load_png_rgb(path, &w, &h, &ch);
    if (!rgb) return NULL;

    ElevTile *tile = malloc(sizeof(ElevTile));
    tile->width    = w;
    tile->height   = h;
    tile->data     = malloc(sizeof(float) * w * h);
    if (!tile->data) { free(rgb); free(tile); return NULL; }

    for (uint32_t y = 0; y < h; y++) {
        for (uint32_t x = 0; x < w; x++) {
            const uint8_t *px = rgb + y * w * ch + x * ch;
            tile->data[y * w + x] = rgb2elev(px[0], px[1], px[2]);
        }
    }
    free(rgb);
    return tile;
}

/*
 * 8方向オーバーラップで読み込む (258×258)
 * NODATA ピクセルは ELEV_NODATA (-9999) のまま返す
 */
ElevTile *elev_load_with_overlap_8dir(const char *base, TileCoord tc)
{
    uint32_t W = TILE_PIX + 2;  /* 258 */
    uint32_t H = TILE_PIX + 2;  /* 258 */

    ElevTile *tile = malloc(sizeof(ElevTile));
    if (!tile) return NULL;
    tile->width  = W;
    tile->height = H;
    /* NODATA 初期値: 後で dem10 補完または SEA 変換する */
    tile->data   = malloc(W * H * sizeof(float));
    if (!tile->data) { free(tile); return NULL; }
    for (uint32_t i = 0; i < W * H; i++)
        tile->data[i] = ELEV_NODATA;

    int off = 1;  /* メインタイルを (1,1) に配置 */

    /* 1. メインタイル (256×256) */
    load_tile_into(tile, off, off, base, tc.x, tc.y,
                   0, 0, TILE_PIX, TILE_PIX);

    /* 2. 上 (y-1) - 下端1行 */
    load_tile_into(tile, off, 0, base, tc.x, tc.y-1,
                   0, TILE_PIX-1, TILE_PIX, TILE_PIX);

    /* 3. 下 (y+1) - 上端1行 */
    load_tile_into(tile, off, TILE_PIX + off, base, tc.x, tc.y+1,
                   0, 0, TILE_PIX, 1);

    /* 4. 左 (x-1) - 右端1列 */
    load_tile_into(tile, 0, off, base, tc.x-1, tc.y,
                   TILE_PIX-1, 0, TILE_PIX, TILE_PIX);

    /* 5. 右 (x+1) - 左端1列 */
    load_tile_into(tile, TILE_PIX + off, off, base, tc.x+1, tc.y,
                   0, 0, 1, TILE_PIX);

    /* 6. 左上 */
    load_tile_into(tile, 0, 0, base, tc.x-1, tc.y-1,
                   TILE_PIX-1, TILE_PIX-1, TILE_PIX, TILE_PIX);

    /* 7. 右上 */
    load_tile_into(tile, TILE_PIX + off, 0, base, tc.x+1, tc.y-1,
                   0, TILE_PIX-1, 1, TILE_PIX);

    /* 8. 左下 */
    load_tile_into(tile, 0, TILE_PIX + off, base, tc.x-1, tc.y+1,
                   TILE_PIX-1, 0, TILE_PIX, 1);

    /* 9. 右下 */
    load_tile_into(tile, TILE_PIX + off, TILE_PIX + off,
                   base, tc.x+1, tc.y+1, 0, 0, 1, 1);

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

/*
 * 8方向オーバーラップ + dem10b補完付きタイル読み込み
 *
 * 処理:
 *   1. dem5 (a/b/c pixel-level fallback) でロード
 *   2. メイン256×256 内の NODATA ピクセルを dem10b で補完
 *   3. 残る NODATA と負値を SEA (0) に変換して返す
 */
ElevTile *elev_load_with_overlap_8dir_with_dem10(const char *base, TileCoord tc)
{
    ElevTile *tile = elev_load_with_overlap_8dir(base, tc);
    if (!tile) return NULL;

    /* メイン256×256内の NODATA を数える (offset +1) */
    int nodata_cnt = 0;
    for (int py = 1; py <= TILE_PIX; py++)
        for (int px = 1; px <= TILE_PIX; px++)
            if (tile->data[py * tile->width + px] == ELEV_NODATA)
                nodata_cnt++;

    if (nodata_cnt > 0) {
        /* dem10b タイル: z15→z14 座標変換 */
        int z14_x = tc.x / 2;
        int z14_y = tc.y / 2;
        int ox    = (tc.x % 2) * TILE_PIX;  /* z14タイル内オフセット */
        int oy    = (tc.y % 2) * TILE_PIX;

        if (!tile_dem10b_is_cached(base, z14_x, z14_y))
            fprintf(stderr, "警告: dem10b タイル未キャッシュ (%d/%d)\n", z14_x, z14_y);

        char dem10_path[512];
        uint32_t dw, dh, dch;
        make_tile_path(dem10_path, sizeof(dem10_path), base, 14, z14_x, z14_y, "b");
        uint8_t *dem10_rgb = load_png_rgb(dem10_path, &dw, &dh, &dch);

        if (dem10_rgb) {
            for (int py = 1; py <= TILE_PIX; py++) {
                for (int px = 1; px <= TILE_PIX; px++) {
                    int idx = py * tile->width + px;
                    if (tile->data[idx] != ELEV_NODATA) continue;

                    int dpx = ox + (px - 1);
                    int dpy = oy + (py - 1);
                    if (dpx < (int)dw && dpy < (int)dh) {
                        const uint8_t *p =
                            dem10_rgb + dpy * dw * dch + dpx * dch;
                        float e = rgb2elev(p[0], p[1], p[2]);
                        if (e != ELEV_NODATA) tile->data[idx] = e;
                    }
                }
            }
            free(dem10_rgb);
        }
    }

    /* NODATA と負値を SEA に変換 */
    for (uint32_t i = 0; i < tile->width * tile->height; i++) {
        if (tile->data[i] == ELEV_NODATA || tile->data[i] < ELEV_SEA)
            tile->data[i] = ELEV_SEA;
    }

    return tile;
}

void elev_load_tile_into_big(ElevTile *big, int dst_x, int dst_y,
                              const char *base, int tx, int ty)
{
    load_tile_into(big, dst_x, dst_y, base, tx, ty, 0, 0, TILE_PIX, TILE_PIX);
}

/*
 * ビッグタイル内のNODATAピクセルをdem10bで補完し、最後にSEA変換
 *
 * ビッグタイルレイアウト:
 *   px=0, py=0      : 外周ボーダー
 *   px=1, py=1 〜   : z15タイル (range_x_min, range_y_min) から
 *
 * z14タイル単位でまとめてfetch・補完することで重複ダウンロードを防ぐ。
 */
void elev_fill_nodata_dem10b(ElevTile *big, const char *base,
                              int range_x_min, int range_y_min)
{
    int tile_w = ((int)big->width  - 2) / TILE_PIX;
    int tile_h = ((int)big->height - 2) / TILE_PIX;

    int x14_min = range_x_min / 2;
    int x14_max = (range_x_min + tile_w - 1) / 2;
    int y14_min = range_y_min / 2;
    int y14_max = (range_y_min + tile_h - 1) / 2;

    int missing_dem10b = 0;

    for (int y14 = y14_min; y14 <= y14_max; y14++) {
        for (int x14 = x14_min; x14 <= x14_max; x14++) {
            /* このz14タイルが担当するz15範囲(ビッグタイル内) */
            int x15_lo = (x14 * 2 >= range_x_min) ? x14 * 2 : range_x_min;
            int x15_hi = (x14 * 2 + 1 <= range_x_min + tile_w - 1)
                         ? x14 * 2 + 1 : range_x_min + tile_w - 1;
            int y15_lo = (y14 * 2 >= range_y_min) ? y14 * 2 : range_y_min;
            int y15_hi = (y14 * 2 + 1 <= range_y_min + tile_h - 1)
                         ? y14 * 2 + 1 : range_y_min + tile_h - 1;

            int px_lo = (x15_lo - range_x_min) * TILE_PIX + 1;
            int px_hi = (x15_hi - range_x_min + 1) * TILE_PIX;
            int py_lo = (y15_lo - range_y_min) * TILE_PIX + 1;
            int py_hi = (y15_hi - range_y_min + 1) * TILE_PIX;

            /* NODATAが存在するか確認 */
            int has_nodata = 0;
            for (int py = py_lo; py <= py_hi && !has_nodata; py++)
                for (int px = px_lo; px <= px_hi && !has_nodata; px++)
                    if (big->data[(size_t)py * big->width + px] == ELEV_NODATA)
                        has_nodata = 1;
            if (!has_nodata) continue;

            if (!tile_dem10b_is_cached(base, x14, y14))
                missing_dem10b++;

            char dem10_path[512];
            make_tile_path(dem10_path, sizeof(dem10_path), base, 14, x14, y14, "b");
            uint32_t dw, dh, dch;
            uint8_t *dem10_rgb = load_png_rgb(dem10_path, &dw, &dh, &dch);
            if (!dem10_rgb) continue;

            for (int py = py_lo; py <= py_hi; py++) {
                for (int px = px_lo; px <= px_hi; px++) {
                    size_t idx = (size_t)py * big->width + px;
                    if (big->data[idx] != ELEV_NODATA) continue;

                    int z15_tx = range_x_min + (px - 1) / TILE_PIX;
                    int z15_ty = range_y_min + (py - 1) / TILE_PIX;
                    int dpx = (z15_tx % 2) * TILE_PIX + (px - 1) % TILE_PIX;
                    int dpy = (z15_ty % 2) * TILE_PIX + (py - 1) % TILE_PIX;
                    if (dpx < (int)dw && dpy < (int)dh) {
                        const uint8_t *p =
                            dem10_rgb + dpy * dw * dch + dpx * dch;
                        float e = rgb2elev(p[0], p[1], p[2]);
                        if (e != ELEV_NODATA) big->data[idx] = e;
                    }
                }
            }
            free(dem10_rgb);
        }
    }

    if (missing_dem10b > 0)
        fprintf(stderr, "警告: dem10b タイル未キャッシュ %d 件 - prefetch_tiles.py を先に実行してください\n",
                missing_dem10b);

    /* NODATA（ボーダー含む）と負値をSEAに変換 */
    for (uint32_t i = 0; i < big->width * big->height; i++) {
        if (big->data[i] == ELEV_NODATA || big->data[i] < ELEV_SEA)
            big->data[i] = ELEV_SEA;
    }
}

/* 後方互換: 4方向オーバーラップ版 (257×257) */
ElevTile *elev_load_with_overlap(const char *base, TileCoord tc)
{
    uint32_t W = TILE_PIX + 1;
    uint32_t H = TILE_PIX + 1;

    ElevTile *tile = malloc(sizeof(ElevTile));
    tile->width    = W;
    tile->height   = H;
    tile->data     = calloc(W * H, sizeof(float));
    if (!tile->data) { free(tile); return NULL; }

    load_tile_into(tile, 0, 0, base, tc.x, tc.y, 0, 0, TILE_PIX, TILE_PIX);
    load_tile_into(tile, TILE_PIX, 0, base, tc.x+1, tc.y, 0, 0, 1, TILE_PIX);
    load_tile_into(tile, 0, TILE_PIX, base, tc.x, tc.y+1, 0, 0, TILE_PIX, 1);
    load_tile_into(tile, TILE_PIX, TILE_PIX, base, tc.x+1, tc.y+1, 0, 0, 1, 1);

    /* NODATA を SEA に変換 */
    for (uint32_t i = 0; i < W * H; i++)
        if (tile->data[i] == ELEV_NODATA || tile->data[i] < ELEV_SEA)
            tile->data[i] = ELEV_SEA;

    return tile;
}
