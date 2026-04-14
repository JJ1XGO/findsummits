/*
 * test_mesh_analyze.c - mesh_analyze.cの動作確認
 *
 * SOTA日本支部 未登録サミット候補探索ツール
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
        return 1;
    }

    int meshcode = atoi(argv[1]);

    MeshAnalyzeConfig cfg = {
        .tile_dir      = "/mnt/findsummits/tiles/15",
        .result_dir    = "/mnt/findsummits/results",
        .min_prominence = 150.0f,
    };

    return mesh_analyze(&cfg, meshcode);
}
