#!/usr/bin/env python3
"""
merge.py: 解析CSV統合 + summitslist.csv突き合わせ

出力CSV の列:
  match_status - SOTAリストとの突合結果
    matched  - 現行SOTAサミットと一致したピーク
    new      - 新規候補（ZZ/ZZ-XXXダミーコード付与）
    deleted  - 現行SOTAサミットで未検出（削除候補）
  stability - 解析品質
    confirmed - 確定ピーク（全解析で is_tile_top=0、解析回数=期待値）
    unstable  - 不安定ピーク（is_tile_top=1 が含まれる、または解析回数不一致）
    -         - 削除候補（解析結果なし）
"""
import argparse
import configparser
import csv
import datetime
import math
import os
from pathlib import Path


def _load_dotenv():
    env_file = Path(__file__).parent.parent / ".env"
    if not env_file.exists():
        return
    for line in env_file.read_text().splitlines():
        line = line.strip()
        if not line or line.startswith("#") or "=" not in line:
            continue
        key, _, val = line.partition("=")
        key = key.strip()
        val = val.split("#")[0].strip()
        if key and key not in os.environ:
            os.environ[key] = val

_load_dotenv()
_DATA_DIR = Path(os.environ.get("DATA_DIR", "/data"))
_PROJECT_DIR = Path(__file__).parent.parent
_CONFIG_PATH = _PROJECT_DIR / "params/fetch_config.ini"

def _load_config():
    cfg = configparser.ConfigParser()
    if _CONFIG_PATH.exists():
        cfg.read(_CONFIG_PATH)
    return cfg

_config = _load_config()

ZOOM = 15
TILE_PIX = 256
MIN_PROMINENCE = 150.0
DEFAULT_TOLERANCE = _config.getint("merge", "tolerance_px", fallback=0)
DEFAULT_CSV_DIR = _DATA_DIR / "results/csv"
DEFAULT_SUMMITSLIST = _PROJECT_DIR / "ref/summitslist.csv"
DEFAULT_OUTPUT = _DATA_DIR / "results/merged.csv"


def latlon_to_pixel(lat, lon):
    n = 2 ** ZOOM
    x = (lon + 180) / 360 * n * TILE_PIX
    lat_rad = math.radians(lat)
    y = (1 - math.log(math.tan(lat_rad) + 1 / math.cos(lat_rad)) / math.pi) / 2 * n * TILE_PIX
    return int(x), int(y)


def chebyshev(px1, py1, px2, py2):
    return max(abs(px1 - px2), abs(py1 - py2))


