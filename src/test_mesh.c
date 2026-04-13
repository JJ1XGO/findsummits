/*
 * test_mesh.c - mesh.cの動作確認
 *
 * SOTA日本支部 未登録サミット候補探索ツール
 * Copyright (C) 2026 JJ1XGO
 * GPL-3.0
 */
#include <stdio.h>
#include <stdlib.h>
#include "mesh.h"

int main(int argc, char *argv[])
{
    /* テストするメッシュコード */
    int meshcodes[] = {
        5339,   /* 東京 */
        5338,   /* 大阪 */
        4929,   /* 福岡(九州北部) */
        3036,   /* 鹿児島(九州南部) */
        6441,   /* 札幌(北海道) */
    };
    int n = sizeof(meshcodes) / sizeof(meshcodes[0]);

    if (argc >= 2)  {
        /* 引数があれば指定メッシュコードでテスト */
        meshcodes[0] = atoi(argv[1]);
        n = 1;
    }

    for (int i = 0; i < n; i++) {
        MeshTileRange range;
        if (mesh_to_tile_range(meshcodes[i], 15, &range) != 0)
            continue;

        printf("  タイル範囲: x=%d〜%d y=%d〜%d\n",
               range.x_min, range.x_max,
               range.y_min, range.y_max);
        printf("  タイル数: %d × %d = %d枚\n",
               range.tile_w, range.tile_h,
               range.tile_w * range.tile_h);

        /* メモリ試算 */
        long pixels = (long)range.tile_w * 256 *
                      (long)range.tile_h * 256;
        printf("  ピクセル数: %ld\n", pixels);
        printf("  メモリ(float): %.1f MB\n",
               pixels * 4.0 / 1024 / 1024);
        printf("\n");
    }
    return 0;
}
