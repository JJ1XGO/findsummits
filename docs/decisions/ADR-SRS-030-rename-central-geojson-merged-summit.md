# ADR-SRS-030: 中心成果物 GeoJSON を merged_summit.geojson へリネーム

| 状態 | 採用・未実装 |
| 決定日 | 2026-06-18 |

## Context

[ADR-SRS-013](ADR-SRS-013-merged-geojson-as-central-data.md) で FR-009 の出力を `merged.geojson`（中心成果物）と位置付けた。しかしその後、以下の問題が顕在化した。

1. **和名の衝突**: FR-018 が生成する中間ファイル `merged_peak.geojson`（和名「統合ピーク候補 GeoJSON」）と、`merged.geojson`（和名「統合ピーク候補」）で「統合ピーク候補」という語が重複しており、どちらが最終成果物か和名から区別できない。

2. **概念との乖離**: FR-009 の説明に「データ概念がピーク中心 → サミット中心へ切り替わる節目」と明記されているにもかかわらず、中心成果物の和名に「ピーク候補」という語が残っていた。

3. **ファイル名のペア関係が不明確**: 出力の対として生成される `merged_summit.xlsx` とのペア関係が、ファイル名から読み取れなかった（`merged.geojson` と `merged_summit.xlsx` は basename が揃っていない）。

なお [ADR-SRS-029](ADR-SRS-029-geojson-naming-align-with-csv.md) では per-mesh GeoJSON の basename を対応する CSV に揃えており、本 ADR はその方針を中心成果物へも一貫して適用するものである。

## Decision

### ファイル名のリネーム

`merged.geojson` → **`merged_summit.geojson`**

- `merged_summit.xlsx`（サミット一覧（突合後））と basename を揃え、セット関係を明確化する。
- 中身・フィーチャ構成・プロパティ定義は変更しない。

### 和名の変更

「統合ピーク候補（`merged.geojson`）」→ **「突合済み統合 GeoJSON（`merged_summit.geojson`）」**

- 「突合済み」により SOTA 突合**後**の最終成果物であることを示し、中間ファイル「統合ピーク候補 GeoJSON（`merged_peak.geojson`）」（突合**前**）と明確に区別する。
- 「統合」は複数の per-mesh 成果物を統合したという本来の意味を維持する。

## Alternatives

### 案 A: 和名のみ変更・ファイル名 `merged.geojson` 維持

ファイル名を変えずに和名だけ整理する案。変更コストは最小だが、`merged_summit.xlsx` とのペア関係がファイル名から読み取れないままとなる。ADR-SRS-029 の「CSV と GeoJSON の basename 対応」方針とも一貫しない。→ **却下**

### 案 B: `merged.geojson` のまま全維持

和名・ファイル名ともに現状維持する案。和名の重複・概念乖離・ペア不明確が解消されない。→ **却下**

## Consequences

### SRS への影響

- `merged.geojson` への参照（和名「統合ピーク候補」を含む）を全箇所で `merged_summit.geojson`（和名「突合済み統合 GeoJSON」）へ更新する。
- 主要更新箇所: 3.2 主要コンポーネント構成, 3.3 フェーズ俯瞰, FR-009 入出力テーブル・説明, FR-013, FR-014, 7.2 内部データ一覧 等。

### 関連 ADR への影響

`merged.geojson` を参照する以下の ADR 本文を `merged_summit.geojson` へ更新する:

- [ADR-SRS-013](ADR-SRS-013-merged-geojson-as-central-data.md)（中心成果物定義元・リネーム経緯を追記）
- [ADR-SRS-026](ADR-SRS-026-intermediate-geojson-peak-col-visualization.md)
- [ADR-SRS-011](ADR-SRS-011-delete-zone-polygon.md)
- [ADR-URD-016](ADR-URD-016-observability-intermediate-visualization-ur.md)
- [ADR-URD-014](ADR-URD-014-gsi-tile-attribution-policy.md)
