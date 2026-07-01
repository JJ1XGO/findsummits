/*
 * terrain_colormap_demo.c - 標高地形図カラーマップ比較用試作ツール（本番パイプライン外）
 *
 * 陰影起伏(hillshade)を使わず、明度(Lightness)を標高に対して単調に
 * 変化させるカラーランプで標高差の視認性が上がるかを検証する。
 * 現行配色(save_terrain_rgb_image)と明度単調版を同一データから2枚出力し比較する。
 */
#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <png.h>
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

static void hsl_to_rgb(double h, double s, double l, uint8_t *r, uint8_t *g, uint8_t *b)
{
    double c = (1.0 - fabs(2.0 * l - 1.0)) * s;
    double hp = h / 60.0;
    double x = c * (1.0 - fabs(fmod(hp, 2.0) - 1.0));
    double r1, g1, b1;
    if (hp < 1.0)      { r1 = c; g1 = x; b1 = 0; }
    else if (hp < 2.0) { r1 = x; g1 = c; b1 = 0; }
    else if (hp < 3.0) { r1 = 0; g1 = c; b1 = x; }
    else if (hp < 4.0) { r1 = 0; g1 = x; b1 = c; }
    else if (hp < 5.0) { r1 = x; g1 = 0; b1 = c; }
    else               { r1 = c; g1 = 0; b1 = x; }
    double m = l - c / 2.0;
    *r = (uint8_t)((r1 + m) * 255.0 + 0.5);
    *g = (uint8_t)((g1 + m) * 255.0 + 0.5);
    *b = (uint8_t)((b1 + m) * 255.0 + 0.5);
}

/*
 * 明度単調版カラーマップ:
 * 色相は現行の低地(黄緑)→山地(緑)→亜高山(茶)の推移を踏襲しつつ、
 * 明度(L)を標高に対して単調減少させ、頂上域だけ白(雪冠風)に跳ね上げる。
 * 現行版の900〜2700m帯で明度がほぼ横ばい(148→113→118)になっている問題への対策。
 */
static void elev_to_rgb_monotone(float elev, uint8_t *r, uint8_t *g, uint8_t *b)
{
    static const double stops[] = {
        -6000.0, -1.0, 0.0, 300.0, 900.0, 1800.0, 2700.0, 3800.0,
    };
    /* {hue, saturation, lightness} */
    static const double hsl[][3] = {
        {200.0, 0.35, 0.30}, /* -6000m: 深い青(NODATA) */
        {200.0, 0.45, 0.57}, /*    -1m: 沿岸の青       */
        { 80.0, 0.55, 0.88}, /*     0m: 明るい黄緑     */
        { 95.0, 0.45, 0.68}, /*   300m: 台地           */
        {110.0, 0.35, 0.50}, /*   900m: 山地           */
        { 30.0, 0.55, 0.35}, /*  1800m: 亜高山         */
        { 20.0, 0.40, 0.22}, /*  2700m: 高山           */
        {  0.0, 0.00, 0.93}, /*  3800m: 頂上(雪冠風)   */
    };
    static const int n = 8;

    double e = (double)elev;

    if (e < -9000.0) {
        hsl_to_rgb(hsl[0][0], hsl[0][1], hsl[0][2], r, g, b);
        return;
    }
    if (e <= 0.0) e = -1.0;

    if (e >= stops[n - 1]) {
        hsl_to_rgb(hsl[n-1][0], hsl[n-1][1], hsl[n-1][2], r, g, b);
        return;
    }
    for (int i = 0; i < n - 1; i++) {
        if (e >= stops[i] && e < stops[i + 1]) {
            double t = (e - stops[i]) / (stops[i + 1] - stops[i]);
            double h = hsl[i][0] + t * (hsl[i+1][0] - hsl[i][0]);
            double s = hsl[i][1] + t * (hsl[i+1][1] - hsl[i][1]);
            double l = hsl[i][2] + t * (hsl[i+1][2] - hsl[i][2]);
            hsl_to_rgb(h, s, l, r, g, b);
            return;
        }
    }
}

/*
 * 参考パレット: commit 4b1e391（2026-04-25）で使われていた15段階グラデーション。
 * 5339_terrain.png はこのパレットで生成されたもの（コミットメッセージに
 * 「5440・5339 で動作確認済み」と明記）。RGB値をそのまま流用する。
 */
