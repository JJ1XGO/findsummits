/*
 * fetch.c - 標高タイルのダウンロードとキャッシュ管理
 *
 * SOTA日本支部 未登録サミット候補探索ツール
 * Copyright (C) 2026 JJ1XGO
 * GPL-3.0
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <curl/curl.h>
#include "fetch.h"

/* URL形式: dem5{a/b/c}_png */
#define GSI_DEM5_URL "https://cyberjapandata.gsi.go.jp/xyz/dem5%s_png/15/%d/%d.png"
/* URL形式: dem10b_png (z=14) */
#define GSI_DEM10B_URL "https://cyberjapandata.gsi.go.jp/xyz/dem10b_png/14/%d/%d.png"

void make_tile_cache_path(char *buf, size_t bufsize,
                          const char *base, int z,
                          int x, int y, const char *dem)
{
    snprintf(buf, bufsize, "%s/%d/%d/%d_%s.png", base, z, x, y, dem);
}

int tile_is_cached(const char *base, int x, int y)
{
    char path[512];
    const char *dems[] = {"a", "b", "c"};
    struct stat st;
    for (int d = 0; d < 3; d++) {
        make_tile_cache_path(path, sizeof(path), base, 15, x, y, dems[d]);
        if (stat(path, &st) == 0)
            return 1;
    }
    return 0;
}

int tile_dem10b_is_cached(const char *base, int x14, int y14)
{
    char path[512];
    struct stat st;
    make_tile_cache_path(path, sizeof(path), base, 14, x14, y14, "b");
    return stat(path, &st) == 0;
}

static size_t write_to_file(void *ptr, size_t size,
                             size_t nmemb, void *stream)
{
    return fwrite(ptr, size, nmemb, (FILE*)stream);
}

static long download_url(const char *url, const char *path)
{
    CURL *curl = curl_easy_init();
    if (!curl) return -1;

    FILE *fp = fopen(path, "wb");
    if (!fp) { curl_easy_cleanup(curl); return -1; }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_file);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    CURLcode res = curl_easy_perform(curl);
    fclose(fp);

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK || http_code != 200) {
        remove(path);
        return -1;
    }

    struct stat st;
    if (stat(path, &st) == 0) return st.st_size;
    return -1;
}

static void ensure_dirs(const char *base, int z, int x)
{
    char dir[512];
    snprintf(dir, sizeof(dir), "%s/%d", base, z);
    mkdir(dir, 0755);
    snprintf(dir, sizeof(dir), "%s/%d/%d", base, z, x);
    mkdir(dir, 0755);
}

int fetch_tile(const FetchConfig *cfg, int x, int y)
{
    if (tile_is_cached(cfg->tile_dir, x, y))
        return 1;

    ensure_dirs(cfg->tile_dir, 15, x);

    char path[512];
    char url[512];
    const char *dems[] = {"a", "b", "c"};

    for (int d = 0; d < 3; d++) {
        make_tile_cache_path(path, sizeof(path), cfg->tile_dir, 15, x, y, dems[d]);
        snprintf(url, sizeof(url), GSI_DEM5_URL, dems[d], x, y);

        long size = download_url(url, path);
        if (size > 0)
            return 1;  /* ダウンロード成功 */
    }
    return 0;
}

int fetch_dem10b_tile(const FetchConfig *cfg, int x14, int y14)
{
    if (tile_dem10b_is_cached(cfg->tile_dir, x14, y14))
        return 1;

    ensure_dirs(cfg->tile_dir, 14, x14);

    char path[512];
    char url[512];
    make_tile_cache_path(path, sizeof(path), cfg->tile_dir, 14, x14, y14, "b");
    snprintf(url, sizeof(url), GSI_DEM10B_URL, x14, y14);

    long size = download_url(url, path);
    return size > 0 ? 1 : 0;
}

/*
 * スレッド用ワーカー
 */
typedef struct {
    const FetchConfig *cfg;
    int    *tile_xs;
    int    *tile_ys;
    int     n;
    int     fetched;
    int     nodata;
} WorkerArg;

static void *worker_thread(void *arg)
{
    WorkerArg *wa = (WorkerArg*)arg;
    wa->fetched = 0;
    wa->nodata  = 0;

    for (int i = 0; i < wa->n; i++) {
        if (tile_is_cached(wa->cfg->tile_dir,
                           wa->tile_xs[i], wa->tile_ys[i])) {
            continue;
        }
        int ret = fetch_tile(wa->cfg, wa->tile_xs[i], wa->tile_ys[i]);
        if (ret == 1) {
            wa->fetched++;
            if (wa->cfg->interval_ms > 0)
                usleep(wa->cfg->interval_ms * 1000);
        } else {
            wa->nodata++;
        }
    }
    return NULL;
}

