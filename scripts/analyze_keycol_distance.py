#!/usr/bin/env python3
"""
Key Col 距離分布の実データ検証スクリプト。

2つのアプローチ:
 1. 既解析9メッシュCSVから Peak-Col 距離分布（下限の確認）
 2. SOTA JAリストから Peak→より高い近隣JAピーク距離（Key Col距離の上限代理）
"""

import argparse
import csv
import glob
import sys
from pathlib import Path

import numpy as np

R_EARTH_KM = 6371.0


def haversine_km(lat1, lon1, lat2, lon2):
    lat1 = np.radians(lat1)
    lat2 = np.radians(lat2)
    dlat = lat2 - lat1
    dlon = np.radians(lon2) - np.radians(lon1)
    a = np.sin(dlat / 2) ** 2 + np.cos(lat1) * np.cos(lat2) * np.sin(dlon / 2) ** 2
    return 2 * R_EARTH_KM * np.arcsin(np.sqrt(a))


def percentiles(values, pcts=(50, 90, 95, 99, 100)):
    if len(values) == 0:
        return {p: float("nan") for p in pcts}
    return {p: float(np.percentile(values, p)) for p in pcts}


def fmt_pct(d):
    return " ".join(f"p{p}={v:.2f}km" for p, v in d.items())


def load_mesh_csv(path):
    """解析済みCSVを読み込み、カラム別numpy配列のdictを返す"""
    peak_lat, peak_lon, col_lat, col_lon, prom, is_tile_top = [], [], [], [], [], []
    with open(path, newline="") as f:
        r = csv.DictReader(f)
        for row in r:
            peak_lat.append(float(row["peak_lat"]))
            peak_lon.append(float(row["peak_lon"]))
            col_lat.append(float(row["col_lat"]))
            col_lon.append(float(row["col_lon"]))
            prom.append(float(row["prominence"]))
            is_tile_top.append(int(row["is_tile_top"]))
    return {
        "peak_lat": np.array(peak_lat),
        "peak_lon": np.array(peak_lon),
        "col_lat": np.array(col_lat),
        "col_lon": np.array(col_lon),
        "prominence": np.array(prom),
        "is_tile_top": np.array(is_tile_top),
    }


def analyze_mesh_csvs(csv_dir: Path):
    files = sorted(glob.glob(str(csv_dir / "*.csv")))
    if not files:
        print(f"[WARN] no csv found in {csv_dir}", file=sys.stderr)
        return

    all_data = {k: [] for k in ["peak_lat", "peak_lon", "col_lat", "col_lon", "prominence", "is_tile_top", "source"]}
    per_source = {}
    for f in files:
        d = load_mesh_csv(f)
        src = Path(f).stem
        per_source[src] = d
        for k in d:
            all_data[k].append(d[k])
        all_data["source"].append(np.array([src] * len(d["peak_lat"])))

    arr = {k: np.concatenate(v) for k, v in all_data.items()}

    print("=" * 60)
    print("[Approach 1] 9-mesh analyzed CSV distribution")
    print("=" * 60)
    n = len(arr["peak_lat"])
    top = int(arr["is_tile_top"].sum())
    print(f"total peaks: {n}")
    print(f"  is_tile_top=0: {n - top} ({(1 - top / n) * 100:.1f}%)")
    print(f"  is_tile_top=1: {top} ({top / n * 100:.1f}%)  ← Key Col が解析範囲外")

    mask = arr["is_tile_top"] == 0
    d_inner = haversine_km(
        arr["peak_lat"][mask], arr["peak_lon"][mask],
        arr["col_lat"][mask], arr["col_lon"][mask],
    )
    print(f"\nPeak-Col distance (is_tile_top=0, n={len(d_inner)}):")
    print(f"  {fmt_pct(percentiles(d_inner))}")
    print("  ※ BORDER_TILES=8 (~10km) により上限打ち切り")

    print("\nper-source breakdown:")
    for src, d in per_source.items():
        nn = len(d["peak_lat"])
        top_rate = d["is_tile_top"].mean() * 100 if nn else 0
        m = d["is_tile_top"] == 0
        if m.sum() > 0:
            dd = haversine_km(
                d["peak_lat"][m], d["peak_lon"][m],
                d["col_lat"][m], d["col_lon"][m],
            )
            print(f"  {src}: n={nn:>4}, is_tile_top={top_rate:5.1f}%, "
                  f"dist_p99={np.percentile(dd, 99):5.2f}km, dist_max={dd.max():5.2f}km")
        else:
            print(f"  {src}: n={nn:>4}, is_tile_top={top_rate:5.1f}%")


