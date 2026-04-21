#!/usr/bin/env python3
"""
merge.py: 176CSV統合 + summitslist.csv突き合わせ

出力CSV の status 列:
  existing  - 現行SOTAサミットと一致したピーク
  new       - 新規候補（ZZ/ZZ-XXXダミーコード付与）
  deleted   - 現行SOTAサミットで未検出（削除候補）
"""
import argparse
import csv
import datetime
import math
from pathlib import Path

ZOOM = 15
TILE_PIX = 256
MIN_PROMINENCE = 150.0
DEFAULT_CSV_DIR = Path("/mnt/findsummits/results/csv")
DEFAULT_SUMMITSLIST = Path("/mnt/findsummits/ref/summitslist.csv")
DEFAULT_OUTPUT = Path("/mnt/findsummits/results/merged.csv")


def latlon_to_pixel(lat, lon):
    n = 2 ** ZOOM
    x = (lon + 180) / 360 * n * TILE_PIX
    lat_rad = math.radians(lat)
    y = (1 - math.log(math.tan(lat_rad) + 1 / math.cos(lat_rad)) / math.pi) / 2 * n * TILE_PIX
    return int(x), int(y)


def chebyshev(px1, py1, px2, py2):
    return max(abs(px1 - px2), abs(py1 - py2))


def load_peaks(csv_dir):
    peaks = []
    for csv_path in sorted(csv_dir.glob("*.csv")):
        with open(csv_path) as f:
            for row in csv.DictReader(f):
                lat = float(row["peak_lat"])
                lon = float(row["peak_lon"])
                px, py = latlon_to_pixel(lat, lon)
                peaks.append({
                    "peak_lat":   lat,
                    "peak_lon":   lon,
                    "peak_elev":  float(row["peak_elev"]),
                    "col_lat":    float(row["col_lat"]),
                    "col_lon":    float(row["col_lon"]),
                    "col_elev":   float(row["col_elev"]),
                    "prominence": float(row["prominence"]),
                    "is_tile_top": int(row["is_tile_top"]),
                    "px": px,
                    "py": py,
                })
    return peaks


def load_summits(summitslist_path):
    today = datetime.date.today()
    summits = []
    with open(summitslist_path) as f:
        f.readline()  # 1行目は日付ヘッダー
        for row in csv.DictReader(f):
            if not row["SummitCode"].startswith("JA"):
                continue
            valid_to = datetime.datetime.strptime(row["ValidTo"], "%d/%m/%Y").date()
            if valid_to < today:
                continue
            lat = float(row["Latitude"])
            lon = float(row["Longitude"])
            px, py = latlon_to_pixel(lat, lon)
            summits.append({
                "SummitCode": row["SummitCode"],
                "SummitName": row["SummitName"],
                "AltM":       int(row["AltM"]),
                "Latitude":   lat,
                "Longitude":  lon,
                "px": px,
                "py": py,
            })
    return summits


def match_peaks(peaks, summits, tolerance):
    matched = []       # (peak, summit)
    new_peaks = []     # peak のみ
    remaining = list(summits)

    for peak in peaks:
        best_summit = None
        best_dist = float("inf")
        for summit in remaining:
            dist = chebyshev(peak["px"], peak["py"], summit["px"], summit["py"])
            if dist <= tolerance and dist < best_dist:
                best_summit = summit
                best_dist = dist
        if best_summit is not None:
            matched.append((peak, best_summit))
            remaining.remove(best_summit)
        elif peak["prominence"] >= MIN_PROMINENCE:
            new_peaks.append(peak)

    deleted = remaining  # 突き合わせで残ったSOTAサミット = 未検出
    return matched, new_peaks, deleted


def build_rows(matched, new_peaks, deleted):
    rows = []

    for peak, summit in matched:
        rows.append({
            "status":      "existing",
            "summit_code": summit["SummitCode"],
            "summit_name": summit["SummitName"],
            "sota_alt_m":  summit["AltM"],
            "peak_lat":    peak["peak_lat"],
            "peak_lon":    peak["peak_lon"],
            "peak_elev":   peak["peak_elev"],
            "col_lat":     peak["col_lat"],
            "col_lon":     peak["col_lon"],
            "col_elev":    peak["col_elev"],
            "prominence":  peak["prominence"],
            "is_tile_top": peak["is_tile_top"],
        })

    new_peaks_sorted = sorted(new_peaks, key=lambda p: (p["px"], p["py"]))
    for i, peak in enumerate(new_peaks_sorted):
        rows.append({
            "status":      "new",
            "summit_code": f"ZZ/ZZ-{i:03d}",
            "summit_name": "",
            "sota_alt_m":  "",
            "peak_lat":    peak["peak_lat"],
            "peak_lon":    peak["peak_lon"],
            "peak_elev":   peak["peak_elev"],
            "col_lat":     peak["col_lat"],
            "col_lon":     peak["col_lon"],
            "col_elev":    peak["col_elev"],
            "prominence":  peak["prominence"],
            "is_tile_top": peak["is_tile_top"],
        })

    for summit in deleted:
        rows.append({
            "status":      "deleted",
            "summit_code": summit["SummitCode"],
            "summit_name": summit["SummitName"],
            "sota_alt_m":  summit["AltM"],
            "peak_lat":    summit["Latitude"],
            "peak_lon":    summit["Longitude"],
            "peak_elev":   "",
            "col_lat":     "",
            "col_lon":     "",
            "col_elev":    "",
            "prominence":  "",
            "is_tile_top": "",
        })

    return rows


FIELDNAMES = [
    "status", "summit_code", "summit_name", "sota_alt_m",
    "peak_lat", "peak_lon", "peak_elev",
    "col_lat", "col_lon", "col_elev", "prominence", "is_tile_top",
]


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--tolerance", type=int, default=0,
                        help="突き合わせ許容距離（ズーム15ピクセル、チェビシェフ距離）デフォルト: 0")
    parser.add_argument("--csv-dir",     type=Path, default=DEFAULT_CSV_DIR)
    parser.add_argument("--summitslist", type=Path, default=DEFAULT_SUMMITSLIST)
    parser.add_argument("--output",      type=Path, default=DEFAULT_OUTPUT)
    args = parser.parse_args()

    print(f"ピークCSVロード: {args.csv_dir}")
    peaks = load_peaks(args.csv_dir)
    print(f"  {len(peaks)} ピーク")

    print(f"summitslist.csvロード: {args.summitslist}")
    summits = load_summits(args.summitslist)
    print(f"  {len(summits)} サミット (JA・有効)")

    print(f"突き合わせ (tolerance={args.tolerance}px ≒ {args.tolerance * 4.8:.0f}m)")
    matched, new_peaks, deleted = match_peaks(peaks, summits, args.tolerance)
    print(f"  既存一致:       {len(matched)}")
    print(f"  新規候補:       {len(new_peaks)}")
    print(f"  未検出(削除候補): {len(deleted)}")

    rows = build_rows(matched, new_peaks, deleted)

    args.output.parent.mkdir(parents=True, exist_ok=True)
    with open(args.output, "w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=FIELDNAMES)
        writer.writeheader()
        writer.writerows(rows)

    print(f"出力: {args.output} ({len(rows)} 行)")


if __name__ == "__main__":
    main()
