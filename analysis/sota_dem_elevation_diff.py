#!/usr/bin/env python3
"""
sota_dem_elevation_diff.py - SOTA サミット登録標高と DEM 標高の差分分析

SOTA summitslist.csv の JA/* サミット全件について、
登録標高(AltM) と DEM タイル由来の標高を比較し、
delete_zone_max_drop の推奨値を算出する。

出力:
  analysis/sota_dem_elevation_diff.csv  - サミット別の差分データ
  stdout                                - 統計サマリ

使い方:
  venv/bin/python3 analysis/sota_dem_elevation_diff.py [--sample N]

  --sample N : JA サミットの先頭 N 件のみ処理（動作確認用）
"""
import argparse
import configparser
import csv
import math
import os
import sys
from pathlib import Path

from PIL import Image

# ---- 定数 ----

ZOOM_DEM5 = 15
ZOOM_DEM10 = 14
TILE_SIZE = 256
DEM_TYPES_5 = ["a", "b", "c"]


# ---- 座標変換 ----

def latlon_to_tile(lat: float, lon: float, z: int) -> tuple[int, int]:
    n = 2 ** z
    tx = int((lon + 180.0) / 360.0 * n)
    sin_lat = math.sin(math.radians(lat))
    ty = int((1.0 - math.log((sin_lat + 1.0) / math.cos(math.radians(lat))) / math.pi) / 2.0 * n)
    return tx, ty


def latlon_to_tile_px(lat: float, lon: float, z: int) -> tuple[int, int, int, int]:
    """(tile_x, tile_y, px_within_tile_x, px_within_tile_y) を返す"""
    n = 2 ** z
    x_float = (lon + 180.0) / 360.0 * n
    sin_lat = math.sin(math.radians(lat))
    y_float = (1.0 - math.log((sin_lat + 1.0) / math.cos(math.radians(lat))) / math.pi) / 2.0 * n

    tx = int(x_float)
    ty = int(y_float)
    px = int((x_float - tx) * TILE_SIZE)
    py = int((y_float - ty) * TILE_SIZE)

    # 境界クランプ（浮動小数点誤差対策）
    px = max(0, min(TILE_SIZE - 1, px))
    py = max(0, min(TILE_SIZE - 1, py))

    return tx, ty, px, py


# ---- PNG デコード ----

def tile_path(tile_dir: str, z: int, tx: int, ty: int, dem: str) -> str:
    return os.path.join(tile_dir, str(z), str(tx), f"{ty}_{dem}.png")


def decode_pixel(img: Image.Image, px: int, py: int) -> float:
    """PNG のピクセル (px, py) から標高 (m) を返す。NODATA なら -9999.0"""
    r, g, b = img.getpixel((px, py))[:3]
    if r == 128 and g == 0 and b == 0:
        return -9999.0
    return (r * 65536 + g * 256 + b) / 100.0


def load_tile(path: str) -> Image.Image | None:
    """タイル PNG を開く。存在しない場合は None を返す"""
    if not os.path.exists(path):
        return None
    try:
        return Image.open(path).convert("RGB")
    except Exception:
        return None


# ---- DEM 標高取得 ----

def get_dem_elevation(lat: float, lon: float, tile_dir: str) -> tuple[float, str]:
    """
    DEM タイルから標高を取得する。

    フォールバック順: DEM5a → DEM5b → DEM5c → DEM10b
    戻り値: (標高 m, dem_source)
    dem_source: "5a" / "5b" / "5c" / "10b" / "NODATA"
    """
    # DEM5 (z=15)
    tx15, ty15, px15, py15 = latlon_to_tile_px(lat, lon, ZOOM_DEM5)
    for dem in DEM_TYPES_5:
        path = tile_path(tile_dir, ZOOM_DEM5, tx15, ty15, dem)
        img = load_tile(path)
        if img is None:
            continue
        elev = decode_pixel(img, px15, py15)
        if elev != -9999.0:
            return elev, f"5{dem}"

    # DEM10b (z=14)
    tx14, ty14, px14, py14 = latlon_to_tile_px(lat, lon, ZOOM_DEM10)
    path = tile_path(tile_dir, ZOOM_DEM10, tx14, ty14, "b")
    img = load_tile(path)
    if img is not None:
        elev = decode_pixel(img, px14, py14)
        if elev != -9999.0:
            return elev, "10b"

    return -9999.0, "NODATA"


# ---- 推奨値計算 ----

def calc_recommended_drop(max_abs_diff: float) -> int:
    """
    delete_zone_max_drop 推奨値 = 150 + max_abs_diff を 50m 単位切り上げ

    例: max_abs_diff=47.3 → 150 + 50 = 200
        max_abs_diff=50.0 → 150 + 50 = 200
        max_abs_diff=50.1 → 150 + 100 = 250
    """
    margin = math.ceil(max_abs_diff / 50) * 50
    return 150 + margin


# ---- メイン ----