static void elev_to_rgb_ref15(float elev, uint8_t *r, uint8_t *g, uint8_t *b)
{
    static const double stops[] = {
        -6000.0, -2500.0, -1.0, 0.0, 100.0, 150.0, 300.0, 800.0,
        1000.0, 2500.0, 3000.0, 3500.0, 4000.0, 5000.0, 5500.0
    };
    static const int colors[][3] = {
        {  9,  56, 191},  /* -6000m: #0938BF 濃い青   */
        { 80, 217, 251},  /* -2500m: #50D9FB 明るい青 */
        {183, 229, 250},  /*    -1m: #B7E5FA 薄い青   */
        { 31,  72,   6},  /*     0m: #1F4806 濃い緑   */
        {104, 227, 107},  /*   100m: #68E36B 明るい緑 */
        {152, 214, 133},  /*   150m: #98D685 黄緑     */
        {249, 239, 205},  /*   300m: #F9EFCD 薄黄     */
        {224, 187, 125},  /*   800m: #E0BB7D 茶色     */
        {211, 166,  45},  /*  1000m: #D3A62D 濃い茶  */
        {153, 118,  24},  /*  2500m: #997618 暗い茶  */
        {112,  91,  16},  /*  3000m: #705B10 濃い茶  */
        { 95,  81,  13},  /*  3500m: #5F510D 濃褐色  */
        {165, 100,  83},  /*  4000m: #A56453 赤茶    */
        { 92,  29,   9},  /*  5000m: #5C1D09 黒茶    */
        {255, 250, 250},  /*  5500m: #FFFAFA 白/snow */
    };
    static const int n = 15;

    double e = (double)elev;

    if (e < -9000.0) {
        *r = colors[0][0]; *g = colors[0][1]; *b = colors[0][2]; return;
    }
    if (e <= 0.0) e = -1.0;

    if (e >= stops[n - 1]) {
        *r = colors[n-1][0]; *g = colors[n-1][1]; *b = colors[n-1][2]; return;
    }
    for (int i = 0; i < n - 1; i++) {
        if (e >= stops[i] && e < stops[i + 1]) {
            double t = (e - stops[i]) / (stops[i + 1] - stops[i]);
            *r = (uint8_t)(colors[i][0] + t * (colors[i+1][0] - colors[i][0]) + 0.5);
            *g = (uint8_t)(colors[i][1] + t * (colors[i+1][1] - colors[i][1]) + 0.5);
            *b = (uint8_t)(colors[i][2] + t * (colors[i+1][2] - colors[i][2]) + 0.5);
            return;
        }
    }
}

/*
 * 参考パレット(ref15)の緑グループ(0/100/150/300m)と茶色グループ
 * (800/1000/2500/3000m)を丸ごと交換した実験版。低地=茶色系、山=緑系になる。
 * 3500m以上(濃褐色・赤茶・黒茶・白)は元のまま。
 */
static void elev_to_rgb_ref15_swapped(float elev, uint8_t *r, uint8_t *g, uint8_t *b)
{
    static const double stops[] = {
        -6000.0, -2500.0, -1.0, 0.0, 100.0, 150.0, 300.0, 800.0,
        1000.0, 2500.0, 3000.0, 3500.0, 4000.0, 5000.0, 5500.0
    };
    static const int colors[][3] = {
        {  9,  56, 191},  /* -6000m: 濃い青(そのまま)   */
        { 80, 217, 251},  /* -2500m: 明るい青(そのまま) */
        {183, 229, 250},  /*    -1m: 薄い青(そのまま)   */
        {224, 187, 125},  /*     0m: 茶色(元800mの色)   */
        {211, 166,  45},  /*   100m: 濃い茶(元1000mの色) */
        {153, 118,  24},  /*   150m: 暗い茶(元2500mの色) */
        {112,  91,  16},  /*   300m: 濃い茶(元3000mの色) */
        { 31,  72,   6},  /*   800m: 濃い緑(元0mの色)   */
        {104, 227, 107},  /*  1000m: 明るい緑(元100mの色) */
        {152, 214, 133},  /*  2500m: 黄緑(元150mの色)   */
        {249, 239, 205},  /*  3000m: 薄黄(元300mの色)   */
        { 95,  81,  13},  /*  3500m: 濃褐色(そのまま)   */
        {165, 100,  83},  /*  4000m: 赤茶(そのまま)     */
        { 92,  29,   9},  /*  5000m: 黒茶(そのまま)     */
        {255, 250, 250},  /*  5500m: 白/snow(そのまま)  */
    };
    static const int n = 15;

    double e = (double)elev;

    if (e < -9000.0) {
        *r = colors[0][0]; *g = colors[0][1]; *b = colors[0][2]; return;
    }
    if (e <= 0.0) e = -1.0;

    if (e >= stops[n - 1]) {
        *r = colors[n-1][0]; *g = colors[n-1][1]; *b = colors[n-1][2]; return;
    }
    for (int i = 0; i < n - 1; i++) {
        if (e >= stops[i] && e < stops[i + 1]) {
            double t = (e - stops[i]) / (stops[i + 1] - stops[i]);
            *r = (uint8_t)(colors[i][0] + t * (colors[i+1][0] - colors[i][0]) + 0.5);
            *g = (uint8_t)(colors[i][1] + t * (colors[i+1][1] - colors[i][1]) + 0.5);
            *b = (uint8_t)(colors[i][2] + t * (colors[i+1][2] - colors[i][2]) + 0.5);
            return;
        }
    }
}

