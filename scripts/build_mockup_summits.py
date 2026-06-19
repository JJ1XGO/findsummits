#!/usr/bin/env python3
"""
モックアップ用 SOTAサミット全件データ生成スクリプト。
ref/geojson_v31/ja*_v31.geojson を読み込み、docs/mockup/summits_data.js を生成する。
テストデータが揃ったら geojson を差し替えて再実行することで更新できる。
"""
import glob
import json
import os
import re
import sys

GEOJSON_GLOB = os.path.join(os.path.dirname(__file__), "../ref/geojson_v31/ja*_v31.geojson")
OUTPUT_JS = os.path.join(os.path.dirname(__file__), "../docs/mockup/summits_data.js")

NAME_RE = re.compile(r'^([^(]+)\((.+)\)$')


def parse_name(raw):
    """'JA/FI-001(赤兎山)' → (code, name_jp)"""
    m = NAME_RE.match(raw.strip())
    if m:
        return m.group(1).strip(), m.group(2).strip()
    return raw.strip(), ""


def parse_points(altstr):
    """'1629m 10pt ' → 10"""
    m = re.search(r'(\d+)pt', altstr or "")
    return int(m.group(1)) if m else 0


def parse_alt(altstr):
    """'1629m 10pt ' → 1629"""
    m = re.search(r'(\d+)m', altstr or "")
    return int(m.group(1)) if m else None


# GEOJSON_DATA にそのままマージできる summit フィーチャ形式で出力する。
# match_status="matched" → ビューアの「変更なし」カテゴリで初期表示される。
features = []
for path in sorted(glob.glob(GEOJSON_GLOB)):
    with open(path, encoding="utf-8-sig") as f:
        data = json.load(f)
    for feat in data.get("features", []):
        props = feat.get("properties", {})
        geom = feat.get("geometry", {})
        raw_name = props.get("name", "")
        code, name_jp = parse_name(raw_name)
        features.append({
            "type": "Feature",
            "geometry": geom,
            "properties": {
                "feature_type": "summit",
                "match_status": "matched",
                "summit_code": code,
                "summit_name": props.get("読み", ""),
                "summit_name_jp": name_jp,
                "sota_alt_m": parse_alt(props.get("標高", "")),
                "sota_points": parse_points(props.get("標高", "")),
                "gl": props.get("GL", ""),
                "area": props.get("市郡区", ""),
            }
        })

fc = {"type": "FeatureCollection", "features": features}
js = f"window.ALL_SUMMITS = {json.dumps(fc, ensure_ascii=False, separators=(',', ':'))};\n"

with open(OUTPUT_JS, "w", encoding="utf-8") as f:
    f.write(js)

print(f"生成完了: {len(features)} 件 → {OUTPUT_JS}")
