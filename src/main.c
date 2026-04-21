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

static const MeshAnalyzeConfig DEFAULT_CFG = {
    .tile_dir       = "/mnt/findsummits/tiles",
    .result_dir     = "/mnt/findsummits/results/csv",
    .min_prominence = 150.0f,
};

/*
 * メッシュリストファイルから1次メッシュコードを読み込んで順次解析する
 */
static int run_from_file(const MeshAnalyzeConfig *cfg, const char *list_path)
{
    FILE *fp = fopen(list_path, "r");
    if (!fp) {
        fprintf(stderr, "リストファイルを開けません: %s\n", list_path);
        return 1;
    }

    char line[64];
    int total = 0, ok = 0, ng = 0;
    while (fgets(line, sizeof(line), fp)) {
        char *p = line;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '#' || *p == '\n' || *p == '\r' || *p == '\0')
            continue;

        int meshcode = atoi(p);
        if (meshcode < 1000 || meshcode > 9999) {
            fprintf(stderr, "無効なメッシュコード: %s", line);
            continue;
        }

        total++;
        printf("\n=== [%d] メッシュ%d ===\n", total, meshcode);
        int ret = mesh_analyze(cfg, meshcode);
        if (ret == 0) ok++; else ng++;
    }

    fclose(fp);
    printf("\n=== 完了: %d件処理 (成功:%d 失敗:%d) ===\n", total, ok, ng);
    return ng > 0 ? 1 : 0;
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr,
            "使い方:\n"
            "  %s <1次メッシュコード>    単一メッシュ解析\n"
            "  %s <リストファイル>       ファイルから一括解析\n"
            "例:\n"
            "  %s 4929\n"
            "  %s tasks/mesh_list_japan.txt\n",
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

    if (is_meshcode) {
        int meshcode = atoi(arg);
        printf("対象メッシュ: %d\n\n", meshcode);
        int ret = mesh_analyze(&DEFAULT_CFG, meshcode);
        if (ret == 0)
            printf("\n解析完了！ 結果は %s/%d.csv に保存されました。\n",
                   DEFAULT_CFG.result_dir, meshcode);
        else
            fprintf(stderr, "解析に失敗しました。\n");
        return ret;
    } else {
        printf("メッシュリスト: %s\n\n", arg);
        return run_from_file(&DEFAULT_CFG, arg);
    }
}
