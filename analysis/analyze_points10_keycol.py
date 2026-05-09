#!/usr/bin/env python3
"""
Points=10 の JA サミットを対象に、
6×6 メッシュ解析で Key Col を検出できるかを調査するスクリプト。

手法: 親ピーク（より高い最近接 JA サミット）までの距離を
      Key Col 距離の上限代理として使用する。
      Key Col は対象〜親ピーク間の稜線上にあるため、
      Key Col 距離 ≤ 親ピーク距離（保守的推定）。

出力: docs/decisions/research/6x6-mesh-keycol-coverage-study.md
"""

import csv
import math
import sys
from datetime import date
from pathlib import Path

# 1次メッシュサイズ（概算）
MESH_LAT_KM = 74.0  # 0.667° × 111.32km/° ≈ 74km（緯度方向・短辺）
MESH_LON_KM = 88.0  # 1.0° × cos(35°) × 111.32km/° ≈ 91km → 保守的に 88km

# n×n メッシュの保守的解析半径: (n-1)/2 × MESH_LAT_KM
MESH_SIZES = [3, 4, 5, 6, 7, 8]

R_EARTH_KM = 6371.0


def haversine_km(lat1, lon1, lat2, lon2):
    lat1, lat2 = math.radians(lat1), math.radians(lat2)
    dlat = lat2 - lat1
    dlon = math.radians(lon2 - lon1)
    a = math.sin(dlat / 2) ** 2 + math.cos(lat1) * math.cos(lat2) * math.sin(dlon / 2) ** 2
    return 2 * R_EARTH_KM * math.asin(math.sqrt(a))


def mesh_radius_km(n):
    """n×n メッシュの解析半径（緯度方向・保守値）"""
    return (n - 1) / 2.0 * MESH_LAT_KM


def percentile(sorted_values, p):
    if not sorted_values:
        return float("nan")
    idx = (len(sorted_values) - 1) * p / 100.0
    lo = int(idx)
    hi = lo + 1
    if hi >= len(sorted_values):
        return sorted_values[-1]
    return sorted_values[lo] * (1 - (idx - lo)) + sorted_values[hi] * (idx - lo)


def load_ja_summits(path: Path):
    summits = []
    with open(path, newline="", encoding="utf-8-sig") as f:
        next(f)  # 1行目: メタ情報行 "SOTA Summits List (Date=...)"
        reader = csv.DictReader(f)
        for row in reader:
            code = row.get("SummitCode", "")
            if not code.startswith("JA/"):
                continue
            try:
                alt = float(row["AltM"])
                lat = float(row["Latitude"])
                lon = float(row["Longitude"])
                pts = int(row["Points"])
            except (ValueError, KeyError):
                continue
            summits.append({
                "code": code,
                "name": row.get("SummitName", ""),
                "alt": alt,
                "lat": lat,
                "lon": lon,
                "points": pts,
            })
    return summits


def find_parent_distances(targets, all_summits):
    """各ターゲットサミットの親ピーク（最近接の高い JA サミット）までの距離を返す"""
    results = []
    total = len(targets)
    for i, t in enumerate(targets):
        if (i + 1) % 20 == 0 or i + 1 == total:
            print(f"  {i + 1}/{total} ...", end="\r", flush=True)
        best_dist = None
        best_parent = None
        for s in all_summits:
            if s["alt"] <= t["alt"]:
                continue
            d = haversine_km(t["lat"], t["lon"], s["lat"], s["lon"])
            if best_dist is None or d < best_dist:
                best_dist = d
                best_parent = s
        results.append({
            "summit": t,
            "parent": best_parent,
            "dist_km": best_dist,
        })
    print()
    return results


