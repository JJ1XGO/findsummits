/*
 * test_terrain_image.c - 標高地形図PNG単体出力ツール（ピーク/コル解析なし）
 *
 * 富士山を含む1次メッシュコード5338を中心とした3×3メッシュの
 * 標高地形図サンプルイメージ作成用。mesh_analyze() は地形図出力の後に
 * 必ず Union-Find 解析 + CSV 出力まで行うため、解析を伴わず地形図のみ
 * 欲しい場合に本ツールを使う。
 */
#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mesh_analyze.h"
#include "mesh.h"
#include "elevation.h"

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
        while (*vstart == ' ' || *vstart == '\t') vstart++;
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
    int meshcodes[9] = {5237, 5238, 5239, 5337, 5338, 5339, 5437, 5438, 5439};
    int center_meshcode = 5338;
    if (argc >= 2) center_meshcode = atoi(argv[1]);

    load_dotenv("params/config.ini");
    const char *data_dir = getenv("DATA_DIR");
    if (!data_dir || data_dir[0] == '\0') data_dir = "/data";

    char tile_dir[512], img_path[512];
    snprintf(tile_dir, sizeof(tile_dir), "%s/tiles", data_dir);
    snprintf(img_path, sizeof(img_path), "%s/images/%d_terrain.png", data_dir, center_meshcode);

    printf("=== 標高地形図PNG単体出力ツール（解析なし） ===\n");
    printf("対象9メッシュ: ");
    for (int i = 0; i < 9; i++) printf("%d ", meshcodes[i]);
    printf("\n出力先: %s\n\n", img_path);

    MeshTileRange combined;
    if (mesh_to_tile_range(meshcodes[0], 15, &combined) != 0) {
        fprintf(stderr, "メッシュ範囲計算失敗: %d\n", meshcodes[0]);
        return 1;
    }
    for (int i = 1; i < 9; i++) {
        MeshTileRange r;
        if (mesh_to_tile_range(meshcodes[i], 15, &r) != 0) {
            fprintf(stderr, "メッシュ範囲計算失敗: %d\n", meshcodes[i]);
            return 1;
        }
        if (r.x_min < combined.x_min) combined.x_min = r.x_min;
        if (r.x_max > combined.x_max) combined.x_max = r.x_max;
        if (r.y_min < combined.y_min) combined.y_min = r.y_min;
        if (r.y_max > combined.y_max) combined.y_max = r.y_max;
    }
    combined.tile_w = combined.x_max - combined.x_min + 1;
    combined.tile_h = combined.y_max - combined.y_min + 1;

    printf("結合範囲: x=%d〜%d y=%d〜%d (%d×%d=%d枚)\n",
           combined.x_min, combined.x_max,
           combined.y_min, combined.y_max,
           combined.tile_w, combined.tile_h,
           combined.tile_w * combined.tile_h);

    ElevTile *big = load_mesh_tile(tile_dir, &combined, NULL);
    if (!big) {
        fprintf(stderr, "イメージ作成失敗\n");
        return 1;
    }

    save_terrain_rgb_image(big, img_path, NULL);
    elev_destroy(big);

    printf("\n完了\n");
    return 0;
}
