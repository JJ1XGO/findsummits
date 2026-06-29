#!/usr/bin/env python3
"""
output_geojson.py: merged_peak.csv から GeoJSON を生成する

フィーチャ構成:
  - Point: 各行の座標（match_status で色分け）
      matched → 緑 (#00AA00)
      new     → マゼンタ (#FF00FF)
      deleted → 灰色 (#888888)
  - LineString: matched 行のみ、検出ピーク → SOTA元座標（ずれ確認用）
"""
import argparse
import configparser
import csv
import datetime
import json
import os
from pathlib import Path

_PROJECT_DIR = Path(__file__).parent.parent
_MAIN_CONFIG_PATH = _PROJECT_DIR / "params/config.ini"

def _load_config():
    cfg = configparser.ConfigParser()
    if _MAIN_CONFIG_PATH.exists():
        cfg.read(_MAIN_CONFIG_PATH)
    return cfg

_config = _load_config()
if "DATA_DIR" not in os.environ and _config.has_option("paths", "DATA_DIR"):
    os.environ["DATA_DIR"] = _config.get("paths", "DATA_DIR")
_DATA_DIR = Path(os.environ.get("DATA_DIR", "/data"))

DEFAULT_INPUT  = _DATA_DIR / "results/merged.csv"
DEFAULT_OUTPUT = _DATA_DIR / "results/merged_peak.geojson"

GSI_ICON_URL = "https://maps.gsi.go.jp/portal/sys/v4/symbols/{}.png"
ICONS = {
    "matched": GSI_ICON_URL.format(827),  # シアン▲
    "new":     GSI_ICON_URL.format(825),  # オレンジ▲
    "deleted": GSI_ICON_URL.format(823),  # 灰▲
}


def row_to_features(row):
    features = []

    lat_s = row.get("peak_lat", "")
    lon_s = row.get("peak_lon", "")
    if not lat_s or not lon_s:
        return features

    lat = float(lat_s)
    lon = float(lon_s)
    ms       = row["match_status"]
    icon_url = ICONS.get(ms, ICONS["deleted"])

    elev_s = row.get("peak_elev", "")
    prom_s = row.get("prominence", "")
    name_elev = f"{elev_s}m" if elev_s else "?m"
    name_prom = f"prom={prom_s}m" if prom_s else ""

    if ms == "matched":
        label = f"{row['summit_code']} {row['summit_name']} ({name_elev}, {name_prom})"
    elif ms == "new":
        label = f"NEW {row['summit_code']} ({name_elev}, {name_prom})"
    else:
        label = f"DELETED {row['summit_code']} {row['summit_name']}"

    props = {
        "name":           label,
        "match_status":   ms,
        "stability":      row.get("stability", ""),
        "summit_code":    row.get("summit_code", ""),
        "summit_name":    row.get("summit_name", ""),
        "sota_alt_m":     row.get("sota_alt_m", ""),
        "peak_elev":      elev_s,
        "prominence":     prom_s,
        "col_elev":       row.get("col_elev", ""),
        "is_tile_top":    row.get("is_tile_top", ""),
        "analysis_count": row.get("analysis_count", ""),
        "expected_count": row.get("expected_count", ""),
        # 地理院地図 Icon方式
        "_markerType":  "Icon",
        "_iconUrl":     icon_url,
        "_iconSize":    [24, 24],
        "_iconAnchor":  [12, 12],
    }

    features.append({
        "type": "Feature",
        "geometry": {"type": "Point", "coordinates": [lon, lat]},
        "properties": props,
    })

    # matched のみ: 検出ピーク → SOTA元座標 の LineString
    if ms == "matched":
        orig_lat_s = row.get("orig_lat", "")
        orig_lon_s = row.get("orig_lon", "")
        if orig_lat_s and orig_lon_s:
            orig_lat = float(orig_lat_s)
            orig_lon = float(orig_lon_s)
            if orig_lat != lat or orig_lon != lon:
                features.append({
                    "type": "Feature",
                    "geometry": {
                        "type": "LineString",
                        "coordinates": [[lon, lat], [orig_lon, orig_lat]],
                    },
                    "properties": {
                        "name":         f"{row['summit_code']} ずれ",
                        "summit_code":  row["summit_code"],
                        "_color":       "#FFA500",
                        "_weight":      2,
                        "stroke":       "#FFA500",
                        "stroke-width": 2,
                    },
                })

    return features


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--input",  type=Path, default=DEFAULT_INPUT)
    parser.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    args = parser.parse_args()

    log_dir = _DATA_DIR / "logs"
    log_dir.mkdir(parents=True, exist_ok=True)
    log_ts   = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
    log_path = log_dir / f"output_geojson_{log_ts}.log"
    log_file = open(log_path, "w")

    def log(msg=""):
        print(msg)
        print(msg, file=log_file)

    start_time = datetime.datetime.now()
    log(f"output_geojson.py 開始: {start_time.strftime('%Y-%m-%d %H:%M:%S')}")
    log(f"入力: {args.input}")

    features = []
    counts = {"matched": 0, "new": 0, "deleted": 0}

    with open(args.input) as f:
        rows = (line for line in f if not line.startswith("#"))
        for row in csv.DictReader(rows):
            ms = row.get("match_status", "")
            if ms in counts:
                counts[ms] += 1
            features.extend(row_to_features(row))

    geojson = {
        "type": "FeatureCollection",
        "metadata": {
            "attribution": "地理院タイル（標高タイル）を加工して作成。出典: 国土地理院",
            "source_url": "https://maps.gsi.go.jp/development/ichiran.html",
            "license_url": "https://www.gsi.go.jp/kikakuchousei/kikakuchousei40182.html",
        },
        "features": features,
    }

    args.output.parent.mkdir(parents=True, exist_ok=True)
    with open(args.output, "w", encoding="utf-8") as f:
        json.dump(geojson, f, ensure_ascii=False)

    end_time = datetime.datetime.now()
    elapsed  = (end_time - start_time).total_seconds()

    log()
    log("=== 結果サマリー ===")
    log(f"  matched: {counts['matched']}件")
    log(f"  new:     {counts['new']}件")
    log(f"  deleted: {counts['deleted']}件")
    log(f"  総フィーチャ数: {len(features)}")
    log(f"出力: {args.output}")
    log(f"output_geojson.py 終了: {end_time.strftime('%Y-%m-%d %H:%M:%S')} (所要時間: {elapsed:.1f}秒)")
    log_file.close()


if __name__ == "__main__":
    main()
