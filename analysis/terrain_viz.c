#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <png.h>

/* ----------------------------------------
   標高デコード
---------------------------------------- */
double decode_elevation(int r, int g, int b) {
    return -10000.0 + ((r * 65536.0 + g * 256.0 + b) * 0.1);
}

/* ----------------------------------------
   2色間の線形補間
---------------------------------------- */
void lerp_color(double t,
                int r1, int g1, int b1,
                int r2, int g2, int b2,
                int *r, int *g, int *b) {
    if (t < 0.0) t = 0.0;
    if (t > 1.0) t = 1.0;
    *r = (int)(r1 + t * (r2 - r1));
    *g = (int)(g1 + t * (g2 - g1));
    *b = (int)(b1 + t * (b2 - b1));
}

/* ----------------------------------------
   標高 → グラデーション色
   日本の地形向けに調整済み
---------------------------------------- */
void elevation_to_color(double elev, int *r, int *g, int *b) {

    // 標高の区切り点（m）
    static const double stops[] = {
        -200.0,   //  0: 深海
           0.0,   //  1: 海岸線
         150.0,   //  2: 低地下限（SOTAバンド1開始）
         500.0,   //  3: バンド1/2境界
         650.0,   //  4: バンド2/3境界
         850.0,   //  5: バンド3/4境界
        1100.0,   //  6: バンド4/5境界
        1500.0,   //  7: バンド5/6境界
        3000.0,   //  8: 高山帯
        3800.0    //  9: 富士山頂付近
    };

    // 対応する色
    static const int colors[][3] = {
        { 20,  60, 150},  //  0: 深海：濃紺
        { 65, 150, 210},  //  1: 海岸線：水色
        {180, 220, 140},  //  2: 低地：黄緑
        {120, 185,  80},  //  3: 丘陵：緑
        {190, 160,  90},  //  4: 山麓：黄茶
        {160, 120,  60},  //  5: 中山：茶色
        {130,  90,  50},  //  6: 高山麓：濃茶
        {180, 170, 160},  //  7: 亜高山：灰色
        {220, 215, 210},  //  8: 高山帯：明るい灰
        {255, 255, 255}   //  9: 山頂付近：白
    };

    int n = 10;

    // 範囲外
    if (elev <= stops[0]) {
        *r = colors[0][0]; *g = colors[0][1]; *b = colors[0][2];
        return;
    }
    if (elev >= stops[n-1]) {
        *r = colors[n-1][0]; *g = colors[n-1][1]; *b = colors[n-1][2];
        return;
    }

    // 該当区間を補間
    for (int i = 0; i < n-1; i++) {
        if (elev >= stops[i] && elev < stops[i+1]) {
            double t = (elev - stops[i]) / (stops[i+1] - stops[i]);
            lerp_color(t,
                colors[i][0],   colors[i][1],   colors[i][2],
                colors[i+1][0], colors[i+1][1], colors[i+1][2],
                r, g, b);
            return;
        }
    }
}

/* ----------------------------------------
   PNG読み込み
---------------------------------------- */
int read_png(const char *filename,
             int *width, int *height,
             png_bytep **row_pointers) {

    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        fprintf(stderr, "ERROR: cannot open %s\n", filename);
        return -1;
    }

    png_structp png = png_create_read_struct(
                        PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    png_infop info = png_create_info_struct(png);

    if (setjmp(png_jmpbuf(png))) {
        fprintf(stderr, "ERROR: libpng read error\n");
        fclose(fp);
        return -1;
    }

    png_init_io(png, fp);
    png_read_info(png, info);

    *width  = png_get_image_width(png, info);
    *height = png_get_image_height(png, info);

    // 8bit RGBに統一
    png_set_strip_alpha(png);
    png_set_strip_16(png);
    png_set_packing(png);
    png_read_update_info(png, info);

    *row_pointers = (png_bytep *)malloc(sizeof(png_bytep) * (*height));
    for (int y = 0; y < *height; y++) {
        (*row_pointers)[y] = (png_byte *)malloc(
                                png_get_rowbytes(png, info));
    }

    png_read_image(png, *row_pointers);

    png_destroy_read_struct(&png, &info, NULL);
    fclose(fp);
    return 0;
}

/* ----------------------------------------
   PNG書き出し
---------------------------------------- */
int write_png(const char *filename,
              int width, int height,
              png_bytep *row_pointers) {

    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        fprintf(stderr, "ERROR: cannot open %s\n", filename);
        return -1;
    }

    png_structp png = png_create_write_struct(
                        PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    png_infop info = png_create_info_struct(png);

    if (setjmp(png_jmpbuf(png))) {
        fprintf(stderr, "ERROR: libpng write error\n");
        fclose(fp);
        return -1;
    }

    png_init_io(png, fp);
    png_set_IHDR(png, info, width, height,
                 8, PNG_COLOR_TYPE_RGB,
                 PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_DEFAULT,
                 PNG_FILTER_TYPE_DEFAULT);
    png_write_info(png, info);
    png_write_image(png, row_pointers);
    png_write_end(png, NULL);

    png_destroy_write_struct(&png, &info);
    fclose(fp);
    return 0;
}

/* ----------------------------------------
   メイン処理
---------------------------------------- */
int main(int argc, char *argv[]) {

    if (argc != 3) {
        fprintf(stderr, "Usage: %s input.png output.png\n", argv[0]);
        return 1;
    }

    const char *input_file  = argv[1];
    const char *output_file = argv[2];

    // 読み込み
    int width, height;
    png_bytep *src_rows = NULL;

    if (read_png(input_file, &width, &height, &src_rows) != 0) {
        return 1;
    }
    printf("INFO: %s  size=%dx%d\n", input_file, width, height);

    // 出力バッファ確保
    png_bytep *dst_rows = (png_bytep *)malloc(sizeof(png_bytep) * height);
    for (int y = 0; y < height; y++) {
        dst_rows[y] = (png_byte *)malloc(width * 3);
    }

    // 標高統計（デバッグ用）
    double elev_min =  1e9;
    double elev_max = -1e9;

    // ピクセル変換
    for (int y = 0; y < height; y++) {
        png_bytep src_row = src_rows[y];
        png_bytep dst_row = dst_rows[y];

        for (int x = 0; x < width; x++) {
            int r = src_row[x * 3 + 0];
            int g = src_row[x * 3 + 1];
            int b = src_row[x * 3 + 2];

            double elev = decode_elevation(r, g, b);

            // 統計更新
            if (elev < elev_min) elev_min = elev;
            if (elev > elev_max) elev_max = elev;

            // 色変換
            int cr, cg, cb;
            elevation_to_color(elev, &cr, &cg, &cb);

            dst_row[x * 3 + 0] = (png_byte)cr;
            dst_row[x * 3 + 1] = (png_byte)cg;
            dst_row[x * 3 + 2] = (png_byte)cb;
        }
    }

    printf("INFO: elev_min=%.1fm  elev_max=%.1fm\n", elev_min, elev_max);

    // 書き出し
    if (write_png(output_file, width, height, dst_rows) != 0) {
        return 1;
    }
    printf("INFO: wrote %s\n", output_file);

    // メモリ解放
    for (int y = 0; y < height; y++) {
        free(src_rows[y]);
        free(dst_rows[y]);
    }
    free(src_rows);
    free(dst_rows);

    return 0;
}
