#!/usr/bin/env python3
"""GeoJSON の自作 lint チェッカー（geojson-validator ラッパー）

RFC7946 構造違反（validate_structure）と無効ジオメトリ（validate_geometries の
invalid 基準のみ）を検出する。problematic（座標精度過剰・重複ノード等）は対象外。

BOM 付きファイル（SOTA 公式 ref/geojson_v31/*.geojson 等）も utf-8-sig で読む。
"""
import json
import os
import sys

import geojson_validator

geojson_validator.configure_logging(enabled=False)


def check_file(filepath):
    violations = []
    try:
        with open(filepath, encoding='utf-8-sig') as f:
            data = json.load(f)
    except (OSError, UnicodeDecodeError, json.JSONDecodeError) as e:
        return [f"{filepath}: LOAD-ERROR: {e}"]

    structure = geojson_validator.validate_structure(data)
    if structure:
        violations.append(f"{filepath}: STRUCTURE: {structure}")

    geom = geojson_validator.validate_geometries(data)
    for rule, indices in geom.get('invalid', {}).items():
        violations.append(f"{filepath}: INVALID-{rule}: {len(indices)} feature(s)")

    return violations


def collect_geojson_files(paths):
    files = []
    for path in paths:
        if os.path.isfile(path) and path.endswith('.geojson'):
            files.append(path)
        elif os.path.isdir(path):
            for root, _, filenames in os.walk(path):
                for name in sorted(filenames):
                    if name.endswith('.geojson'):
                        files.append(os.path.join(root, name))
    return files


def main():
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <path> [path...]", file=sys.stderr)
        sys.exit(1)

    files = collect_geojson_files(sys.argv[1:])
    all_violations = []
    for f in files:
        all_violations.extend(check_file(f))

    for v in all_violations:
        print(v)

    sys.exit(1 if all_violations else 0)


if __name__ == '__main__':
    main()
