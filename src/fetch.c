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

#define GSI_URL_FMT \
    "https://cyberjapandata.gsi.go.jp/xyz/dem5%s_png/15/%d/%d.png"
#define MIN_VALID_SIZE 1000

void make_tile_cache_path(char *buf, size_t bufsize,
                          const char *tile_dir,
                          int x, int y, const char *dem)
{
    snprintf(buf, bufsize, "%s/%d/%d_%s.png", tile_dir, x, y, dem);
}

int tile_is_cached(const char *tile_dir, int x, int y)
{
    char path[512];
    const char *dems[] = {"a", "b", "c"};
    struct stat st;
    for (int d = 0; d < 3; d++) {
        make_tile_cache_path(path, sizeof(path), tile_dir, x, y, dems[d]);
        if (stat(path, &st) == 0 && st.st_size > MIN_VALID_SIZE)
            return 1;
    }
    return 0;
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

int fetch_tile(const FetchConfig *cfg, int x, int y)
{
    if (tile_is_cached(cfg->tile_dir, x, y))
        return 1;

    char dirpath[512];
    snprintf(dirpath, sizeof(dirpath), "%s/%d", cfg->tile_dir, x);
    mkdir(dirpath, 0755);

    char path[512];
    char url[512];
    const char *dems[] = {"a", "b", "c"};

    for (int d = 0; d < 3; d++) {
        make_tile_cache_path(path, sizeof(path),
                             cfg->tile_dir, x, y, dems[d]);
        snprintf(url, sizeof(url), GSI_URL_FMT, dems[d], x, y);

        long size = download_url(url, path);

        if (size > MIN_VALID_SIZE) {
            return 1;
        } else if (size > 0 && size <= MIN_VALID_SIZE) {
            remove(path);
            break;  /* dem5aが小さければ海・範囲外 */
        } else {
            if (size > 0) remove(path);
        }
    }
    return 0;
}

/*
 * スレッド用ワーカー
 */
typedef struct {
    const FetchConfig *cfg;
    int    *tile_xs;    /* タイルX座標の配列 */
    int    *tile_ys;    /* タイルY座標の配列 */
    int     n;          /* 担当タイル数 */
    int     fetched;    /* 取得成功数 */
    int     nodata;     /* データなし数 */
} WorkerArg;

static void *worker_thread(void *arg)
{
    WorkerArg *wa = (WorkerArg*)arg;
    wa->fetched = 0;
    wa->nodata  = 0;

    for (int i = 0; i < wa->n; i++) {
        if (tile_is_cached(wa->cfg->tile_dir,
                           wa->tile_xs[i], wa->tile_ys[i])) {
            continue;  /* キャッシュ済みはスキップ */
        }
        int ret = fetch_tile(wa->cfg, wa->tile_xs[i], wa->tile_ys[i]);
        if (ret == 1) {
            wa->fetched++;
            /* DL成功時のみウェイト */
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

    /* 全タイル座標をリスト化 */
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

    int need = idx;  /* キャッシュ未済のタイル数 */
    printf("  キャッシュ済み: %d枚 / 未取得: %d枚\n", cached, need);

    if (need == 0) {
        printf("タイル取得完了: 全てキャッシュ済み\n");
        free(xs); free(ys);
        return total;
    }

    /* スレッドにタイルを分配 */
    if (nthreads > need) nthreads = need;
    pthread_t *threads   = malloc(sizeof(pthread_t)  * nthreads);
    WorkerArg *args      = malloc(sizeof(WorkerArg)  * nthreads);
    int      **thread_xs = malloc(sizeof(int*)       * nthreads);
    int      **thread_ys = malloc(sizeof(int*)       * nthreads);

    int base = need / nthreads;
    int rem  = need % nthreads;
    int pos  = 0;

    for (int t = 0; t < nthreads; t++) {
        int n = base + (t < rem ? 1 : 0);
        thread_xs[t] = xs + pos;
        thread_ys[t] = ys + pos;
        args[t].cfg     = cfg;
        args[t].tile_xs = thread_xs[t];
        args[t].tile_ys = thread_ys[t];
        args[t].n       = n;
        pos += n;
    }

    /* スレッド起動 */
    struct timespec ts_start, ts_now;
    clock_gettime(CLOCK_MONOTONIC, &ts_start);

    for (int t = 0; t < nthreads; t++)
        pthread_create(&threads[t], NULL, worker_thread, &args[t]);

    /* 進捗監視(メインスレッド) */
    while (1) {
        sleep(5);  /* 5秒ごとに進捗表示 */

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
        format_remaining(srem, sizeof(srem), elapsed,
                         cached + done, total);

        printf("  [%s経過 残り約%s] %d/%d"
               " (取得:%d キャッシュ:%d データなし:%d)\n",
               sel, srem, cached + done, total,
               fetched, cached, nodata);
        fflush(stdout);

        if (done >= need) break;  /* 全スレッド完了 */
    }

    /* スレッド終了待ち */
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
