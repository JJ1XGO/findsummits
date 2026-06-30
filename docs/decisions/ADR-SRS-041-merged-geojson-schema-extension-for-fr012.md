# ADR-SRS-041: merged_summit.geojson スキーマ拡張による FR-012 生成可能化

| 状態 | 採用・未実装 |
| 決定日 | 2026-06-28 |

## Context

[FR-012](../20_SRS.md#fr-012-サミット一覧申請内容反映版生成)（サミット一覧（申請内容反映版）生成）は HTML ビューア（[FR-019](../20_SRS.md#fr-019-html-ビューア機能仕様)）内でブラウザ生成される。
[FR-013](../20_SRS.md#fr-013-html-ビューア生成) が埋め込む入力は `merged_summit.geojson` のみであり（ADR-SRS-013）、[FR-012](../20_SRS.md#fr-012-サミット一覧申請内容反映版生成) が参照できるのも
この geojson だけである。

一方、SRS が [FR-012](../20_SRS.md#fr-012-サミット一覧申請内容反映版生成) の出力として要求するカラムのうち、以下の 6 項目が
`merged_summit.geojson` の [FR-009](../20_SRS.md#fr-009-sotaリスト突合match_status-判定) スキーマ正本（SRS [FR-009](../20_SRS.md#fr-009-sotaリスト突合match_status-判定)「各フィーチャのプロパティ」）に存在しなかった:

| カラム | 属する geojson フィーチャ |
|---|---|
| `col_margin_px` | Point: コル |
| `analysis_count` | Point: ピーク |
| `expected_count` | Point: ピーク |
| `municipality` | Point: ピーク / Point: 既存 SOTA サミット |
| `dominant_peak_code` | Point: 既存 SOTA サミット（dominant 従属の delete のみ） |
| `dominant_peak_dist_m` | Point: 既存 SOTA サミット（dominant 従属の delete のみ） |

これらは [FR-009](../20_SRS.md#fr-009-sotaリスト突合match_status-判定) が生成する `merged_summit.xlsx`（サミット一覧（突合後））には含まれており、
[FR-012](../20_SRS.md#fr-012-サミット一覧申請内容反映版生成) と merged_summit.xlsx の「カラム構成は同一」という前提（SRS §6.5 付近）を維持するためにも
geojson への追加が必要である。

## Decision

`merged_summit.geojson` の各フィーチャ・プロパティ定義（[FR-009](../20_SRS.md#fr-009-sotaリスト突合match_status-判定) スキーマ正本）に
上記 6 プロパティを追加する。各プロパティの値は [FR-009](../20_SRS.md#fr-009-sotaリスト突合match_status-判定) がバッチ処理時に格納し、
[FR-012](../20_SRS.md#fr-012-サミット一覧申請内容反映版生成) はブラウザ内で geojson から読み出して XLSX を生成する。

これにより:

- [FR-012](../20_SRS.md#fr-012-サミット一覧申請内容反映版生成) が `merged_summit.geojson` のみを入力として全出力カラムを生成できるようになる
- merged_summit.geojson と merged_summit.xlsx のカラム統一が維持される
- [FR-013](../20_SRS.md#fr-013-html-ビューア生成) の geojson 埋め込み方式（ADR-SRS-013）は変更不要

## Alternatives

**案B: [FR-012](../20_SRS.md#fr-012-サミット一覧申請内容反映版生成) の出力カラムを削減する**

6 項目を [FR-012](../20_SRS.md#fr-012-サミット一覧申請内容反映版生成) の出力から除外し、geojson で賄えるカラムのみで構成する案。
ただし以下の理由で却下:

- `municipality`・`dominant_peak_code`・`dominant_peak_dist_m` は申請担当者が確認に使う有用情報であり欠落させるべきでない
- `analysis_count`・`expected_count` は解析品質を示す診断情報で、申請書作成時の参照値として必要
- SRS §6.5 の「カラム構成は同一」という前提が崩れ、別途修正が波及する

## Consequences

- [FR-009](../20_SRS.md#fr-009-sotaリスト突合match_status-判定) スキーマ正本（SRS [FR-009](../20_SRS.md#fr-009-sotaリスト突合match_status-判定) の各フィーチャ・プロパティ表）に 6 プロパティを追加する（SRS 更新）
- [FR-009](../20_SRS.md#fr-009-sotaリスト突合match_status-判定) の実装でこれらの値を geojson フィーチャに格納するよう追従が必要（実装フェーズ）
- [FR-012](../20_SRS.md#fr-012-サミット一覧申請内容反映版生成) の実装は geojson から全カラムを読み出せる前提で構築できる
- [FR-013](../20_SRS.md#fr-013-html-ビューア生成) の geojson 埋め込みは変更不要（geojson を丸ごと埋め込む方式のため）
- merged_summit.geojson のサイズがプロパティ追加分だけ増加するが、診断情報のため許容できる
