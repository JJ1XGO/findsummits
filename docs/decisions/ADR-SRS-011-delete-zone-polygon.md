# ADR-SRS-011: delete判定ゾーンポリゴンの導入

| 状態 | 採用・未実装 |
| 決定日 | 2026-05-21 |

## Context

### 削除判定方式の課題（旧方針: コル等高線ポリゴン）

ピーク域ポリゴン生成（[FR-016](../20_SRS.md#fr-016-ピーク域ポリゴン生成)）で生成していた「コル等高線ポリゴン」（Flood Fill 閾値 = `col_elev` 以上）を削除判定に用いる旧方針には、以下の課題があった。

1. **独立峰問題**: プロミネンス 500m 超の独立峰（富士山・利尻岳・大雪山等）では、コル等高線が遠方の山域まで連続し、ポリゴンが日本全土規模に膨張する
2. **ガード条件の恣意性**: 上記回避のため「既存 SOTA プロミネンス > 500m なら削除除外」のガード条件が必要となるが、500m 値に技術的根拠がない
3. **コル未確定ピークの判定欠落**: `key_col_resolved=false`（広域再解析でも解消しない陸地最高峰級）のピークではコル等高線ポリゴン自体が生成されず、削除判定から漏れる
4. **判定方向の脆弱性**: 「自分より高い周辺ピーク」を基準とするとき、SOTA リスト登録標高と DEM 標高の前後関係が逆転すると判定が不安定になる

### 実データ検証（九州・四国前プロジェクト）

前プロジェクト `findsummits4sotaja` の削除 6 件で、削除側サミットと統合先（高い側）ピークの間の minimax 鞍部標高差（`main_Δ` = 統合先ピーク標高 − minimax 鞍部標高）を実測した結果:

- 6 件全件が `main_Δ` 100〜178m（最小 100.3m、最大 178.0m、平均 125.4m）に分布
- 「AZ 外 50m ゾーン」案では 1 件も拾えない
- 削除判定の本質は **新規ピーク発見によりプロミネンスが SOTA 閾値 150m 未満に再計算される現象** であり、`main_Δ` ≈ 150m に収束する構造的必然がある

詳細は [research/keycol-threshold-analysis.md](research/keycol-threshold-analysis.md) を参照。

## Decision

### delete判定ゾーンポリゴン の定義

各ピークについて、削除判定専用のポリゴンを生成する:

- **feature_type**: `delete_zone`
- **Flood Fill 閾値**: `max(col_elev, peak_elev - delete_zone_max_drop)` 以上。`key_col_resolved=false`（`col_elev` 未確定）のピークでは `peak_elev - delete_zone_max_drop` を閾値とし、`delete_zone_max_drop` 上限キャップによりプロミネンス不明でもポリゴン生成が可能（広域再解析後も delete判定ゾーンは再生成しない。広域再解析の目的は `key_col_resolved=true` 化のみであり、ポリゴンは通常 per-mesh の結果を使用する）
- **等価式**: ピーク標高から `min(prominence, delete_zone_max_drop)` 以内の連続エリア
- **パラメータ**: `delete_zone_max_drop`（`params/config.ini`）

`delete_zone_max_drop` の値は **250m** とする（2026-05-21 実測にて確定）。

確定値の算出:

```text
delete_zone_max_drop = 150 + ceil(max_abs_diff / 50) * 50
                     = 150 + ceil(52.26 / 50) * 50
                     = 150 + 100 = 250
```

- 150m: SOTA プロミネンス閾値
- 52.26m: JA サミット 4,514 件（廃止サミット除外）の abs(SOTA登録標高 − DEM標高) 最大値（JA/TK-014 大山, 東京）
- 50m 単位切り上げ: 余裕込みで切りのよい整数

実測の詳細: `analysis/sota_dem_elevation_diff.py` を全 JA サミット 4,600 件に対して実行（2026-05-21）。`ValidTo` が現在日付より前の廃止サミット 86 件を除外した 4,514 件すべてで abs_diff を取得。平均 2.09m・中央値 1.19m・95p 7.24m・99p 12.66m・最大 52.26m。

なお、当初は暫定値として 250m を設定していたが、これは感覚値であり、実測の根拠を持たなかった。実測により偶然同値となったが、本確定は実測データに基づく独立した判断である。

### 既存サミット判定（FR-009）

ピークから生成された AZ ポリゴンおよび delete判定ゾーンポリゴンに対し、SOTA リストの各サミット座標で point-in-polygon 判定を行う。判定は**座標のみ**で行い、SOTA リスト登録標高と DEM 標高の前後関係には依存しない（標高無関係）。

| `summit.match_status` | 条件 |
|---|---|
| `matched` | いずれかのピークの AZ 内に存在 |
| `delete` | いずれかのピークの delete判定ゾーン内かつ AZ 外に存在 |
| `unmatched` | いずれにも該当しない（要確認の孤立サミット。件数しきい値超過時のみ停止） |

`unmatched` は、噴火・山体崩壊・カルデラ陥没で山が消失・大幅低下した場合（＝削除すべきサミット）と、`delete_zone_max_drop` の値が小さすぎる等の不備（＝システム不備）の両方で発生しうる。座標だけでは両者を機械区別できないため、本 ADR 制定時に想定した「本来発生しないべき不備・即停止」から、[ADR-SRS-037](ADR-SRS-037-unmatched-summit-needs-review.md) により「**担当者の確認を要する状態（要確認）として続行**」へ方針を改めた。`unmatched` 単独では停止せず（`merged_summit.xlsx`・HTML ビューアの「要確認」カテゴリで提示）、件数が **要確認サミット件数しきい値**（`unmatched_review_threshold`）を超えた場合のみ解析異常の疑いとして merge.py が non-zero exit で停止し、後続の [FR-013](../20_SRS.md#fr-013-html-ビューア生成)（GeoJSON/HTML 生成）をスキップする。

### コル等高線ポリゴンの廃止

旧方針の「コル等高線ポリゴン」（`feature_type="key_col_boundary"`、Flood Fill 閾値 = `col_elev`）は廃止する。判定構造（peak.match_status の matched / dominant / new、summit.match_status の matched / delete）と主ピーク特定アルゴリズム（最小プロミネンス）は維持する。

### 副次効果

- **独立峰問題の自然解消**: 250m キャップによりポリゴンが日本全土規模に膨張しない
- **ガード条件不要**: 500m のような恣意的な閾値が不要に
- **広域再解析のトリガー縮小**: AZ（標高差 25m）・delete判定ゾーン（250m 上限キャップ）はいずれも複数の 3×3 メッシュ解析を統合する段階で完結ポリゴンが見つかる想定のため、[FR-014](../20_SRS.md#fr-014-広域結合解析オーケストレーション)（広域結合解析オーケストレーション）のトリガーは `key_col_resolved=false` のみに限定できる。`area_complete=false` が想定外に発生した場合は merge.py の `is_area_incomplete` 不備フラグで処理停止する
- **削除判定のシンプル化**: 座標のみで判定するため、SOTA 登録標高と DEM 標高の前後関係に依存しない

## Alternatives

### 案 A: AZ + AZ 外 50m ゾーン（不採用）

ピーク標高 −25m（AZ）/−50m（追加ゾーン）の固定閾値で 2 段階判定する案。九州・四国実測で 6 件全件が `main_Δ` ≥ 100m なため、50m ゾーンでは 1 件も拾えない。固定閾値ではプロミネンスの大小に追従できず、判定が機能しない。

### 案 B: コル等高線ポリゴン継続 + ガード条件（不採用）

旧方針を維持し「既存 SOTA プロミネンス > 500m なら削除除外」をガード条件として追加する案。500m 値が恣意的で、独立峰問題への根本対処にならない。delete判定ゾーン方式では 250m キャップにより独立峰問題が自然解消するため、ガード条件自体が不要になる。

### 案 C: コル等高線ポリゴン継続 + 暫定ポリゴン union（不採用）

Union-Find を拡張し、暫定 `col_elev` で暫定ポリゴンを生成する案。コル等高線方針自体が独立峰問題を抱えているため、複雑化するだけで根本解決にならない。

### 案 D: 固定閾値 300m ポリゴン（不採用）

`peak_elev - 300m` 以上の固定閾値で削除判定する案。プロミネンスに連動しないため、プロミネンス 150〜300m のサミットでポリゴンが実プロミネンス（コル）を超えて拡張し、判定が過剰になる。delete判定ゾーン方式（`max(col_elev, peak_elev - 250m)`）はプロミネンス連動でこの問題を回避する。

## Consequences

### 影響を受ける文書・実装

- **ADR-URD-007** (peak-match-status-terminology): 「コル等高線内」表記を「delete判定ゾーン内」に更新
- **ADR-SRS-008** (dominant-peak-identification): 「コル等高線ポリゴン」「feature_type=key_col_boundary」表記を delete判定ゾーン関連に更新（アルゴリズム本体は維持）
- **[FR-016](../20_SRS.md#fr-016-ピーク域ポリゴン生成)**: コル等高線ポリゴン仕様を削除し、delete判定ゾーンポリゴン仕様を追加
- **[FR-009](../20_SRS.md#fr-009-sotaリスト突合match_status-判定)**: `summit.match_status` に `unmatched` 追加、エラー停止仕様追加
- **[FR-013](../20_SRS.md#fr-013-html-ビューア生成)**: dominant/new フィーチャ構成のポリゴン種別を変更
- **[FR-014](../20_SRS.md#fr-014-広域結合解析オーケストレーション)**: 再解析トリガーを `key_col_resolved=false` のみに限定（AZ・delete判定ゾーンの `area_complete=false` はトリガー対象外）。広域モードでは Key コル特定のみ行い、ポリゴン生成（[FR-016](../20_SRS.md#fr-016-ピーク域ポリゴン生成)）は実行しない。出力は広域 per-mesh CSV のみで GeoJSON は出力しない
- **GLOSSARY**: 「delete判定ゾーン」用語追加、「コル等高線ポリゴン」用語削除
- **C エンジン** (`src/analyze.c`, `src/mesh_analyze.c`): Flood Fill 閾値とポリゴン種別の変更
- **merge.py**: AZ / delete判定ゾーンの point-in-polygon 実装、不備フラグ格納、exit code 制御
- **output_geojson.py**: dominant フィーチャ構成変更
- **params/config.ini.example**: `delete_zone_max_drop` パラメータ追加

> **補足（ADR-SRS-013 採用後）**: 不備フラグは `merged.csv` の列ではなく `merged_summit.geojson` のメタデータプロパティに格納する（詳細は [ADR-SRS-013](ADR-SRS-013-merged-geojson-as-central-data.md)）。`merged.csv` は `merged_summit.geojson` から派生するエビデンス CSV であり不備フラグは含めない。
>
> **補足（ADR-SRS-033 採用後）**: top-level boolean 不備フラグ（`is_unmatched_summit`/`is_area_incomplete`/`is_key_col_unresolved`）は `merged_summit.geojson` の metadata からも削除し、不備確認の責務を `merged_summit.xlsx`（per-row 表示）へ移行する。per-feature プロパティ（`key_col_resolved`・`area_complete`）は維持する（詳細は [ADR-SRS-033](ADR-SRS-033-defect-confirmation-via-xlsx.md)）。

### 未確定事項

- 全国解析実行時の `summit.match_status="unmatched"` 発生件数の分布実測（[ADR-SRS-037](ADR-SRS-037-unmatched-summit-needs-review.md) で `unmatched` を要確認扱いへ変更したため、「発生しないことの実証」から「発生件数の分布確認・`unmatched_review_threshold` 確定値の決定」へ更新）