static void format_elapsed(char *buf, size_t bufsize, double sec)
{
    int h = (int)(sec / 3600);
    int m = (int)((sec - h * 3600) / 60);
    int s = (int)(sec - h * 3600 - m * 60);
    if (h > 0)
        snprintf(buf, bufsize, "%d時間%d分%d秒", h, m, s);
    else if (m > 0)
        snprintf(buf, bufsize, "%d分%d秒", m, s);
    else
        snprintf(buf, bufsize, "%d秒", s);
}

static void format_remaining(char *buf, size_t bufsize,
                              double elapsed, int done, int total)
{
    if (done == 0) { snprintf(buf, bufsize, "計算中..."); return; }
    double remaining = elapsed / done * (total - done);
    format_elapsed(buf, bufsize, remaining);
}

/*
 * 1次メッシュ全タイルを並列ダウンロードする
 */
int fetch_mesh(const FetchConfig *cfg, const MeshTileRange *range)
{
    int total = range->tile_w * range->tile_h;
    int nthreads = cfg->max_parallel > 0 ? cfg->max_parallel : 4;

    printf("タイル取得開始: %d枚 (%d並列)\n", total, nthreads);

    int *xs = malloc(sizeof(int) * total);
    int *ys = malloc(sizeof(int) * total);
    if (!xs || !ys) { free(xs); free(ys); return -1; }

    int idx = 0;
    int cached = 0;
    for (int ty = range->y_min; ty <= range->y_max; ty++)
        for (int tx = range->x_min; tx <= range->x_max; tx++) {
            if (tile_is_cached(cfg->tile_dir, tx, ty)) {
                cached++;
            } else {
                xs[idx] = tx;
                ys[idx] = ty;
                idx++;
            }
        }

    int need = idx;
    printf("  キャッシュ済み: %d枚 / 未取得: %d枚\n", cached, need);

    if (need == 0) {
        printf("タイル取得完了: 全てキャッシュ済み\n");
        free(xs); free(ys);
        return total;
    }

    if (nthreads > need) nthreads = need;
    pthread_t *threads   = malloc(sizeof(pthread_t)  * nthreads);
    WorkerArg *args      = malloc(sizeof(WorkerArg)  * nthreads);
    int      **thread_xs = malloc(sizeof(int*)       * nthreads);
    int      **thread_ys = malloc(sizeof(int*)       * nthreads);

    int base_n = need / nthreads;
    int rem    = need % nthreads;
    int pos    = 0;

    for (int t = 0; t < nthreads; t++) {
        int n = base_n + (t < rem ? 1 : 0);
        thread_xs[t] = xs + pos;
        thread_ys[t] = ys + pos;
        args[t].cfg     = cfg;
        args[t].tile_xs = thread_xs[t];
        args[t].tile_ys = thread_ys[t];
        args[t].n       = n;
        pos += n;
    }

    struct timespec ts_start, ts_now;
    clock_gettime(CLOCK_MONOTONIC, &ts_start);

    for (int t = 0; t < nthreads; t++)
        pthread_create(&threads[t], NULL, worker_thread, &args[t]);

    while (1) {
        sleep(5);

        int fetched = 0, nodata = 0;
        for (int t = 0; t < nthreads; t++) {
            fetched += args[t].fetched;
            nodata  += args[t].nodata;
        }
        int done = fetched + nodata;

        clock_gettime(CLOCK_MONOTONIC, &ts_now);
        double elapsed = (ts_now.tv_sec  - ts_start.tv_sec) +
                         (ts_now.tv_nsec - ts_start.tv_nsec) / 1e9;

        char sel[32], srem[32];
        format_elapsed(sel, sizeof(sel), elapsed);
        format_remaining(srem, sizeof(srem), elapsed, cached + done, total);

        printf("  [%s経過 残り約%s] %d/%d"
               " (取得:%d キャッシュ:%d データなし:%d)\n",
               sel, srem, cached + done, total,
               fetched, cached, nodata);
        fflush(stdout);

        if (done >= need) break;
    }

    int fetched = 0, nodata = 0;
    for (int t = 0; t < nthreads; t++) {
        pthread_join(threads[t], NULL);
        fetched += args[t].fetched;
        nodata  += args[t].nodata;
    }

    free(threads); free(args);
    free(thread_xs); free(thread_ys);
    free(xs); free(ys);

    printf("タイル取得完了: 取得:%d キャッシュ:%d データなし:%d\n",
           fetched, cached, nodata);
    return fetched + cached;
}