def main():
    parser = argparse.ArgumentParser(description="SOTA サミット登録標高と DEM 標高の差分分析")
    parser.add_argument("--sample", type=int, default=None,
                        help="JA サミットの先頭 N 件のみ処理（動作確認用）")
    parser.add_argument("--region", default=None,
                        help="特定地域コードのみ処理（例: CB）。動作確認用")
    args = parser.parse_args()

    # config.ini 読み込み
    repo_root = Path(__file__).parent.parent
    config_path = repo_root / "params" / "config.ini"
    if not config_path.exists():
        print(f"エラー: {config_path} が見つかりません。config.ini.example をコピーして設定してください。", file=sys.stderr)
        sys.exit(1)

    config = configparser.ConfigParser()
    config.read(config_path)
    data_dir = config["paths"]["DATA_DIR"]
    tile_dir = os.path.join(data_dir, "tiles")

    if not os.path.isdir(tile_dir):
        print(f"エラー: タイルディレクトリが見つかりません: {tile_dir}", file=sys.stderr)
        sys.exit(1)

    # summitslist.csv 読み込み（JA/* のみ）
    # 1行目はメタデータ ("SOTA Summits List (Date=...)")、2行目からヘッダ
    summits_path = repo_root / "ref" / "summitslist.csv"
    ja_summits = []
    with open(summits_path, newline="", encoding="utf-8") as f:
        next(f)  # メタデータ行をスキップ
        reader = csv.DictReader(f, skipinitialspace=True)
        for row in reader:
            code = row["SummitCode"].strip()
            if not code.startswith("JA/"):
                continue
            if args.region and not code.startswith(f"JA/{args.region}"):
                continue
            try:
                lat = float(row["Latitude"])
                lon = float(row["Longitude"])
                alt_m = float(row["AltM"])
            except (ValueError, KeyError):
                continue
            ja_summits.append({
                "SummitCode": code,
                "SummitName": row.get("SummitName", "").strip(),
                "Lat": lat,
                "Lon": lon,
                "AltM": alt_m,
            })

    if args.sample is not None:
        ja_summits = ja_summits[:args.sample]

    total = len(ja_summits)
    print(f"対象: {total} 件の JA サミット", file=sys.stderr)

    # 各サミットの DEM 標高を取得
    results = []
    nodata_count = 0
    for i, s in enumerate(ja_summits, 1):
        if i % 100 == 0 or i == total:
            print(f"  処理中: {i}/{total}", file=sys.stderr)

        dem_elev, dem_source = get_dem_elevation(s["Lat"], s["Lon"], tile_dir)

        if dem_elev == -9999.0:
            nodata_count += 1
            abs_diff = None
        else:
            abs_diff = abs(s["AltM"] - dem_elev)

        results.append({
            "SummitCode": s["SummitCode"],
            "SummitName": s["SummitName"],
            "Lat": s["Lat"],
            "Lon": s["Lon"],
            "AltM": s["AltM"],
            "DEM_AltM": dem_elev if dem_elev != -9999.0 else "",
            "abs_diff": abs_diff if abs_diff is not None else "",
            "dem_source": dem_source,
        })

    # CSV 出力
    out_path = repo_root / "analysis" / "sota_dem_elevation_diff.csv"
    with open(out_path, "w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=["SummitCode", "SummitName", "Lat", "Lon",
                                                "AltM", "DEM_AltM", "abs_diff", "dem_source"])
        writer.writeheader()
        writer.writerows(results)

    print(f"\nCSV 出力: {out_path}", file=sys.stderr)

    # 統計サマリ
    valid = [r for r in results if r["abs_diff"] != ""]
    diffs = sorted([r["abs_diff"] for r in valid])

    print("\n" + "=" * 60)
    print("SOTA サミット登録標高 vs DEM 標高  差分統計")
    print("=" * 60)
    print(f"総サミット数     : {total:>6}")
    print(f"有効（DEM取得済）: {len(valid):>6}")
    print(f"NODATA           : {nodata_count:>6}")

    if diffs:
        n = len(diffs)
        mean_diff = sum(diffs) / n
        median_diff = diffs[n // 2] if n % 2 == 1 else (diffs[n // 2 - 1] + diffs[n // 2]) / 2
        max_diff = diffs[-1]
        p95 = diffs[int(n * 0.95)]
        p99 = diffs[min(int(n * 0.99), n - 1)]

        print(f"\n  平均         : {mean_diff:>8.2f} m")
        print(f"  中央値       : {median_diff:>8.2f} m")
        print(f"  95 パーセンタイル: {p95:>8.2f} m")
        print(f"  99 パーセンタイル: {p99:>8.2f} m")
        print(f"  最大         : {max_diff:>8.2f} m")

        recommended = calc_recommended_drop(max_diff)
        print(f"\n推奨 delete_zone_max_drop: {recommended} m")
        print(f"  = 150 (SOTA閾値) + {math.ceil(max_diff / 50) * 50} m")
        print(f"    (max_abs_diff={max_diff:.2f} m を 50m 単位切り上げ)")

        # 差分上位 20 件
        top20 = sorted(valid, key=lambda r: r["abs_diff"], reverse=True)[:20]
        print("\n--- 差分上位 20 件 ---")
        print(f"{'SummitCode':<16} {'SummitName':<24} {'AltM':>6} {'DEM':>6} {'|diff|':>7}  src")
        print("-" * 68)
        for r in top20:
            name = r["SummitName"][:22]
            print(f"{r['SummitCode']:<16} {name:<24} {r['AltM']:>6.0f} {r['DEM_AltM']:>6.1f} {r['abs_diff']:>7.2f}  {r['dem_source']}")

    print("=" * 60)


if __name__ == "__main__":
    main()
