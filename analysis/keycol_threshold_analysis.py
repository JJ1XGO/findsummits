"""
九州・四国 削除 6 件の minimax 鞍部標高を計算し、
AZ 内 / AZ 外 50m ゾーン内 / ゾーン外 を分類する。
"""

import csv
import heapq
import math
import os
import re

import numpy as np
from PIL import Image

Image.MAX_IMAGE_PIXELS = None  # 大きな結合 PNG を許可

TILES_DIR = "/data/findsummits4sotaja/tiles/"
TILE_SIZE = 256
ZOOM = 15

# 各ペアの座標 (xlsx と summitslist.csv から取得)
PAIRS = [
    {
        "del_id": "JA6/KG-157", "del_name": "権現ヶ尾",   "del_elev": 484,
        "del_lat": 31.5042,  "del_lon": 130.4668,
        "mrg_id": "JA6/KG-197", "mrg_name": "立神",       "mrg_elev": 478,
        "mrg_lat": 31.491968, "mrg_lon": 130.446744,
    },
    {
        "del_id": "JA6/KG-054", "del_name": "茶屋ヶ岡",   "del_elev": 565,
        "del_lat": 31.91,    "del_lon": 130.6062,
        "mrg_id": "JA6/KG-195", "mrg_name": "貝吹岡",     "mrg_elev": 568,
        "mrg_lat": 31.888207, "mrg_lon": 130.633681,
    },
    {
        "del_id": "JA6/KM-093", "del_name": "小金峰",     "del_elev": 1400,
        "del_lat": 32.5313,  "del_lon": 130.92329,
        "mrg_id": "JA6/KM-142", "mrg_name": "大金峰",     "mrg_elev": 1396,
        "mrg_lat": 32.551986, "mrg_lon": 130.910253,
    },
    {
        "del_id": "JA6/NS-067", "del_name": "三盛山",     "del_elev": 303,
        "del_lat": 32.8601,  "del_lon": 129.09129,
        "mrg_id": "JA6/NS-155", "mrg_name": "遠見番岳",   "mrg_elev": 308,
        "mrg_lat": 32.871532, "mrg_lon": 129.081588,
    },
    {
        "del_id": "JA6/FO-043", "del_name": "（名前なし）", "del_elev": 395,
        "del_lat": 33.7626,  "del_lon": 130.8466,
        "mrg_id": "JA6/FO-092", "mrg_name": "（ピーク名不明）", "mrg_elev": 396,
        "mrg_lat": 33.754424, "mrg_lon": 130.851932,
    },
    {
        "del_id": "JA5/TS-109", "del_name": "（名前なし）", "del_elev": 660,
        "del_lat": 34.1755,  "del_lon": 134.3717,
        "mrg_id": "JA5/TS-053", "mrg_name": "大山",       "mrg_elev": 691,
        "mrg_lat": 34.1637,  "mrg_lon": 134.3946,
    },
]


def latlon_to_pixel_float(lat, lon, xmin_tile, ymin_tile):
    """緯度経度 → PNG 内ピクセル座標 (float) に変換"""
    n = 2 ** ZOOM
    tx = (lon + 180) / 360 * n
    lat_rad = math.radians(lat)
    ty = (1 - math.log(math.tan(lat_rad) + 1 / math.cos(lat_rad)) / math.pi) / 2 * n
    px = (tx - xmin_tile) * TILE_SIZE
    py = (ty - ymin_tile) * TILE_SIZE
    return px, py


def decode_elevation(rgb_array):
    """RGB numpy array → 標高 (float) array。無効値は -9999。"""
    r = rgb_array[:, :, 0].astype(np.int32)
    g = rgb_array[:, :, 1].astype(np.int32)
    b = rgb_array[:, :, 2].astype(np.int32)
    raw = r * 65536 + g * 256 + b
    elev = np.where(raw < 2**23, raw / 100.0, (raw - 2**24) / 100.0)
    invalid = (r == 128) & (g == 0) & (b == 0)
    elev[invalid] = -9999.0
    return elev.astype(np.float32)


