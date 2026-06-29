#!/usr/bin/env python3
"""
prefetch_tiles.py - 国土地理院標高タイルを事前一括取得する

3×3最小矩形解析の前に実行して、全タイルをキャッシュしておく。
If-Modified-Since を使って更新分のみダウンロードする。

依存: requests
"""
import argparse
import configparser
import datetime
import email.utils
import math
import os
import queue
import sys
import threading
import time

import requests

GSI_DEM5_URL  = "https://cyberjapandata.gsi.go.jp/xyz/dem5{dem}_png/15/{x}/{y}.png"
GSI_DEM10B_URL = "https://cyberjapandata.gsi.go.jp/xyz/dem_png/14/{x}/{y}.png"


# ---- メッシュ座標変換 ----

def latlon_to_tile(lat, lon, z):
    n = 2 ** z
    tx = int((lon + 180.0) / 360.0 * n)
    sin_lat = math.sin(math.radians(lat))
    ty = int((1.0 - math.log((sin_lat + 1.0) / math.cos(math.radians(lat))) / math.pi) / 2.0 * n)
    return tx, ty


def mesh_to_tile_range(meshcode, z=15):
    lat_code = meshcode // 100
    lon_code = meshcode % 100
    lat_north = (lat_code + 1) * 2.0 / 3.0
    lat_south = lat_code * 2.0 / 3.0
    lon_west  = lon_code + 100.0
    lon_east  = lon_code + 101.0
    tx_w, ty_n = latlon_to_tile(lat_north, lon_west, z)
    tx_e, ty_s = latlon_to_tile(lat_south, lon_east, z)
    return tx_w, tx_e, ty_n, ty_s