/*
 * 地物イメージ準拠版カラーマップ（v3・日本(富士山)の標高レンジに合わせ再調整）:
 * 海岸線=グレー、23区=濃いグレー、多摩(平野部)=明るい茶色、
 * 丘陵地帯を経て八王子〜青木ヶ原樹海=濃い緑、そこから森林限界手前まで
 * 徐々に薄くなる緑、森林限界付近=紅葉の赤、その上=グレー、薄い黄色、山頂=白。
 * v2からの変更点（ユーザーフィードバック反映）:
 * - 標高レンジは富士山(3776m)基準のまま維持（参考にした15段階パレットの
 *   世界規模レンジ5500mは日本には合わないため採用しない）
 * - 80m→900mの間に300m(丘陵地帯)を追加し、平野→山の遷移で黄色が
 *   広範囲に残っていた問題を解消（丘陵地帯で早めに緑寄りへ移行）
 * - 森林限界付近(2400m)を薄茶から紅葉の赤へ変更
 */
static void elev_to_rgb_landscape(float elev, uint8_t *r, uint8_t *g, uint8_t *b)
{
    static const double stops[] = {
        -6000.0, -1.0, 0.0, 20.0, 80.0, 300.0, 900.0, 2200.0, 2400.0, 2700.0, 3300.0, 3776.0,
    };
    /* {hue, saturation, lightness} */
    static const double hsl[][3] = {
        {200.0, 0.35, 0.30}, /* -6000m: 深い青(NODATA)     */
        {200.0, 0.45, 0.57}, /*    -1m: 沿岸の青           */
        {  0.0, 0.03, 0.72}, /*     0m: グレー(海岸線)     */
        {  0.0, 0.04, 0.58}, /*    20m: 濃いグレー(23区)   */
        { 32.0, 0.48, 0.60}, /*    80m: 明るい茶色(多摩・平野部) */
        { 75.0, 0.40, 0.48}, /*   300m: 丘陵地帯（緑への早期移行） */
        {120.0, 0.50, 0.25}, /*   900m: 濃い緑(八王子〜樹海) */
        {100.0, 0.35, 0.58}, /*  2200m: 薄くなった緑(森林限界手前) */
        {  8.0, 0.55, 0.48}, /*  2400m: 紅葉の赤(森林限界付近) */
        {  0.0, 0.05, 0.55}, /*  2700m: グレー             */
        { 50.0, 0.40, 0.70}, /*  3300m: 薄い黄色           */
        {  0.0, 0.00, 0.95}, /*  3776m: 白(山頂)           */
    };
    static const int n = 12;

    double e = (double)elev;

    if (e < -9000.0) {
        hsl_to_rgb(hsl[0][0], hsl[0][1], hsl[0][2], r, g, b);
        return;
    }
    if (e <= 0.0) e = -1.0;

    if (e >= stops[n - 1]) {
        hsl_to_rgb(hsl[n-1][0], hsl[n-1][1], hsl[n-1][2], r, g, b);
        return;
    }
    for (int i = 0; i < n - 1; i++) {
        if (e >= stops[i] && e < stops[i + 1]) {
            double t = (e - stops[i]) / (stops[i + 1] - stops[i]);
            double h = hsl[i][0] + t * (hsl[i+1][0] - hsl[i][0]);
            double s = hsl[i][1] + t * (hsl[i+1][1] - hsl[i][1]);
            double l = hsl[i][2] + t * (hsl[i+1][2] - hsl[i][2]);
            hsl_to_rgb(h, s, l, r, g, b);
            return;
        }
    }
}

