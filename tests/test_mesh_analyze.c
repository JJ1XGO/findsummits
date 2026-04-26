/*
 * test_mesh_analyze.c - mesh_analyze.cの動作確認
 */
#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "mesh_analyze.h"
#include "mesh.h"

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
        setenv(key, val, 0);
    }
    fclose(f);
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "使い方: %s <1次メッシュコード> [--save-image]\n", argv[0]);
        return 1;
    }

    int meshcode  = atoi(argv[1]);
    int save_image = (argc >= 3 && strcmp(argv[2], "--save-image") == 0);

    load_dotenv("params/config.ini");
    const char *data_dir = getenv("DATA_DIR");
    if (!data_dir || data_dir[0] == '\0') data_dir = "/data";

    char tile_dir[512], result_dir[512], img_dir[512];
    snprintf(tile_dir,   sizeof(tile_dir),   "%s/tiles",       data_dir);
    snprintf(result_dir, sizeof(result_dir), "%s/results/csv", data_dir);
    snprintf(img_dir,    sizeof(img_dir),    "%s/images",      data_dir);

    MeshAnalyzeConfig cfg = {
        .tile_dir       = tile_dir,
        .result_dir     = result_dir,
        .img_dir        = save_image ? img_dir : NULL,
        .min_prominence = 150.0f,
        .mesh_set       = NULL,
    };

    printf("=== SOTA未登録サミット候補探索ツール (テスト) ===\n");
    printf("対象メッシュ: %d\n\n", meshcode);

    struct timespec ts_start, ts_end;
    clock_gettime(CLOCK_MONOTONIC, &ts_start);

    int ret = mesh_analyze(&cfg, meshcode);

    clock_gettime(CLOCK_MONOTONIC, &ts_end);
    double elapsed = (ts_end.tv_sec - ts_start.tv_sec) +
                     (ts_end.tv_nsec - ts_start.tv_nsec) / 1e9;

    if (ret == 0)
        printf("\n解析完了 (%.1f秒)\n", elapsed);
    else
        fprintf(stderr, "解析失敗\n");

    return ret;
}