def mesh_neighbor(code, dlat, dlon):
    return (code // 100 + dlat) * 100 + (code % 100 + dlon)


def mesh_bbox(mesh_set):
    """メッシュセットから地理的bbox (lat_min, lat_max, lon_min, lon_max) を計算"""
    lat_min = min(c // 100 for c in mesh_set) * (2 / 3)
    lat_max = (max(c // 100 for c in mesh_set) + 1) * (2 / 3)
    lon_min = min(c % 100 for c in mesh_set) + 100
    lon_max = max(c % 100 for c in mesh_set) + 1 + 100
    return lat_min, lat_max, lon_min, lon_max


def load_mesh_set(path):
    if path is None:
        return None
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


def expected_count(center_mesh, mesh_set):
    """解析に含まれるはずの回数を計算する（center_mesh 自身 + 隣接メッシュ数）"""
    if mesh_set is None:
        return None
    count = 1
    for dlat in (-1, 0, 1):
        for dlon in (-1, 0, 1):
            if dlat == 0 and dlon == 0:
                continue
            nb = mesh_neighbor(center_mesh, dlat, dlon)
            if nb in mesh_set:
                count += 1
    return count


def load_peaks(csv_dir):
    peaks = []
    for csv_path in sorted(csv_dir.glob("*.csv")):
        with open(csv_path) as f:
            for row in csv.DictReader(f):
                lat = float(row["peak_lat"])
                lon = float(row["peak_lon"])
                px, py = latlon_to_pixel(lat, lon)
                peaks.append({
                    "peak_lat":     lat,
                    "peak_lon":     lon,
                    "peak_elev":    float(row["peak_elev"]),
                    "col_lat":      float(row["col_lat"]),
                    "col_lon":      float(row["col_lon"]),
                    "col_elev":     float(row["col_elev"]),
                    "prominence":   float(row["prominence"]),
                    "is_tile_top":  int(row["is_tile_top"]),
                    "col_margin_px": int(row.get("col_margin_px", -1)),
                    "center_mesh":  int(row.get("center_mesh", 0)),
                    "px": px,
                    "py": py,
                })
    return peaks


def merge_duplicates(peaks, mesh_set):
    """
    同一 (px, py) を持つレコードを統合する。

    採用規則:
      - col_elev が最小のレコードを代表に採用（保守的評価）
      - is_tile_top のいずれかが 1、または解析回数 != 期待値 → unstable
    """
    groups = {}
    for p in peaks:
        key = (p["px"], p["py"])
        if key not in groups:
            groups[key] = []
        groups[key].append(p)

    merged = []
    for key, group in groups.items():
        # col_elev 最小のレコードを代表にする（is_tile_top=1 は col_elev=-9999 なので最小になりがち）
        # is_tile_top=0 のものを優先するため、(is_tile_top, col_elev) でソートして最小を選ぶ
        def sort_key(r):
            return (r["is_tile_top"], r["col_elev"] if r["col_elev"] > -9998 else float("inf"))
        best = min(group, key=sort_key)

        analysis_count = len(group)
        ec = expected_count(best["center_mesh"], mesh_set)

        any_tile_top = any(r["is_tile_top"] for r in group)
        if any_tile_top or (ec is not None and analysis_count != ec):
            stability = "unstable"
        else:
            stability = "confirmed"

        merged.append({
            **best,
            "stability":       stability,
            "analysis_count":  analysis_count,
            "expected_count":  ec if ec is not None else "",
        })

    return merged


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
    """
    解析ピークと既存SOTAサミットを突き合わせる。

    重複判定は解析結果間では exact match (tolerance=0 固定)。
    ここでの tolerance は SOTAリスト突合のみに使用する。
    """
    matched = []
    new_peaks = []
    remaining = list(summits)

    for peak in peaks:
        if peak["prominence"] < MIN_PROMINENCE:
            continue
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
        else:
            new_peaks.append(peak)

    deleted = remaining
    return matched, new_peaks, deleted


def build_rows(matched, new_peaks, deleted):
    rows = []

    for peak, summit in matched:
        rows.append({
            "match_status":    "matched",
            "stability":       peak["stability"],
            "summit_code":     summit["SummitCode"],
            "summit_name":     summit["SummitName"],
            "sota_alt_m":      summit["AltM"],
            "peak_lat":        peak["peak_lat"],
            "peak_lon":        peak["peak_lon"],
            "peak_elev":       peak["peak_elev"],
            "col_lat":         peak["col_lat"],
            "col_lon":         peak["col_lon"],
            "col_elev":        peak["col_elev"],
            "prominence":      peak["prominence"],
            "is_tile_top":     peak["is_tile_top"],
            "col_margin_px":   peak["col_margin_px"],
            "analysis_count":  peak["analysis_count"],
            "expected_count":  peak["expected_count"],
        })

    new_peaks_sorted = sorted(new_peaks, key=lambda p: (p["px"], p["py"]))
    for i, peak in enumerate(new_peaks_sorted):
        rows.append({
            "match_status":    "new",
            "stability":       peak["stability"],
            "summit_code":     f"ZZ/ZZ-{i:03d}",
            "summit_name":     "",
            "sota_alt_m":      "",
            "peak_lat":        peak["peak_lat"],
            "peak_lon":        peak["peak_lon"],
            "peak_elev":       peak["peak_elev"],
            "col_lat":         peak["col_lat"],
            "col_lon":         peak["col_lon"],
            "col_elev":        peak["col_elev"],
            "prominence":      peak["prominence"],
            "is_tile_top":     peak["is_tile_top"],
            "col_margin_px":   peak["col_margin_px"],
            "analysis_count":  peak["analysis_count"],
            "expected_count":  peak["expected_count"],
        })

    for summit in deleted:
        rows.append({
            "match_status":    "deleted",
            "stability":       "-",
            "summit_code":     summit["SummitCode"],
            "summit_name":     summit["SummitName"],
            "sota_alt_m":      summit["AltM"],
            "peak_lat":        summit["Latitude"],
            "peak_lon":        summit["Longitude"],
            "peak_elev":       "",
            "col_lat":         "",
            "col_lon":         "",
            "col_elev":        "",
            "prominence":      "",
            "is_tile_top":     "",
            "col_margin_px":   "",
            "analysis_count":  "",
            "expected_count":  "",
        })

    return rows


FIELDNAMES = [
    "match_status", "stability", "summit_code", "summit_name", "sota_alt_m",
    "peak_lat", "peak_lon", "peak_elev",
    "col_lat", "col_lon", "col_elev",
    "prominence", "is_tile_top", "col_margin_px",
    "analysis_count", "expected_count",
]


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--tolerance",   type=int, default=DEFAULT_TOLERANCE,
                        help=f"SOTAリスト突合の許容距離（ズーム15ピクセル、チェビシェフ距離）デフォルト: {DEFAULT_TOLERANCE} (params/fetch_config.ini の merge.tolerance_px)")
    parser.add_argument("--mesh-list",   type=Path, default=None,
                        help="メッシュコードリスト（期待解析回数計算に使用）")
    parser.add_argument("--csv-dir",     type=Path, default=DEFAULT_CSV_DIR)
    parser.add_argument("--summitslist", type=Path, default=DEFAULT_SUMMITSLIST)
    parser.add_argument("--output",      type=Path, default=DEFAULT_OUTPUT)
    args = parser.parse_args()

    mesh_set = load_mesh_set(args.mesh_list)
    if mesh_set:
        print(f"メッシュリスト: {args.mesh_list} ({len(mesh_set)} 件)")

    print(f"ピークCSVロード: {args.csv_dir}")
    raw_peaks = load_peaks(args.csv_dir)
    print(f"  {len(raw_peaks)} レコード（重複含む）")

    peaks = merge_duplicates(raw_peaks, mesh_set)
    confirmed = sum(1 for p in peaks if p["stability"] == "confirmed")
    unstable  = sum(1 for p in peaks if p["stability"] == "unstable")
    print(f"  重複マージ後: {len(peaks)} ピーク（確定:{confirmed} 不安定:{unstable}）")

    print(f"summitslist.csvロード: {args.summitslist}")
    summits = load_summits(args.summitslist)
    print(f"  {len(summits)} サミット (JA・有効)")
    if mesh_set:
        lat_min, lat_max, lon_min, lon_max = mesh_bbox(mesh_set)
        summits = [s for s in summits
                   if lat_min <= s["Latitude"] <= lat_max
                   and lon_min <= s["Longitude"] <= lon_max]
        print(f"  → bboxフィルタ後: {len(summits)} サミット "
              f"(lat {lat_min:.3f}–{lat_max:.3f}, lon {lon_min:.3f}–{lon_max:.3f})")

    print(f"突き合わせ (tolerance={args.tolerance}px ≒ {args.tolerance * 4.8:.0f}m)")
    matched, new_peaks, deleted = match_peaks(peaks, summits, args.tolerance)
    print(f"  既存一致:         {len(matched)}")
    print(f"  新規候補:         {len(new_peaks)}")
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