/* save_terrain_rgb_image と同じ縮小・出力ロジックだが、カラーマップ関数を差し替え可能にしたもの */
static void save_with_colormap(const ElevTile *big, const char *path,
                                void (*colormap)(float, uint8_t *, uint8_t *, uint8_t *))
{
    uint32_t src_w = big->width;
    uint32_t src_h = big->height;
    uint32_t dst_w, dst_h;
    if (src_w > src_h) {
        dst_w = 6000;
        dst_h = (uint32_t)((double)src_h * dst_w / src_w + 0.5);
    } else {
        dst_h = 6000;
        dst_w = (uint32_t)((double)src_w * dst_h / src_h + 0.5);
    }

    FILE *fp = fopen(path, "wb");
    if (!fp) { fprintf(stderr, "出力失敗: %s\n", path); return; }

    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    png_infop info = png_create_info_struct(png);
    png_init_io(png, fp);
    png_set_IHDR(png, info, dst_w, dst_h, 8, PNG_COLOR_TYPE_RGB,
                 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_write_info(png, info);

    uint8_t *row = malloc(dst_w * 3);
    for (uint32_t y = 0; y < dst_h; y++) {
        uint32_t sy = (uint32_t)((double)y * src_h / dst_h);
        for (uint32_t x = 0; x < dst_w; x++) {
            uint32_t sx = (uint32_t)((double)x * src_w / dst_w);
            float elev = big->data[sy * src_w + sx];
            uint8_t r, g, b;
            colormap(elev, &r, &g, &b);
            row[x * 3 + 0] = r;
            row[x * 3 + 1] = g;
            row[x * 3 + 2] = b;
        }
        png_write_row(png, row);
    }
    free(row);
    png_write_end(png, NULL);
    png_destroy_write_struct(&png, &info);
    fclose(fp);
}

/*
 * 標高データの生バイナリキャッシュ（試作の反復高速化用・本番出力ではない）。
 * 9メッシュ結合のタイル読み込みが数分かかるため、カラーマップ調整のたびに
 * 読み直さずに済むよう width/height/data をそのままダンプ・復元する。
 */
static ElevTile *load_cached_big_tile(const char *cache_path, const char *tile_dir,
                                       const MeshTileRange *combined)
{
    FILE *cf = fopen(cache_path, "rb");
    if (cf) {
        ElevTile *big = malloc(sizeof(ElevTile));
        if (!big) { fclose(cf); return NULL; }
        if (fread(&big->width, sizeof(big->width), 1, cf) == 1 &&
            fread(&big->height, sizeof(big->height), 1, cf) == 1) {
            size_t n = (size_t)big->width * big->height;
            big->data = malloc(n * sizeof(float));
            if (big->data && fread(big->data, sizeof(float), n, cf) == n) {
                fclose(cf);
                printf("キャッシュから読み込み: %s\n", cache_path);
                return big;
            }
            free(big->data);
        }
        free(big);
        fclose(cf);
    }

    ElevTile *big = load_mesh_tile(tile_dir, combined, NULL);
    if (!big) return NULL;

    FILE *wf = fopen(cache_path, "wb");
    if (wf) {
        fwrite(&big->width, sizeof(big->width), 1, wf);
        fwrite(&big->height, sizeof(big->height), 1, wf);
        fwrite(big->data, sizeof(float), (size_t)big->width * big->height, wf);
        fclose(wf);
        printf("キャッシュへ保存: %s\n", cache_path);
    }
    return big;
}

/*
 * PNG出力座標(6000x4927系)から緯度経度を逆算するデバッグモード。
 * 使い方: terrain_colormap_demo --latlon <png_x> <png_y>
 */
static int run_latlon_mode(int argc, char *argv[])
{
    int meshcodes[9] = {5237, 5238, 5239, 5337, 5338, 5339, 5437, 5438, 5439};
    MeshTileRange combined;
    if (mesh_to_tile_range(meshcodes[0], 15, &combined) != 0) return 1;
    for (int i = 1; i < 9; i++) {
        MeshTileRange r;
        if (mesh_to_tile_range(meshcodes[i], 15, &r) != 0) return 1;
        if (r.x_min < combined.x_min) combined.x_min = r.x_min;
        if (r.x_max > combined.x_max) combined.x_max = r.x_max;
        if (r.y_min < combined.y_min) combined.y_min = r.y_min;
        if (r.y_max > combined.y_max) combined.y_max = r.y_max;
    }
    combined.tile_w = combined.x_max - combined.x_min + 1;
    combined.tile_h = combined.y_max - combined.y_min + 1;

    uint32_t src_w = (uint32_t)combined.tile_w * 256 + 2;
    uint32_t src_h = (uint32_t)combined.tile_h * 256 + 2;
    /* save_with_colormap と同じ縮小率(長辺6000px)を再現 */
    uint32_t dst_w, dst_h;
    if (src_w > src_h) {
        dst_w = 6000;
        dst_h = (uint32_t)((double)src_h * dst_w / src_w + 0.5);
    } else {
        dst_h = 6000;
        dst_w = (uint32_t)((double)src_w * dst_h / src_h + 0.5);
    }

    int png_x = atoi(argv[2]);
    int png_y = atoi(argv[3]);
    int orig_x = (int)((double)png_x * src_w / dst_w);
    int orig_y = (int)((double)png_y * src_h / dst_h);

    double lat, lon;
    pixel_to_latlon(&combined, orig_x, orig_y, &lat, &lon);
    printf("PNG座標(%d,%d) [出力%ux%u] → 緯度%.5f 経度%.5f\n", png_x, png_y, dst_w, dst_h, lat, lon);
    return 0;
}

int main(int argc, char *argv[])
{
    if (argc >= 4 && strcmp(argv[1], "--latlon") == 0) {
        return run_latlon_mode(argc, argv);
    }

    int meshcodes[9] = {5237, 5238, 5239, 5337, 5338, 5339, 5437, 5438, 5439};
    int center_meshcode = 5338;
    if (argc >= 2) center_meshcode = atoi(argv[1]);

    load_dotenv("params/config.ini");
    const char *data_dir = getenv("DATA_DIR");
    if (!data_dir || data_dir[0] == '\0') data_dir = "/data";

    char tile_dir[512], out_current[512], out_monotone[512], out_landscape[512], out_ref15[512], out_ref15sw[512], cache_path[512];
    snprintf(tile_dir, sizeof(tile_dir), "%s/tiles", data_dir);
    snprintf(out_current, sizeof(out_current), "%s/images/%d_cmp_current.png", data_dir, center_meshcode);
    snprintf(out_monotone, sizeof(out_monotone), "%s/images/%d_cmp_monotone.png", data_dir, center_meshcode);
    snprintf(out_landscape, sizeof(out_landscape), "%s/images/%d_cmp_landscape.png", data_dir, center_meshcode);
    snprintf(out_ref15, sizeof(out_ref15), "%s/images/%d_cmp_ref15.png", data_dir, center_meshcode);
    snprintf(out_ref15sw, sizeof(out_ref15sw), "%s/images/%d_cmp_ref15_swapped.png", data_dir, center_meshcode);
    snprintf(cache_path, sizeof(cache_path), "%s/images/%d_cmp_bigtile.cache", data_dir, center_meshcode);

    printf("=== カラーマップ比較試作ツール ===\n");
    printf("現行版出力先    : %s\n", out_current);
    printf("明度単調版出力先: %s\n", out_monotone);
    printf("地物準拠版出力先: %s\n", out_landscape);
    printf("参考15段階版出力先: %s\n", out_ref15);
    printf("参考15段階(緑茶交換)版出力先: %s\n", out_ref15sw);

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

    ElevTile *big = load_cached_big_tile(cache_path, tile_dir, &combined);
    if (!big) {
        fprintf(stderr, "イメージ作成失敗\n");
        return 1;
    }

    save_terrain_rgb_image(big, out_current, NULL);
    printf("現行版出力完了\n");
    save_with_colormap(big, out_monotone, elev_to_rgb_monotone);
    printf("明度単調版出力完了\n");
    save_with_colormap(big, out_landscape, elev_to_rgb_landscape);
    printf("地物準拠版出力完了\n");
    save_with_colormap(big, out_ref15, elev_to_rgb_ref15);
    printf("参考15段階版出力完了\n");
    save_with_colormap(big, out_ref15sw, elev_to_rgb_ref15_swapped);
    printf("参考15段階(緑茶交換)版出力完了\n");

    elev_destroy(big);
    printf("\n完了\n");
    return 0;
}
