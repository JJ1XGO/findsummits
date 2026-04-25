/*
 * main.c - SOTA日本支部 未登録サミット候補探索ツール 本番エントリポイント
 *
 * Copyright (C) 2026 JJ1XGO
 * GPL-3.0
 */
#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mesh_analyze.h"

static void load_dotenv(const char *path)
{
    FILE *f = fopen(path, "r");
    if (!f) return;
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        char *p = line;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '#' || *p == '\n' || *p == '\r' || *p == '\0') continue;
        char *eq = strchr(p, '=');
        if (!eq) continue;
        char key[256];
        int klen = (int)(eq - p);
        if (klen <= 0 || klen >= (int)sizeof(key)) continue;
        memcpy(key, p, klen);
        key[klen] = '\0';
        for (int i = klen - 1; i >= 0 && (key[i] == ' ' || key[i] == '\t'); i--) key[i] = '\0';
        char val[256];
        char *vstart = eq + 1;
        int vlen = (int)strlen(vstart);
        if (vlen >= (int)sizeof(val)) vlen = (int)sizeof(val) - 1;
        memcpy(val, vstart, vlen);
        val[vlen] = '\0';
        char *comment = strchr(val, '#');
        if (comment) *comment = '\0';
        for (int i = (int)strlen(val) - 1;
             i >= 0 && (val[i] == ' ' || val[i] == '\t' || val[i] == '\n' || val[i] == '\r');
             i--) val[i] = '\0';
        setenv(key, val, 0); /* 既存の環境変数は上書きしない */
    }
    fclose(f);
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
            "  %s params/mesh_list_japan.txt\n",
            argv[0], argv[0], argv[0], argv[0]);
        return 1;
    }

    load_dotenv(".env");
    const char *data_dir = getenv("DATA_DIR");
    if (!data_dir || data_dir[0] == '\0') data_dir = "/data";

    char tile_dir[512], result_dir[512], img_dir[512], log_dir[512];
    snprintf(tile_dir,   sizeof(tile_dir),   "%s/tiles",       data_dir);
    snprintf(result_dir, sizeof(result_dir), "%s/results/csv", data_dir);
    snprintf(img_dir,    sizeof(img_dir),    "%s/images",      data_dir);
    snprintf(log_dir,    sizeof(log_dir),    "%s/logs",        data_dir);

    printf("=== SOTA未登録サミット候補探索ツール ===\n");

    /* 数字のみならメッシュコード、それ以外はファイルパス */
    const char *arg = argv[1];
    int is_meshcode = 1;
    for (int i = 0; arg[i] != '\0'; i++) {
        if (arg[i] < '0' || arg[i] > '9') { is_meshcode = 0; break; }
    }

    MeshAnalyzeConfig cfg = {
        .tile_dir       = tile_dir,
        .result_dir     = result_dir,
        .img_dir        = img_dir,
        .log_dir        = log_dir,
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
