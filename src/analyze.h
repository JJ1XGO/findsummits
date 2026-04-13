/*
 * analyze.h - 標高タイル解析
 *
 * SOTA日本支部 未登録サミット候補探索ツール
 * Copyright (C) 2026 JJ1XGO
 * GPL-3.0
 */
#ifndef ANALYZE_H
#define ANALYZE_H

#include <stdint.h>
#include "elevation.h"

typedef struct {
    int32_t peak_x, peak_y;
    float   peak_elev;
    int32_t col_x, col_y;
    float   col_elev;
    float   prominence;
    int     is_tile_top;    /* タイル最高峰フラグ(境界またぎ未解決) */
} PeakResult;

typedef struct {
    PeakResult *peaks;
    int         peak_cnt;
} AnalyzeResult;

AnalyzeResult *analyze_tile(const char *png_path, float min_prominence);
AnalyzeResult *analyze_tile_overlap(const char *tile_dir,
                                     TileCoord tc,
                                     float min_prominence);
AnalyzeResult *analyze_tile_data(const ElevTile *tile,
                                  uint32_t main_w, uint32_t main_h,
                                  float min_prominence);
void           analyze_result_destroy(AnalyzeResult *result);

#endif /* ANALYZE_H */