def find_tile_file(px1, py1, px2, py2):
    """両ピクセル座標（タイル整数単位）をカバーする PNG ファイルパスを返す"""
    pattern = re.compile(r"_15-(\d+)-(\d+)_15-(\d+)-(\d+)\.png$")
    tx1, ty1 = int(px1 // TILE_SIZE), int(py1 // TILE_SIZE)
    tx2, ty2 = int(px2 // TILE_SIZE), int(py2 // TILE_SIZE)
    best = None
    best_area = float("inf")
    for fname in os.listdir(TILES_DIR):
        m = pattern.search(fname)
        if not m:
            continue
        xmin, ymin, xmax, ymax = int(m.group(1)), int(m.group(2)), int(m.group(3)), int(m.group(4))
        if xmin <= min(tx1, tx2) and max(tx1, tx2) <= xmax and ymin <= min(ty1, ty2) and max(ty1, ty2) <= ymax:
            area = (xmax - xmin) * (ymax - ymin)
            if area < best_area:
                best_area = area
                best = (fname, xmin, ymin, xmax, ymax)
    return best


def minimax_col_elev(grid, start, end, margin_tiles=50):
    """
    Bottleneck Shortest Path (max-heap Dijkstra)。
    start/end の周辺 margin_tiles タイル分の領域で計算して高速化。
    戻り値: minimax 鞍部標高 (float)
    """
    rows, cols = grid.shape
    sr, sc = start  # (row=y, col=x)
    er, ec = end

    # 計算領域をクロップ（マージン付き）
    margin_px = margin_tiles * TILE_SIZE
    r0 = max(0, min(sr, er) - margin_px)
    r1 = min(rows, max(sr, er) + margin_px + 1)
    c0 = max(0, min(sc, ec) - margin_px)
    c1 = min(cols, max(sc, ec) + margin_px + 1)

    sub = grid[r0:r1, c0:c1]
    ss = (sr - r0, sc - c0)
    se = (er - r0, ec - c0)

    sub_rows, sub_cols = sub.shape
    dist = np.full((sub_rows, sub_cols), -np.inf, dtype=np.float32)
    dist[ss] = sub[ss]

    heap = [(-float(sub[ss]), ss[0], ss[1])]

    while heap:
        neg_d, r, c = heapq.heappop(heap)
        d = -neg_d

        if (r, c) == se:
            return d

        if d < dist[r, c]:
            continue

        for dr, dc in [(-1, 0), (1, 0), (0, -1), (0, 1),
                       (-1, -1), (-1, 1), (1, -1), (1, 1)]:
            nr, nc = r + dr, c + dc
            if 0 <= nr < sub_rows and 0 <= nc < sub_cols:
                elev = float(sub[nr, nc])
                if elev < -9000:
                    continue
                nd = min(d, elev)
                if nd > dist[nr, nc]:
                    dist[nr, nc] = nd
                    heapq.heappush(heap, (-nd, nr, nc))

    return float(dist[se])


def classify(delta, az_threshold=25.0, zone_threshold=50.0):
    if delta <= az_threshold:
        return "AZ内"
    elif delta <= zone_threshold:
        return f"AZ外{zone_threshold:.0f}mゾーン内"
    else:
        return "ゾーン外"


def main():
    results = []

    for p in PAIRS:
        print(f"\n--- {p['del_id']} → {p['mrg_id']} ---")

        # 座標 → タイル座標（整数）
        n = 2 ** ZOOM
        def to_tile_int(lat, lon, n=n):
            tx = int((lon + 180) / 360 * n)
            lat_rad = math.radians(lat)
            ty = int((1 - math.log(math.tan(lat_rad) + 1 / math.cos(lat_rad)) / math.pi) / 2 * n)
            return tx, ty

        dtx, dty = to_tile_int(p["del_lat"], p["del_lon"])
        mtx, mty = to_tile_int(p["mrg_lat"], p["mrg_lon"])

        found = find_tile_file(dtx * TILE_SIZE, dty * TILE_SIZE, mtx * TILE_SIZE, mty * TILE_SIZE)
        if not found:
            print("  ERROR: カバーする PNG が見つからない")
            results.append({**p, "col_H": None, "mrg_minus_H": None, "del_minus_H": None, "class": "ERROR"})
            continue

        fname, xmin, ymin, xmax, ymax = found
        fpath = os.path.join(TILES_DIR, fname)
        print(f"  PNG: {fname}")

        print("  PNG 読み込み中...", end="", flush=True)
        img = Image.open(fpath)
        arr = np.array(img)
        print(f" {arr.shape[1]}x{arr.shape[0]}px")

        grid = decode_elevation(arr)

        # 座標 → ピクセル座標
        dpx, dpy = latlon_to_pixel_float(p["del_lat"], p["del_lon"], xmin, ymin)
        mpx, mpy = latlon_to_pixel_float(p["mrg_lat"], p["mrg_lon"], xmin, ymin)
        drow, dcol = int(round(dpy)), int(round(dpx))
        mrow, mcol = int(round(mpy)), int(round(mpx))

        print(f"  削除側ピクセル: ({dcol}, {drow}), 統合先ピクセル: ({mcol}, {mrow})")
        print(f"  削除側標高(grid): {grid[drow, dcol]:.1f}m, 統合先標高(grid): {grid[mrow, mcol]:.1f}m")

        print("  minimax 鞍部標高計算中...", end="", flush=True)
        H = minimax_col_elev(grid, (drow, dcol), (mrow, mcol))
        print(f" H = {H:.1f}m")

        mrg_minus_H = p["mrg_elev"] - H
        del_minus_H = p["del_elev"] - H
        # 主指標: 高い方の標高 - H
        higher_elev = max(p["mrg_elev"], p["del_elev"])
        main_delta = higher_elev - H
        cls = classify(main_delta)

        print(f"  統合先({p['mrg_elev']}m) - H = {mrg_minus_H:.1f}m")
        print(f"  削除側({p['del_elev']}m) - H = {del_minus_H:.1f}m")
        print(f"  高い方({higher_elev}m) - H = {main_delta:.1f}m → [{cls}]")

        results.append({
            **p,
            "col_H": round(H, 1),
            "mrg_minus_H": round(mrg_minus_H, 1),
            "del_minus_H": round(del_minus_H, 1),
            "main_delta": round(main_delta, 1),
            "class": cls,
        })

    # CSV 出力
    out_csv = "/workspace/analysis/keycol_threshold_analysis.csv"
    fieldnames = [
        "del_id", "del_name", "del_elev",
        "mrg_id", "mrg_name", "mrg_elev",
        "col_H", "mrg_minus_H", "del_minus_H", "main_delta", "class",
    ]
    with open(out_csv, "w", newline="", encoding="utf-8") as f:
        w = csv.DictWriter(f, fieldnames=fieldnames, extrasaction="ignore")
        w.writeheader()
        w.writerows(results)

    print("\n=== サマリー ===")
    header = (
        f"{'削除ID':<16} {'統合先ID':<16} {'del_elev':>8} {'mrg_elev':>8}"
        f" {'col_H':>8} {'mrg-H':>7} {'del-H':>7} {'main_Δ':>8}  {'分類'}"
    )
    print(header)
    print("-" * 100)
    for r in results:
        if r["col_H"] is None:
            continue
        print(f"{r['del_id']:<16} {r['mrg_id']:<16} {r['del_elev']:>8} {r['mrg_elev']:>8} "
              f"{r['col_H']:>8.1f} {r['mrg_minus_H']:>7.1f} {r['del_minus_H']:>7.1f} "
              f"{r['main_delta']:>8.1f}  {r['class']}")

    classes = [r["class"] for r in results if r["col_H"] is not None]
    for label in ["AZ内", "AZ外50mゾーン内", "ゾーン外"]:
        print(f"  {label}: {classes.count(label)} 件")

    print(f"\nCSV 出力: {out_csv}")


if __name__ == "__main__":
    main()