def build_markdown(all_ja, p10, with_parent, no_parent, dists_sorted,
                   stats, coverage, uncovered_6x6, threshold_6x6):
    n_valid = len(dists_sorted)
    lines = []

    lines += [
        "# 6×6メッシュ解析による Points=10 サミット Key Col 検出可能性調査",
        "",
        "| 項目 | 内容 |",
        "|---|---|",
        f"| 作成日 | {date.today()} |",
        "| 対象データ | `ref/summitslist.csv` |",
        "| スクリプト | `analysis/analyze_points10_keycol.py` |",
        "",
        "---",
        "",
        "## 1. 調査背景・目的",
        "",
        "3×3メッシュ解析方式採用時に全 JA サミットを対象とした Key Col 検出可能性調査",
        "（[3x3-mesh-analysis-study.md](3x3-mesh-analysis-study.md)）を実施した。",
        "今回はその上位版として、**JA サミットの中で最も高難度の Points=10 に限定**し、",
        "6×6メッシュ解析まで広げれば Key Col を確実に検出できるかを定量的に検証する。",
        "",
        "Points=10 のサミットは日本最高峰クラスの独立峰（富士山・大山・鳥海山等）を含むため、",
        "Key Col が最も遠くにある可能性が高い。本調査の結果は FR-014（独立峰広域再解析）の",
        "設計根拠として使用する。",
        "",
        "## 2. 手法",
        "",
        "**親ピーク距離を Key Col 距離の上限代理として使用する。**",
        "",
        "Key Col は対象サミットと親ピーク（より高い最近接 JA サミット）を結ぶ稜線上に存在する。",
        "よって `Key Col 距離 ≤ 親ピーク距離`（保守的な上限推定）。",
        "親ピーク距離がメッシュ解析の半径より短ければ、Key Col もその範囲内に収まる。",
        "",
        "### 1次メッシュのサイズ",
        "",
        "| 方向 | 度数 | 物理距離（概算） |",
        "|---|---|---|",
        "| 緯度 | 0.667° | 74 km |",
        "| 経度（35°N） | 1.0° | 88 km |",
        "",
        "### n×n メッシュの解析半径（保守値）",
        "",
        "n×n メッシュ解析は中心メッシュから各方向 (n−1)/2 メッシュ分広がる。",
        "保守的推定として緯度方向（短辺 74km）を使用する。",
        "",
        "| メッシュサイズ | 各方向の広がり | 解析半径（保守値） |",
        "|---|---|---|",
    ]
    for n in MESH_SIZES:
        r = mesh_radius_km(n)
        meshes = (n - 1) / 2
        lines.append(f"| {n}×{n} | {meshes:.1f} メッシュ | {r:.0f} km |")

    lines += [
        "",
        "## 3. 対象サミット",
        "",
        f"- 全 JA サミット: **{len(all_ja)} 件**（親ピーク探索のベースセット）",
        f"- Points=10 サミット: **{len(p10)} 件**（分析対象）",
        f"  - うち親ピークあり（解析対象）: {n_valid} 件",
        f"  - うち親ピークなし（JA 内での最高峰）: {len(no_parent)} 件",
        "",
    ]

    if no_parent:
        lines.append("親ピークなしのサミット（JA サミットリスト内で最高峰）:")
        lines.append("")
        for r in no_parent:
            s = r["summit"]
            lines.append(f"- `{s['code']}` {s['name']} ({int(s['alt'])}m)")
        lines.append("")

    lines += [
        "## 4. 親ピーク距離の分布統計",
        "",
        f"分析対象: Points=10 かつ親ピークあり（n={n_valid}）",
        "",
        "| パーセンタイル | 親ピーク距離（km）|",
        "|---|---|",
    ]
    for p, v in stats.items():
        label = f"p{p}" if p < 100 else "max"
        lines.append(f"| {label} | {v:.1f} |")

    lines += [
        "",
        "## 5. メッシュサイズ別カバー率",
        "",
        f"分母: 親ピークあり {n_valid} 件（親ピークなし {len(no_parent)} 件は除外）",
        "",
        "| メッシュサイズ | 解析半径（km） | カバー率 | 未カバー件数 |",
        "|---|---|---|---|",
    ]
    for n, c in coverage.items():
        lines.append(f"| {n}×{n} | {c['radius_km']:.0f} | {c['rate']:.1f}% | {c['missed']} |")

    lines += [
        "",
        "## 6. 6×6メッシュで未カバーのサミット",
        "",
        f"6×6 メッシュの解析半径（{threshold_6x6:.0f}km）を超える親ピーク距離を持つサミット。",
        "これらは FR-014（独立峰広域再解析）の対象として検討が必要な候補。",
        "",
    ]

    if uncovered_6x6:
        lines += [
            "| SummitCode | 山名 | 標高(m) | 親ピーク距離(km) | 親ピーク |",
            "|---|---|---|---|---|",
        ]
        for r in uncovered_6x6:
            s = r["summit"]
            p = r["parent"]
            lines.append(
                f"| `{s['code']}` | {s['name']} | {int(s['alt'])} "
                f"| {r['dist_km']:.1f} | `{p['code']}` {p['name']} |"
            )
    else:
        lines.append("**なし** — 全 Points=10 サミットの Key Col は 6×6 メッシュ範囲内に収まる。")

    c6 = coverage[6]
    if c6["missed"] == 0:
        conclusion = (
            f"Points=10 の全サミット（親ピークあり {n_valid} 件）の親ピーク距離が "
            f"6×6 メッシュ解析半径（{threshold_6x6:.0f}km）以内に収まった。\n"
            "6×6 メッシュ解析で Key Col を確実に検出できる可能性が高い。"
        )
    elif c6["rate"] >= 99.0:
        conclusion = (
            f"Points=10 サミットの {c6['rate']:.1f}%（{c6['covered']}/{n_valid} 件）は "
            f"6×6 メッシュ（半径 {threshold_6x6:.0f}km）でカバーされる。\n"
            f"残り {c6['missed']} 件は親ピークが 185km 以上離れた超独立峰であり、"
            "さらに広域の解析またはレベル14再解析が必要。"
        )
    else:
        conclusion = (
            f"Points=10 サミットの {c6['rate']:.1f}%（{c6['covered']}/{n_valid} 件）は "
            f"6×6 メッシュ（半径 {threshold_6x6:.0f}km）でカバーされるが、"
            f"{c6['missed']} 件が未カバーのまま残る。設計の見直しが必要。"
        )

    lines += [
        "",
        "## 7. まとめ・FR-014 設計への示唆",
        "",
    ]
    lines.extend(conclusion.split("\n"))
    lines += [
        "",
        "### FR-014 との関係",
        "",
        "FR-014（独立峰広域再解析）は `is_tile_top=1` または `area_truncated=true` の",
        "ピークに対してズームレベル14 での広域再解析を実施する。",
        "本調査で未カバーとなったサミットは FR-014 の必要性を裏付ける実データとなる。",
        "",
        "---",
        "",
        "*本文書は `analysis/analyze_points10_keycol.py` により自動生成*",
    ]

    return "\n".join(lines) + "\n"


