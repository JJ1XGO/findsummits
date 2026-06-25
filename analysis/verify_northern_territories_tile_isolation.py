#!/usr/bin/env python3
"""
北方領土タイル分離検証スクリプト (ISSUE-078)

ADR-URD-005「北方領土タイル単位除外で十分な精度が得られる理由」の前提検証:
  Z15 タイルレベルで 北方領土6村 と 北海道（歯舞群島含む全フィーチャ）が
  同一タイルを共有しないことを実証する。

実行:
  venv/bin/python3 analysis/verify_northern_territories_tile_isolation.py

出力:
  analysis/northern_territories_tile_isolation.txt  -- サマリー
  analysis/northern_territories_excluded_tiles.geojson  -- 北方領土タイル全体
  analysis/northern_territories_overlapping_tiles.geojson  -- 衝突タイル（あれば）
"""
import configparser
import json
import math
import os
import sys
from pathlib import Path

try:
    from shapely.geometry import box, mapping, shape
    from shapely.ops import unary_union
except ImportError:
    sys.exit("shapely が必要です: pip install shapely")

PROJECT_DIR = Path(__file__).parent.parent
CONFIG_PATH = PROJECT_DIR / "params/config.ini"


def load_data_dir():
    cfg = configparser.ConfigParser()
    if CONFIG_PATH.exists():
        cfg.read(CONFIG_PATH)
    if "DATA_DIR" in os.environ:
        return Path(os.environ["DATA_DIR"])
    if cfg.has_option("paths", "DATA_DIR"):
        return Path(cfg.get("paths", "DATA_DIR"))
    return Path("/data")


DATA_DIR = load_data_dir()
ANALYSIS_DIR = PROJECT_DIR / "analysis"

ZOOM = 15
# 実データ確認済みコード（ADR-URD-005 の 01696-01701 は誤り。正しくは 01695-01700）
# 01695: 色丹郡色丹村（色丹島）
# 01696: 国後郡泊村（国後島）
# 01697: 国後郡留夜別村（国後島）
# 01698: 択捉郡留別村（択捉島）
# 01699: 紗那郡紗那村（択捉島）
# 01700: 蘂取郡蘂取村（択捉島）
NORTHERN_CODES = {"01695", "01696", "01697", "01698", "01699", "01700"}


def latlon_to_tile(lat, lon, z):
    """緯度経度 → Z レベル タイル座標（src/mesh.c 移植）"""
    x = (lon + 180.0) / 360.0 * (2 ** z)
    sinlat = math.sin(math.radians(lat))
    y = (1.0 - math.log((sinlat + 1.0) / math.cos(math.radians(lat))) / math.pi) / 2.0 * (2 ** z)
    return int(x), int(y)


def tile_bbox(tx, ty, z):
    """タイル座標 → 地理 bbox (minx=lon_west, miny=lat_south, maxx=lon_east, maxy=lat_north)"""
    n = 2 ** z
    lon_west = tx / n * 360.0 - 180.0
    lon_east = (tx + 1) / n * 360.0 - 180.0
    lat_north = math.degrees(math.atan(math.sinh(math.pi * (1 - 2 * ty / n))))
    lat_south = math.degrees(math.atan(math.sinh(math.pi * (1 - 2 * (ty + 1) / n))))
    return lon_west, lat_south, lon_east, lat_north


def enumerate_tiles(polygon, z):
    """ポリゴンと交差する全 Z15 タイルの集合を返す"""
    minx, miny, maxx, maxy = polygon.bounds
    tx_min, ty_max = latlon_to_tile(miny, minx, z)
    tx_max, ty_min = latlon_to_tile(maxy, maxx, z)
    tiles = set()
    for tx in range(tx_min, tx_max + 1):
        for ty in range(ty_min, ty_max + 1):
            bx, by, bxx, byy = tile_bbox(tx, ty, z)
            tile_geom = box(bx, by, bxx, byy)
            if polygon.intersects(tile_geom):
                tiles.add((tx, ty))
    return tiles


def tiles_to_geojson(tiles, z):
    features = []
    for (tx, ty) in tiles:
        bx, by, bxx, byy = tile_bbox(tx, ty, z)
        features.append({
            "type": "Feature",
            "geometry": mapping(box(bx, by, bxx, byy)),
            "properties": {"tx": tx, "ty": ty, "z": z},
        })
    return {"type": "FeatureCollection", "features": features}


