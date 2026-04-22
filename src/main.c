/*
 * main.c - SOTA日本支部 未登録サミット候補探索ツール 本番エントリポイント
 *
 * Copyright (C) 2026 JJ1XGO
 * GPL-3.0
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mesh_analyze.h"

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr,
            "使い方:\n"
            "  %s <1次メッシュコード>    単一メッシュ解析\n"
            "  %s <リストファイル>       ファイルから一括解析\n"
            "例:\n"
            "  %s 4929\n"
            "  %s params/mesh_list_japan.txt\n",
            argv[0], argv[0], argv[0], argv[0]);
        return 1;
    }

    printf("=== SOTA未登録サミット候補探索ツール ===\n");

    /* 数字のみならメッシュコード、それ以外はファイルパス */
    const char *arg = argv[1];
    int is_meshcode = 1;
    for (int i = 0; arg[i] != '\0'; i++) {
        if (arg[i] < '0' || arg[i] > '9') { is_meshcode = 0; break; }
    }

    MeshAnalyzeConfig cfg = {
        .tile_dir       = "/mnt/findsummits/tiles",
        .result_dir     = "/mnt/findsummits/results/csv",
        .min_prominence = 130.0f,
        .mesh_set       = NULL,
    };

    if (is_meshcode) {
        int meshcode = atoi(arg);
        printf("対象メッシュ: %d\n\n", meshcode);

        /* 単一メッシュ: 1要素の MeshSet（隣接なし→center のみ解析） */
        MeshSet single_set;
        single_set.codes = &meshcode;
        single_set.count = 1;
        cfg.mesh_set = &single_set;

        int ret = mesh_analyze(&cfg, meshcode);
        if (ret == 0)
            printf("\n解析完了！ 結果は %s/%d.csv に保存されました。\n",
                   cfg.result_dir, meshcode);
        else
            fprintf(stderr, "解析に失敗しました。\n");
        return ret;

    } else {
        printf("メッシュリスト: %s\n\n", arg);

        MeshSet set;
        if (mesh_set_load(&set, arg) < 0) {
            fprintf(stderr, "リストファイルを読み込めません: %s\n", arg);
            return 1;
        }
        cfg.mesh_set = &set;

        int total = 0, ok = 0, ng = 0;
        for (int i = 0; i < set.count; i++) {
            int meshcode = set.codes[i];
            total++;
            printf("\n=== [%d/%d] メッシュ%d ===\n", total, set.count, meshcode);
            int ret = mesh_analyze(&cfg, meshcode);
            if (ret == 0) ok++; else ng++;
        }

        mesh_set_destroy(&set);
        printf("\n=== 完了: %d件処理 (成功:%d 失敗:%d) ===\n", total, ok, ng);
        return ng > 0 ? 1 : 0;
    }
}