def main():
    project_root = Path(__file__).parent.parent
    summitslist = project_root / "ref" / "summitslist.csv"
    out_dir = project_root / "docs" / "decisions" / "research"
    out_md = out_dir / "6x6-mesh-keycol-coverage-study.md"

    if not summitslist.exists():
        print(f"ERROR: {summitslist} が見つかりません", file=sys.stderr)
        sys.exit(1)

    print("=== Points=10 サミット Key Col 距離分析 ===")
    print()

    all_ja = load_ja_summits(summitslist)
    p10 = [s for s in all_ja if s["points"] == 10]
    print(f"全 JA サミット   : {len(all_ja)} 件")
    print(f"Points=10 のみ   : {len(p10)} 件")
    print()

    print("親ピーク距離を計算中...")
    results = find_parent_distances(p10, all_ja)

    with_parent = [r for r in results if r["dist_km"] is not None]
    no_parent = [r for r in results if r["dist_km"] is None]
    dists_sorted = sorted(r["dist_km"] for r in with_parent)
    n_valid = len(dists_sorted)

    print(f"親ピークあり     : {n_valid} 件")
    print(f"親ピークなし     : {len(no_parent)} 件")

    # 統計
    pct_keys = [50, 75, 90, 95, 99, 100]
    stats = {p: percentile(dists_sorted, p) for p in pct_keys}

    # カバレッジ
    coverage = {}
    for n in MESH_SIZES:
        r = mesh_radius_km(n)
        covered = sum(1 for d in dists_sorted if d <= r)
        coverage[n] = {
            "radius_km": r,
            "covered": covered,
            "rate": covered / n_valid * 100 if n_valid else 0.0,
            "missed": n_valid - covered,
        }

    threshold_6x6 = mesh_radius_km(6)
    uncovered_6x6 = sorted(
        [r for r in with_parent if r["dist_km"] > threshold_6x6],
        key=lambda r: -r["dist_km"],
    )

    # --- ターミナル出力 ---
    print()
    print(f"親ピーク距離 統計 (n={n_valid}):")
    for p, v in stats.items():
        label = f"p{p}" if p < 100 else "max"
        print(f"  {label:4s}: {v:7.1f} km")

    print()
    print("メッシュサイズ別カバー率:")
    for n, c in coverage.items():
        marker = " ◀ 6×6" if n == 6 else ""
        print(f"  {n}×{n}  (r≈{c['radius_km']:3.0f}km): {c['rate']:5.1f}%  未カバー {c['missed']:2d} 件{marker}")

    print()
    print(f"6×6メッシュ(r≈{threshold_6x6:.0f}km)で未カバー: {len(uncovered_6x6)} 件")
    for r in uncovered_6x6:
        s = r["summit"]
        p = r["parent"]
        print(f"  {s['code']:20s} {int(s['alt']):4d}m  {r['dist_km']:7.1f}km → {p['code']} {p['name']}")

    # --- Markdown 出力 ---
    md = build_markdown(all_ja, p10, with_parent, no_parent, dists_sorted,
                        stats, coverage, uncovered_6x6, threshold_6x6)
    out_dir.mkdir(parents=True, exist_ok=True)
    out_md.write_text(md, encoding="utf-8")
    print()
    print(f"レポート出力: {out_md}")


if __name__ == "__main__":
    main()
