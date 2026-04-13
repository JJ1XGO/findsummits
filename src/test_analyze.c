/*
 * test_analyze.c - analyze.cの動作確認
 *
 * SOTA日本支部 未登録サミット候補探索ツール
 * Copyright (C) 2026 JJ1XGO
 * GPL-3.0
 */
#include <stdio.h>
#include <stdlib.h>
#include "analyze.h"

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "使い方: %s <PNGファイルパス>\n", argv[0]);
        return 1;
    }

    float min_prominence = 150.0f;
    if (argc >= 3)
        min_prominence = atof(argv[2]);

    printf("解析中: %s (最小プロミネンス: %.1fm)\n",
           argv[1], min_prominence);

    AnalyzeResult *result = analyze_tile(argv[1], min_prominence);
    if (!result) {
        fprintf(stderr, "解析失敗\n");
        return 1;
    }

    printf("検出ピーク数: %d\n\n", result->peak_cnt);
    printf("%-12s %-8s %-12s %-8s %-12s %s\n",
           "ピーク座標","標高","コル座標","コル標高","プロミネンス","備考");

    for (int i = 0; i < result->peak_cnt; i++) {
        PeakResult *p = &result->peaks[i];
        if (p->is_tile_top) {
            printf("  (%4d,%4d) %7.2fm  %-24s  %7.2fm  タイル最高峰\n",
                   p->peak_x, p->peak_y, p->peak_elev,
                   "----", p->prominence);
        } else {
            printf("  (%4d,%4d) %7.2fm  (%4d,%4d) %7.2fm  %7.2fm\n",
                   p->peak_x, p->peak_y, p->peak_elev,
                   p->col_x, p->col_y, p->col_elev,
                   p->prominence);
        }
    }

    analyze_result_destroy(result);
    printf("\n解析完了\n");
    return 0;
}
