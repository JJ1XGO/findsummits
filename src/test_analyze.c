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
#include "elevation.h"

static void print_results(AnalyzeResult *result)
{
    printf("検出ピーク数: %d\n\n", result->peak_cnt);
    printf("  %-12s %-8s %-12s %-8s %-12s %s\n",
           "ピーク座標","標高","コル座標","コル標高","プロミネンス","備考");
    for (int i = 0; i < result->peak_cnt; i++) {
        PeakResult *p = &result->peaks[i];
        if (p->is_tile_top) {
            printf("  (%4d,%4d) %7.2fm  %-24s  %7.2fm  要マージ\n",
                   p->peak_x, p->peak_y, p->peak_elev,
                   "----", p->prominence);
        } else {
            printf("  (%4d,%4d) %7.2fm  (%4d,%4d) %7.2fm  %7.2fm\n",
                   p->peak_x, p->peak_y, p->peak_elev,
                   p->col_x, p->col_y, p->col_elev,
                   p->prominence);
        }
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "使い方:\n");
        fprintf(stderr, "  通常モード:       %s <PNGパス> [最小プロミネンス]\n", argv[0]);
        fprintf(stderr, "  オーバーラップ:   %s <タイルdir> <z> <x> <y> [最小プロミネンス]\n", argv[0]);
        return 1;
    }

    float min_prom = 150.0f;

    if (argc >= 5) {
        /* オーバーラップモード */
        const char *tile_dir = argv[1];
        TileCoord tc = { atoi(argv[2]), atoi(argv[3]), atoi(argv[4]) };
        if (argc >= 6) min_prom = atof(argv[5]);

        printf("オーバーラップ解析: %s z=%d x=%d y=%d (最小プロミネンス: %.1fm)\n\n",
               tile_dir, tc.z, tc.x, tc.y, min_prom);

        AnalyzeResult *result = analyze_tile_overlap(tile_dir, tc, min_prom);
        if (!result) { fprintf(stderr, "解析失敗\n"); return 1; }
        print_results(result);
        analyze_result_destroy(result);

    } else {
        /* 通常モード */
        if (argc >= 3) min_prom = atof(argv[2]);

        printf("解析中: %s (最小プロミネンス: %.1fm)\n\n", argv[1], min_prom);

        AnalyzeResult *result = analyze_tile(argv[1], min_prom);
        if (!result) { fprintf(stderr, "解析失敗\n"); return 1; }
        print_results(result);
        analyze_result_destroy(result);
    }

    printf("\n解析完了\n");
    return 0;
}
