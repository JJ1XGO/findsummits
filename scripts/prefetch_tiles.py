#!/usr/bin/env python3
"""
prefetch_tiles.py - 国土地理院標高タイルを事前一括取得する

3×3最小矩形解析の前に実行して、全タイルをキャッシュしておく。
If-Modified-Since を使って更新分のみダウンロードする。

依存: urllib.request (標準ライブラリのみ)
"""
import argparse
import configparser
import math
import os
import sys
import time
import threading
import queue
from urllib.request import urlopen, Request
from urllib.error import HTTPError, URLError

GSI_DEM5_URL  = "https://cyberjapandata.gsi.go.jp/xyz/dem5{dem}_png/15/{x}/{y}.png"
GSI_DEM10B_URL = "https://cyberjapandata.gsi.go.jp/xyz/dem10b_png/14/{x}/{y}.png"


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
    return os.path.join(tile_dir, str(z), str(x), f"{y}_{dem}.png")


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
        # 3×3最小矩形の計算（隣接が存在する方向のみ拡張）
        x_min0, x_max0, y_min0, y_max0 = mesh_to_tile_range(meshcode, 15)
        x_min, x_max, y_min, y_max = x_min0, x_max0, y_min0, y_max0

        for dlat in (-1, 0, 1):
            for dlon in (-1, 0, 1):
                if dlat == 0 and dlon == 0:
                    continue
                nb = mesh_neighbor(meshcode, dlat, dlon)
                if nb not in mesh_set:
                    continue
                nx0, nx1, ny0, ny1 = mesh_to_tile_range(nb, 15)
                x_min = min(x_min, nx0)
                x_max = max(x_max, nx1)
                y_min = min(y_min, ny0)
                y_max = max(y_max, ny1)

        # dem5 タイル (z=15, a→b→c)
        for ty in range(y_min, y_max + 1):
            for tx in range(x_min, x_max + 1):
                for dem in ("a", "b", "c"):
                    key = (15, tx, ty, dem)
                    if key not in seen:
                        seen.add(key)
                        path = tile_path(tile_dir, 15, tx, ty, dem)
                        jobs.append((15, tx, ty, dem, path))

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

    戻り値: ("ok", None) | ("304", None) | ("404", None) | ("skip", None) | ("err", msg)
    """
    url = tile_url(z, x, y, dem)
    headers = {"User-Agent": user_agent}

    # If-Modified-Since: キャッシュ済みならファイルの mtime を使う
    if os.path.exists(path):
        mtime = os.path.getmtime(path)
        # RFC 7231 形式
        import email.utils
        headers["If-Modified-Since"] = email.utils.formatdate(mtime, usegmt=True)

    backoff = backoff_initial
    for attempt in range(5):
        time.sleep(interval_ms / 1000.0)
        try:
            req = Request(url, headers=headers)
            with urlopen(req, timeout=30) as resp:
                data = resp.read()
                last_mod = resp.headers.get("Last-Modified")
                os.makedirs(os.path.dirname(path), exist_ok=True)
                with open(path, "wb") as f:
                    f.write(data)
                if last_mod:
                    import email.utils
                    try:
                        ts = email.utils.parsedate_to_datetime(last_mod).timestamp()
                        os.utime(path, (ts, ts))
                    except Exception:
                        pass
                return ("ok", None)

        except HTTPError as e:
            if e.code == 304:
                return ("304", None)
            if e.code == 404:
                return ("404", None)
            if e.code in (429, 503):
                retry_after = e.headers.get("Retry-After")
                wait = int(retry_after) if retry_after and retry_after.isdigit() else backoff
                time.sleep(wait)
                backoff = min(backoff * 2, 600)
                continue
            return ("err", f"HTTP {e.code}: {url}")

        except (URLError, OSError) as e:
            if attempt < 4:
                time.sleep(backoff)
                backoff = min(backoff * 2, 600)
                continue
            return ("err", f"{e}: {url}")

    return ("err", f"max retries: {url}")


# ---- ワーカースレッド ----

def worker(job_queue, results, user_agent, interval_ms, backoff_initial, lock, counters):
    while True:
        try:
            job = job_queue.get_nowait()
        except queue.Empty:
            break
        z, x, y, dem, path = job
        status, msg = fetch_one(z, x, y, dem, path, user_agent, interval_ms, backoff_initial)
        with lock:
            counters[status] = counters.get(status, 0) + 1
            if status == "err":
                print(f"  [ERR] {msg}", file=sys.stderr)
        job_queue.task_done()


# ---- メイン ----

def main():
    parser = argparse.ArgumentParser(description="国土地理院標高タイル事前取得スクリプト")
    parser.add_argument("--mesh-list", required=True, help="メッシュコードリストファイル")
    parser.add_argument("--config",    required=True, help="fetch_config.ini ファイル")
    parser.add_argument("--tile-dir",  required=True, help="タイルキャッシュディレクトリ")
    args = parser.parse_args()

    cfg = configparser.ConfigParser()
    cfg.read(args.config)
    email_addr    = cfg.get("fetch", "user_agent_email",  fallback="anonymous")
    max_parallel  = cfg.getint("fetch", "max_parallel",   fallback=4)
    interval_ms   = cfg.getint("fetch", "interval_ms",    fallback=100)
    backoff_init  = cfg.getint("fetch", "backoff_initial_sec", fallback=60)
    user_agent    = f"findsummits/1.0 (mailto:{email_addr})"

    print(f"User-Agent: {user_agent}")
    print(f"並列数: {max_parallel}  間隔: {interval_ms}ms  バックオフ初期: {backoff_init}s")

    mesh_set = load_mesh_set(args.mesh_list)
    print(f"メッシュ数: {len(mesh_set)}")

    jobs = enumerate_jobs(mesh_set, args.tile_dir)
    print(f"ジョブ数: {len(jobs)}")

    job_queue = queue.Queue()
    for job in jobs:
        job_queue.put(job)

    lock = threading.Lock()
    counters = {}

    threads = []
    for _ in range(max_parallel):
        t = threading.Thread(
            target=worker,
            args=(job_queue, None, user_agent, interval_ms, backoff_init, lock, counters),
            daemon=True,
        )
        t.start()
        threads.append(t)

    for t in threads:
        t.join()

    total = sum(counters.values())
    print(f"\n完了: 合計{total}件")
    for k in ("ok", "304", "404", "skip", "err"):
        if k in counters:
            label = {"ok": "取得", "304": "変更なし", "404": "存在なし",
                     "skip": "スキップ", "err": "エラー"}.get(k, k)
            print(f"  {label}: {counters[k]}")


if __name__ == "__main__":
    main()
