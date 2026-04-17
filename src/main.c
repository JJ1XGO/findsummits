/*
 * main.c - SOTA日本支部 未登録サミット候補探索ツール 本番エントリポイント
 *
 * Copyright (C) 2026 JJ1XGO
 * GPL-3.0
 */
#include <stdio.h>
#include <stdlib.h>
#include "mesh_analyze.h"

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "使い方: %s <1次メッシュコード>\n", argv[0]);
        fprintf(stderr, "例: %s 4929\n", argv[0]);
        fprintf(stderr, "    %s 5339   (東京周辺)\n", argv[0]);
        return 1;
    }

    int meshcode = atoi(argv[1]);

    MeshAnalyzeConfig cfg = {
        .tile_dir      = "tiles/15",
        .result_dir    = "results",
        .min_prominence = 150.0f,
    };

    printf("=== SOTA未登録サミット候補探索ツール ===\n");
    printf("対象メッシュ: %d\n\n", meshcode);

    int ret = mesh_analyze(&cfg, meshcode);

    if (ret == 0) {
        printf("\n解析完了！ 結果は results/%d.csv に保存されました。\n", meshcode);
    } else {
        fprintf(stderr, "解析に失敗しました。\n");
    }

    return ret;
}