def main():
    n03_path = DATA_DIR / "ref/N03-20260101.geojson"
    if not n03_path.exists():
        sys.exit(f"N03 GeoJSON が見つかりません: {n03_path}")

    print(f"読み込み中: {n03_path}")
    with open(n03_path, encoding="utf-8") as f:
        data = json.load(f)

    features = data["features"]
    print(f"  総フィーチャ数: {len(features)}")

    north_geoms = []
    hokkaido_geoms = []
    north_actual = {}

    for feat in features:
        props = feat.get("properties") or {}
        pref = (props.get("N03_001") or "").strip()
        code = (props.get("N03_007") or "").strip()
        if code in NORTHERN_CODES:
            north_geoms.append(shape(feat["geometry"]))
            muni = (props.get("N03_004") or "").strip()
            gun  = (props.get("N03_003") or "").strip()
            north_actual[code] = f"{gun}{muni}"
        elif pref == "北海道":
            hokkaido_geoms.append(shape(feat["geometry"]))

    print(f"  北方領土フィーチャ: {len(north_geoms)} 件")
    print(f"  北海道フィーチャ（歯舞群島含む）: {len(hokkaido_geoms)} 件")

    # ADR の対応表とのズレを表示
    print("\n[参考] N03_007 コードの実データ対応（ADR-URD-005 の注釈用）:")
    for code, name in sorted(north_actual.items()):
        print(f"  {code} → {name}")

    print("\ndissolve 中...")
    north_poly   = unary_union(north_geoms)
    hokkaido_poly = unary_union(hokkaido_geoms)
    print("  完了")

    print(f"\nZ{ZOOM} タイル列挙中（北方領土）...")
    t_north = enumerate_tiles(north_poly, ZOOM)
    print(f"  北方領土タイル数: {len(t_north)}")

    print(f"Z{ZOOM} タイル列挙中（北海道）...")
    # 北方領土 bbox 内に絞って処理（高速化）
    minx, miny, maxx, maxy = north_poly.bounds
    # 北海道は北方領土から少し離れているので bbox を少し拡張して安全マージンを取る
    margin = 1.5  # degrees
    hokkaido_clipped = hokkaido_poly.intersection(
        box(minx - margin, miny - margin, maxx + margin, maxy + margin)
    )
    if hokkaido_clipped.is_empty:
        t_hokkaido = set()
        print("  北海道タイル（北方領土近傍）: 0 件（近傍に北海道フィーチャなし）")
    else:
        t_hokkaido = enumerate_tiles(hokkaido_clipped, ZOOM)
        print(f"  北海道タイル（北方領土 ±{margin}° 近傍）: {len(t_hokkaido)}")

    overlap = t_north & t_hokkaido
    print(f"\n衝突タイル数: {len(overlap)}")

    # 最近接距離
    dist_deg = north_poly.distance(hokkaido_poly)
    dist_km  = dist_deg * 111.0
    print(f"北方領土-北海道 最近接距離: {dist_deg:.5f}° ≈ {dist_km:.1f} km")

    # 結果サマリー
    lines = [
        f"=== 北方領土タイル分離検証 (Z{ZOOM}) ===",
        f"N03 ファイル  : {n03_path}",
        f"北方領土タイル数: {len(t_north)}",
        f"北海道タイル数（近傍 ±{margin}°）: {len(t_hokkaido)}",
        f"衝突タイル数  : {len(overlap)}",
        f"最近接距離    : {dist_deg:.5f}° ≈ {dist_km:.1f} km",
        "",
        "[実データ N03_007 コード対応]",
    ]
    for code, name in sorted(north_actual.items()):
        lines.append(f"  {code} → {name}")

    if len(overlap) == 0:
        verdict = "✅ 衝突なし: ADR-URD-005「タイル単位除外で本土を誤除外しない」の前提を実データで確認"
    else:
        verdict = f"❌ 衝突あり: {len(overlap)} タイルで北方領土と北海道が同一タイルに混在 → buffer 対策が必要"
    lines += ["", verdict]

    summary_path = ANALYSIS_DIR / "northern_territories_tile_isolation.txt"
    summary_path.write_text("\n".join(lines), encoding="utf-8")
    print(f"\n{verdict}")
    print(f"サマリー保存: {summary_path}")

    # 除外タイル GeoJSON
    excl_path = ANALYSIS_DIR / "northern_territories_excluded_tiles.geojson"
    excl_path.write_text(json.dumps(tiles_to_geojson(t_north, ZOOM), ensure_ascii=False), encoding="utf-8")
    print(f"除外タイル GeoJSON: {excl_path}  ({len(t_north)} tiles)")

    # 衝突タイル GeoJSON（あれば）
    if overlap:
        ov_path = ANALYSIS_DIR / "northern_territories_overlapping_tiles.geojson"
        ov_path.write_text(json.dumps(tiles_to_geojson(overlap, ZOOM), ensure_ascii=False), encoding="utf-8")
        print(f"衝突タイル GeoJSON: {ov_path}")


if __name__ == "__main__":
    main()
