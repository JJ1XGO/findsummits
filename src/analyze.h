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

/*
 * ピーク解析結果
 */
typedef struct {
    int32_t peak_x, peak_y;     /* ピーク座標(タイル内ピクセル) */
    float   peak_elev;          /* ピーク標高(m) */
    int32_t col_x, col_y;       /* コル座標(タイル内ピクセル) */
    float   col_elev;           /* コル標高(m) */
    float   prominence;         /* プロミネンス(m) */
    int     is_tile_top;        /* タイル内最高峰フラグ */
} PeakResult;

/*
 * タイル解析結果
 */
typedef struct {
    PeakResult *peaks;          /* ピーク結果リスト */
    int         peak_cnt;       /* ピーク数 */
} AnalyzeResult;

/* 関数プロトタイプ */
AnalyzeResult *analyze_tile(const char *png_path, float min_prominence);
void           analyze_result_destroy(AnalyzeResult *result);

#endif /* ANALYZE_H */