def load_ja_summits(summitslist: Path):
    """summitslist.csv から JA サミットを抽出"""
    codes, names, alts, lats, lons = [], [], [], [], []
    with open(summitslist, newline="") as f:
        # 1行目: "SOTA Summits List (Date=...)" / 2行目: ヘッダ
        next(f)
        r = csv.DictReader(f)
        for row in r:
            code = row.get("SummitCode", "")
            if not code.startswith("JA/"):
                continue
            try:
                alt = float(row["AltM"])
                lat = float(row["Latitude"])
                lon = float(row["Longitude"])
            except (ValueError, KeyError):
                continue
            codes.append(code)
            names.append(row.get("SummitName", ""))
            alts.append(alt)
            lats.append(lat)
            lons.append(lon)
    return {
        "SummitCode": np.array(codes),
        "SummitName": np.array(names),
        "AltM": np.array(alts),
        "Latitude": np.array(lats),
        "Longitude": np.array(lons),
    }


def analyze_ja_parent_distance(ja):
    print()
    print("=" * 60)
    print("[Approach 2] JA summits: nearest higher peak distance")
    print("=" * 60)
    n = len(ja["SummitCode"])
    print(f"JA summits: {n}")

    lats, lons, alts = ja["Latitude"], ja["Longitude"], ja["AltM"]
    nearest_km = np.full(n, np.nan)
    nearest_idx = np.full(n, -1, dtype=int)
    for i in range(n):
        mask = alts > alts[i]
        if not np.any(mask):
            continue
        d = haversine_km(lats[i], lons[i], lats[mask], lons[mask])
        jmin = int(np.argmin(d))
        nearest_km[i] = d[jmin]
        nearest_idx[i] = int(np.where(mask)[0][jmin])

    valid = ~np.isnan(nearest_km)
    dists = nearest_km[valid]
    print(f"with higher peak: {len(dists)} (excluding highest)")
    print(f"  {fmt_pct(percentiles(dists))}")

    # 外れ値Top 15
    order = np.argsort(-nearest_km)
    print("\nTop 15 peaks with farthest higher neighbor (= longest possible Key Col distance):")
    print(f"  {'SummitCode':<14} {'AltM':>5} {'dist_km':>8}  SummitName")
    shown = 0
    for i in order:
        if np.isnan(nearest_km[i]):
            continue
        print(f"  {ja['SummitCode'][i]:<14} {int(ja['AltM'][i]):>5} {nearest_km[i]:>8.2f}  {ja['SummitName'][i]}")
        shown += 1
        if shown >= 15:
            break

    print("\ncoverage by buffer size:")
    for thresh in [10, 15, 18, 20, 25, 30, 40, 50, 75, 100]:
        pct = (dists <= thresh).mean() * 100
        miss = int((dists > thresh).sum())
        print(f"  buffer ≤ {thresh:>3}km : {pct:6.2f}%  (missed: {miss})")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--csv-dir", default="/mnt/findsummits/results/csv")
    ap.add_argument("--summitslist", default="/mnt/findsummits/ref/summitslist.csv")
    args = ap.parse_args()

    analyze_mesh_csvs(Path(args.csv_dir))
    ja = load_ja_summits(Path(args.summitslist))
    analyze_ja_parent_distance(ja)

    print()
    print("=" * 60)
    print("判定基準: オフセットグリッド方式 = 1/4メッシュ ≈ 18km バッファ")
    print("  99%ile ≤ 18km ⇒ オフセットグリッド方式で妥当")
    print("=" * 60)


if __name__ == "__main__":
    main()