def mesh_neighbor(code, dlat, dlon):
    return (code // 100 + dlat) * 100 + (code % 100 + dlon)


def load_mesh_set(path):
    codes = set()
    with open(path) as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith('#'):
                continue
            try:
                c = int(line)
                if 1000 <= c <= 9999:
                    codes.add(c)
            except ValueError:
                pass
    return codes


# ---- タイルパス ----

def tile_path(tile_dir, z, x, y, dem):
    service = "dem_png" if z == 14 else f"dem5{dem}_png"
    return os.path.join(tile_dir, service, str(z), str(x), f"{y}.png")


def tile_url(z, x, y, dem):
    if z == 15:
        return GSI_DEM5_URL.format(dem=dem, x=x, y=y)
    else:
        return GSI_DEM10B_URL.format(x=x, y=y)


# ---- 取得ジョブの列挙 ----

def enumerate_jobs(mesh_set, tile_dir):
    """(z, x, y, dem, path) のリストを返す（重複除去済み）"""
    seen = set()
    jobs = []

    for meshcode in sorted(mesh_set):
        x_min, x_max, y_min, y_max = mesh_to_tile_range(meshcode, 15)

        # dem5 タイル (z=15): a→b→c フォールバックのため起点 "a" のみ列挙
        for ty in range(y_min, y_max + 1):
            for tx in range(x_min, x_max + 1):
                key = (15, tx, ty)
                if key not in seen:
                    seen.add(key)
                    path = tile_path(tile_dir, 15, tx, ty, "a")
                    jobs.append((15, tx, ty, "a", path))

        # dem10b タイル (z=14): z15 範囲を z14 に変換
        x14_min, x14_max = x_min // 2, x_max // 2
        y14_min, y14_max = y_min // 2, y_max // 2
        for ty14 in range(y14_min, y14_max + 1):
            for tx14 in range(x14_min, x14_max + 1):
                key = (14, tx14, ty14, "b")
                if key not in seen:
                    seen.add(key)
                    path = tile_path(tile_dir, 14, tx14, ty14, "b")
                    jobs.append((14, tx14, ty14, "b", path))

    return jobs


# ---- HTTP 取得 ----

def fetch_one(z, x, y, dem, path, user_agent, interval_ms, backoff_initial):
    """
    1タイルを取得する。

    戻り値: ("ok", None) | ("304", None) | ("404", None) | ("err", msg)
    """
    url = tile_url(z, x, y, dem)
    headers = {"User-Agent": user_agent}

    # If-Modified-Since: キャッシュ済みならファイルの mtime を使う
    if os.path.exists(path):
        mtime = os.path.getmtime(path)
        headers["If-Modified-Since"] = email.utils.formatdate(mtime, usegmt=True)

    backoff = backoff_initial
    for attempt in range(5):
        time.sleep(interval_ms / 1000.0)
        try:
            resp = requests.get(url, headers=headers, timeout=30)
            if resp.status_code == 304:
                return ("304", None)
            if resp.status_code == 404:
                return ("404", None)
            if resp.status_code in (429, 503):
                retry_after = resp.headers.get("Retry-After")
                wait = int(retry_after) if retry_after and retry_after.isdigit() else backoff
                time.sleep(wait)
                backoff = min(backoff * 2, 600)
                continue
            resp.raise_for_status()
            os.makedirs(os.path.dirname(path), exist_ok=True)
            with open(path, "wb") as f:
                f.write(resp.content)
            last_mod = resp.headers.get("Last-Modified")
            if last_mod:
                try:
                    ts = email.utils.parsedate_to_datetime(last_mod).timestamp()
                    os.utime(path, (ts, ts))
                except Exception:
                    pass
            return ("ok", None)

        except requests.exceptions.RequestException as e:
            if attempt < 4:
                time.sleep(backoff)
                backoff = min(backoff * 2, 600)
                continue
            return ("err", f"{e}: {url}")

    return ("err", f"max retries: {url}")


# ---- dem5 フォールバック取得 ----

def fetch_dem5_with_fallback(x, y, tile_dir, user_agent, interval_ms, backoff_initial):
    """
    dem5a → dem5b → dem5c の順に試みる。
    戻り値: [(dem, status, msg), ...] 試行した全ステップのリスト
    """
    attempts = []
    for dem in ("a", "b", "c"):
        path = tile_path(tile_dir, 15, x, y, dem)
        status, msg = fetch_one(15, x, y, dem, path, user_agent, interval_ms, backoff_initial)
        attempts.append((dem, status, msg))
        if status in ("ok", "304"):
            break
        if status != "404":
            break
    return attempts


# ---- ワーカースレッド ----

PROGRESS_INTERVAL = 1000

def worker(job_queue, results, tile_dir, user_agent, interval_ms, backoff_initial,
           lock, counters, start_time, total_jobs):
    while True:
        try:
            job = job_queue.get_nowait()
        except queue.Empty:
            break
        z, x, y, dem, path = job
        if z == 15:
            attempts = fetch_dem5_with_fallback(x, y, tile_dir, user_agent, interval_ms, backoff_initial)
            with lock:
                for dem_tried, status, msg in attempts:
                    key = f"dem5{dem_tried}"
                    bucket = status if status in ("ok", "304", "404") else "err"
                    counters[key][bucket] += 1
                    if status == "err":
                        print(f"  [ERR] {msg}", file=sys.stderr)
                _print_progress_if_needed(counters, start_time, total_jobs)
        else:
            status, msg = fetch_one(z, x, y, dem, path, user_agent, interval_ms, backoff_initial)
            with lock:
                bucket = status if status in ("ok", "304", "404") else "err"
                counters["dem10b"][bucket] += 1
                if status == "err":
                    print(f"  [ERR] {msg}", file=sys.stderr)
                _print_progress_if_needed(counters, start_time, total_jobs)
        job_queue.task_done()


def _total_done(counters):
    return sum(sum(c.values()) for c in counters.values())


def _fmt_hms(seconds):
    seconds = int(seconds)
    h, rem = divmod(seconds, 3600)
    m, s = divmod(rem, 60)
    return f"{h:02d}:{m:02d}:{s:02d}"


def _print_progress_if_needed(counters, start_time, total_jobs):
    done = _total_done(counters)
    if done % PROGRESS_INTERVAL == 0:
        ok  = sum(c["ok"]  for c in counters.values())
        s304= sum(c["304"] for c in counters.values())
        s404= sum(c["404"] for c in counters.values())
        err = sum(c["err"] for c in counters.values())
        elapsed = time.time() - start_time
        if done > 0 and elapsed > 0:
            rate = done / elapsed
            remaining = (total_jobs - done) / rate if rate > 0 else 0
            time_str = f" | 経過:{_fmt_hms(elapsed)} 残り:{_fmt_hms(remaining)}"
        else:
            time_str = ""
        pct = done / total_jobs * 100 if total_jobs > 0 else 0
        msg = (f"  進捗: {done}/{total_jobs} ({pct:.1f}%)"
               f" | 取得:{ok} 変更なし:{s304} 存在なし:{s404} エラー:{err}{time_str}")
        print(msg, flush=True)


# ---- サマリー生成 ----

def build_summary(start_dt, end_dt, mesh_set, jobs, counters, user_agent, max_parallel, interval_ms, backoff_init):
    elapsed = end_dt - start_dt
    elapsed_str = str(elapsed).split(".")[0]

    lines = []
    lines.append("=" * 60)
    lines.append("prefetch_tiles 実行サマリー")
    lines.append("=" * 60)
    lines.append(f"開始:       {start_dt.strftime('%Y-%m-%d %H:%M:%S')}")
    lines.append(f"終了:       {end_dt.strftime('%Y-%m-%d %H:%M:%S')}")
    lines.append(f"所要時間:   {elapsed_str}")
    lines.append(f"メッシュ:   {sorted(mesh_set)}")
    lines.append(f"ジョブ数:   {len(jobs)}")
    lines.append(f"並列数: {max_parallel}  間隔: {interval_ms}ms  バックオフ初期: {backoff_init}s")
    lines.append(f"User-Agent: {user_agent}")
    lines.append("")

    col_w = 10
    h_label = f"{'':12}"
    h_ok    = f"{'取得':>{col_w}}"
    h_304   = f"{'変更なし':>{col_w}}"
    h_404   = f"{'存在なし':>{col_w}}"
    h_err   = f"{'エラー':>{col_w}}"
    h_tot   = f"{'合計':>{col_w}}"
    header = h_label + h_ok + h_304 + h_404 + h_err + h_tot
    sep = "-" * len(header)
    lines.append(header)
    lines.append(sep)

    total = {"ok": 0, "304": 0, "404": 0, "err": 0}
    for key in ("dem5a", "dem5b", "dem5c", "dem10b"):
        c = counters[key]
        row_sum = sum(c.values())
        if row_sum == 0:
            continue
        lines.append(
            f"{key:12}"
            f"{c['ok']:>{col_w}}"
            f"{c['304']:>{col_w}}"
            f"{c['404']:>{col_w}}"
            f"{c['err']:>{col_w}}"
            f"{row_sum:>{col_w}}"
        )
        for k in total:
            total[k] += c[k]

    lines.append(sep)
    total_all = sum(total.values())
    lines.append(
        f"{'合計':12}"
        f"{total['ok']:>{col_w}}"
        f"{total['304']:>{col_w}}"
        f"{total['404']:>{col_w}}"
        f"{total['err']:>{col_w}}"
        f"{total_all:>{col_w}}"
    )
    lines.append("=" * 60)
    lines.append("※ dem5b/c の行はフォールバック試行分（dem5a/b が 404 だった座標のみ）")
    return "\n".join(lines)


# ---- メイン ----

def main():
    parser = argparse.ArgumentParser(description="国土地理院標高タイル事前取得スクリプト")
    parser.add_argument("--mesh-list", required=True, help="メッシュコードリストファイル")
    parser.add_argument("--config",    required=True, help="fetch_config.ini ファイル")
    parser.add_argument("--tile-dir",  required=True, help="タイルキャッシュディレクトリ")
    parser.add_argument("--log-dir",   default=None,  help="ログ出力ディレクトリ（省略時はファイル出力なし）")
    args = parser.parse_args()

    cfg = configparser.ConfigParser()
    cfg.read(args.config)
    email_addr    = cfg.get("fetch", "user_agent_email",  fallback="anonymous")
    max_parallel  = cfg.getint("fetch", "max_parallel",   fallback=4)
    interval_ms   = cfg.getint("fetch", "interval_ms",    fallback=100)
    backoff_init  = cfg.getint("fetch", "backoff_initial_sec", fallback=60)
    user_agent    = f"findsummits/1.0 (mailto:{email_addr})"

    mesh_set = load_mesh_set(args.mesh_list)
    jobs = enumerate_jobs(mesh_set, args.tile_dir)

    start_dt = datetime.datetime.now()
    print(f"開始: {start_dt.strftime('%Y-%m-%d %H:%M:%S')}")
    print(f"メッシュ: {sorted(mesh_set)}  ジョブ数: {len(jobs)}")
    print(f"並列数: {max_parallel}  間隔: {interval_ms}ms  バックオフ初期: {backoff_init}s")

    job_queue = queue.Queue()
    for job in jobs:
        job_queue.put(job)

    start_time = time.time()
    total_jobs = len(jobs)
    lock = threading.Lock()
    counters = {
        "dem5a":  {"ok": 0, "304": 0, "404": 0, "err": 0},
        "dem5b":  {"ok": 0, "304": 0, "404": 0, "err": 0},
        "dem5c":  {"ok": 0, "304": 0, "404": 0, "err": 0},
        "dem10b": {"ok": 0, "304": 0, "404": 0, "err": 0},
    }

    threads = []
    for _ in range(max_parallel):
        t = threading.Thread(
            target=worker,
            args=(job_queue, None, args.tile_dir, user_agent, interval_ms, backoff_init,
                  lock, counters, start_time, total_jobs),
            daemon=True,
        )
        t.start()
        threads.append(t)

    for t in threads:
        t.join()

    end_dt = datetime.datetime.now()
    summary = build_summary(start_dt, end_dt, mesh_set, jobs, counters,
                            user_agent, max_parallel, interval_ms, backoff_init)
    print("\n" + summary)

    if args.log_dir:
        os.makedirs(args.log_dir, exist_ok=True)
        log_name = f"prefetch_{start_dt.strftime('%Y%m%d_%H%M%S')}.log"
        log_path = os.path.join(args.log_dir, log_name)
        with open(log_path, "w", encoding="utf-8") as f:
            f.write(summary + "\n")
        print(f"\nログ保存: {log_path}")


if __name__ == "__main__":
    main()
