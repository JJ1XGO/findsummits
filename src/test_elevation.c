/*
 * test_elevation.c - elevation.cの動作確認
 *
 * SOTA日本支部 未登録サミット候補探索ツール
 * Copyright (C) 2026 JJ1XGO
 * GPL-3.0
 */
#include <stdio.h>
#include "elevation.h"

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "使い方: %s <PNGファイルパス>\n", argv[0]);
        fprintf(stderr, "例: %s /mnt/findsummits/tiles/15/29180/12955.png\n",
                argv[0]);
        return 1;
    }

    printf("読み込み中: %s\n", argv[1]);
    ElevTile *tile = elev_load_png(argv[1]);
    if (!tile) {
        fprintf(stderr, "読み込み失敗\n");
        return 1;
    }

    printf("サイズ: %u x %u\n", tile->width, tile->height);

    /* 統計情報 */
    float min_elev =  99999.0f;
    float max_elev = -99999.0f;

    int   sea_cnt    = 0;

    for (uint32_t y = 0; y < tile->height; y++) {
        for (uint32_t x = 0; x < tile->width; x++) {
            float e = elev_get(tile, x, y);
            if (e == 0.0f) { sea_cnt++; continue; }
            if (e < min_elev) min_elev = e;
            if (e > max_elev) max_elev = e;
        }
    }

    printf("最高標高: %.2fm\n", max_elev);
    printf("最低標高: %.2fm\n", min_elev);
    printf("海面/無効: %d ピクセル\n", sea_cnt);
    printf("有効ピクセル: %d\n",
           (int)(tile->width * tile->height) - sea_cnt);

    elev_destroy(tile);
    printf("テスト完了\n");
    return 0;
}
