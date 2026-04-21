/*
 * test_fetch.c - fetch.cの動作確認
 *
 * SOTA日本支部 未登録サミット候補探索ツール
 * Copyright (C) 2026 JJ1XGO
 * GPL-3.0
 */
#include <stdio.h>
#include <stdlib.h>
#include "fetch.h"
#include "mesh.h"

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "使い方: %s <1次メッシュコード>\n", argv[0]);
        fprintf(stderr, "例: %s 4929  (福岡周辺)\n", argv[0]);
        return 1;
    }

    int meshcode = atoi(argv[1]);

    FetchConfig cfg = {
        .tile_dir    = "/mnt/findsummits/tiles/15",
        .max_parallel = 8,       /* 今は1並列 */
        .interval_ms  = 100,     /* 100ms間隔 */
    };

    MeshTileRange range;
    if (mesh_to_tile_range(meshcode, 15, &range) != 0)
        return 1;

    printf("タイル範囲: x=%d〜%d y=%d〜%d (%d×%d=%d枚)\n",
           range.x_min, range.x_max,
           range.y_min, range.y_max,
           range.tile_w, range.tile_h,
           range.tile_w * range.tile_h);

    int ret = fetch_mesh(&cfg, &range);
    printf("完了: %d枚取得\n", ret);
    return 0;
}
