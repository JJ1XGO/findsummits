#!/usr/bin/env python3
"""
preprocess_pref_boundaries.py: 国土数値情報 N03 行政区域 GeoJSON を
都道府県/振興局レベルに dissolve して軽量化する。

入力:  $DATA_DIR/ref/N03-2026.geojson  (国土数値情報 全国行政区域 GeoJSON, 約 765MB)
出力:  $DATA_DIR/ref/N03-2026_regions.geojson  (都道府県/振興局単位に dissolve した軽量版)

出力フィーチャのプロパティ:
  assoc       - SOTA エリアコード接頭辞 (JA / JA5 / JA6 / JA8)
  area_code   - SOTA エリアコード (例: TK, IS)
  region_name - 地域名 (都道府県名 or 振興局名)
"""
import argparse
import configparser
import json
import os
from collections import defaultdict
from pathlib import Path

try:
    from shapely.geometry import mapping, shape
    from shapely.ops import unary_union
except ImportError:
    raise SystemExit("shapely が必要です: pip install shapely") from None

_PROJECT_DIR = Path(__file__).parent.parent
_MAIN_CONFIG_PATH = _PROJECT_DIR / "params/config.ini"


def _load_data_dir():
    cfg = configparser.ConfigParser()
    if _MAIN_CONFIG_PATH.exists():
        cfg.read(_MAIN_CONFIG_PATH)
    if "DATA_DIR" in os.environ:
        return Path(os.environ["DATA_DIR"])
    if cfg.has_option("paths", "DATA_DIR"):
        return Path(cfg.get("paths", "DATA_DIR"))
    return Path("/data")


_DATA_DIR = _load_data_dir()

# 都道府県名 → (assoc, area_code)
_PREF_MAP = {
    # 本州
    "愛知県": ("JA", "AC"),
    "青森県": ("JA", "AM"),
    "秋田県": ("JA", "AT"),
    "千葉県": ("JA", "CB"),
    "福井県": ("JA", "FI"),
    "福島県": ("JA", "FS"),
    "岐阜県": ("JA", "GF"),
    "群馬県": ("JA", "GM"),
    "兵庫県": ("JA", "HG"),
    "広島県": ("JA", "HS"),
    "茨城県": ("JA", "IB"),
    "石川県": ("JA", "IK"),
    "岩手県": ("JA", "IT"),
    "神奈川県": ("JA", "KN"),
    "京都府": ("JA", "KT"),
    "三重県": ("JA", "ME"),
    "宮城県": ("JA", "MG"),
    "新潟県": ("JA", "NI"),
    "長野県": ("JA", "NN"),
    "奈良県": ("JA", "NR"),
    "大阪府": ("JA", "OS"),
    "岡山県": ("JA", "OY"),
    "滋賀県": ("JA", "SI"),
    "島根県": ("JA", "SN"),
    "静岡県": ("JA", "SO"),
    "埼玉県": ("JA", "ST"),
    "栃木県": ("JA", "TG"),
    "東京都": ("JA", "TK"),
    "鳥取県": ("JA", "TT"),
    "富山県": ("JA", "TY"),
    "和歌山県": ("JA", "WK"),
    "山口県": ("JA", "YG"),
    "山形県": ("JA", "YM"),
    "山梨県": ("JA", "YN"),
    # 四国
    "愛媛県": ("JA5", "EH"),
    "香川県": ("JA5", "KA"),
    "高知県": ("JA5", "KC"),
    "徳島県": ("JA5", "TS"),
    # 九州・沖縄
    "福岡県": ("JA6", "FO"),
    "鹿児島県": ("JA6", "KG"),
    "熊本県": ("JA6", "KM"),
    "宮崎県": ("JA6", "MZ"),
    "長崎県": ("JA6", "NS"),
    "沖縄県": ("JA6", "ON"),
    "大分県": ("JA6", "OT"),
    "佐賀県": ("JA6", "SG"),
}

# 北海道振興局キーワード → (assoc, area_code)
_HOKKAIDO_MAP = {
    "石狩": ("JA8", "IS"),
    "渡島": ("JA8", "OM"),
    "檜山": ("JA8", "HY"),
    "後志": ("JA8", "SB"),
    "空知": ("JA8", "SC"),
    "上川": ("JA8", "KK"),
    "留萌": ("JA8", "RM"),
    "宗谷": ("JA8", "SY"),
    "オホーツク": ("JA8", "OH"),
    "胆振": ("JA8", "IR"),
    "日高": ("JA8", "HD"),
    "十勝": ("JA8", "TC"),
    "釧路": ("JA8", "KR"),
    "根室": ("JA8", "NM"),
}


def get_region_key(props):
    """N03 フィーチャのプロパティから (assoc, area_code, region_name) を返す。マップ外は None。"""
    n03_001 = (props.get("N03_001") or "").strip()
    n03_002 = (props.get("N03_002") or "").strip()

    if n03_001 == "北海道":
        for keyword, (assoc, code) in _HOKKAIDO_MAP.items():
            if keyword in n03_002:
                return assoc, code, n03_002
        return None

    if n03_001 in _PREF_MAP:
        assoc, code = _PREF_MAP[n03_001]
        return assoc, code, n03_001

    return None


def main():
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--input",  type=Path,
                        default=_DATA_DIR / "ref/N03-2026.geojson",
                        help="入力 N03 GeoJSON ファイル")
    parser.add_argument("--output", type=Path,
                        default=_DATA_DIR / "ref/N03-2026_regions.geojson",
                        help="出力 GeoJSON ファイル（都道府県/振興局レベル）")
    args = parser.parse_args()

    if not args.input.exists():
        raise SystemExit(f"入力ファイルが見つかりません: {args.input}\n"
                         "国土数値情報 N03-2026 全国行政区域 GeoJSON をダウンロードして配置してください。\n"
                         "URL: https://nlftp.mlit.go.jp/ksj/gml/datalist/KsjTmplt-N03-2026.html")

    print(f"読み込み中: {args.input}")
    with open(args.input, encoding="utf-8") as f:
        data = json.load(f)

    features = data.get("features", [])
    print(f"  {len(features)} フィーチャ")

    groups = defaultdict(lambda: {"geoms": [], "assoc": None, "area_code": None, "region_name": None})
    skipped = 0

    for feat in features:
        props = feat.get("properties") or {}
        result = get_region_key(props)
        if result is None:
            skipped += 1
            continue
        assoc, code, name = result
        key = (assoc, code)
        groups[key]["geoms"].append(shape(feat["geometry"]))
        groups[key]["assoc"] = assoc
        groups[key]["area_code"] = code
        groups[key]["region_name"] = name

    print(f"  {len(groups)} 地域に分類（スキップ: {skipped} フィーチャ）")

    out_features = []
    for (assoc, code), grp in sorted(groups.items()):
        print(f"  dissolve: {assoc}/{code} ({grp['region_name']}, {len(grp['geoms'])} ポリゴン) ...",
              end="", flush=True)
        dissolved = unary_union(grp["geoms"])
        print(" 完了")
        out_features.append({
            "type": "Feature",
            "geometry": mapping(dissolved),
            "properties": {
                "assoc":       grp["assoc"],
                "area_code":   grp["area_code"],
                "region_name": grp["region_name"],
            },
        })

    out = {"type": "FeatureCollection", "features": out_features}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    with open(args.output, "w", encoding="utf-8") as f:
        json.dump(out, f, ensure_ascii=False)

    print(f"出力: {args.output} ({len(out_features)} フィーチャ)")


if __name__ == "__main__":
    main()
