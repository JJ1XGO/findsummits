# ADR-SRS-029: 中間 GeoJSON ファイル名を CSV と同 basename に統一

| 状態 | 採用・未実装 |
| 決定日 | 2026-06-18 |

---

## Context

[ADR-SRS-026](ADR-SRS-026-intermediate-geojson-peak-col-visualization.md)（[UR-013](../10_URD.md#ur-013) 可視化対応）により、中間 GeoJSON（per-mesh・merged 両方）がゾーンポリゴンに加えてピーク/コル Point・peak→col 接続線 LineString を含む多目的ファイルになった。

その結果、既存のファイル名と実態に乖離が生じた:

1. **`_activation.geojson` という名前が実態より狭い**: per-mesh の `3-<meshcode>_activation.geojson` はアクティベーションゾーン（AZ）だけでなく delete 判定ゾーン・ピーク/コル Point・peak→col LineString も含む。
2. **`merged_activation.geojson` も同様**: merged GeoJSON も AZ のみではない。かつファイル名に `activation` が入ることで、GeoJSON の役割が「アクティベーションゾーン専用」と誤読される。
3. **CSV と GeoJSON のセット関係が basename から読み取れない**: per-mesh CSV は `3-<meshcode>.csv`、merged は `merged_peak.csv` という命名なのに、対応する GeoJSON は `3-<meshcode>_activation.geojson`・`merged_activation.geojson` と suffix が不揃い。

## Decision

### ファイル名の変更

セットで生成される CSV と GeoJSON の basename を揃える:

| 変更前 | 変更後 | セットの CSV |
|---|---|---|
| `3-<meshcode>_activation.geojson` | `3-<meshcode>.geojson` | `3-<meshcode>.csv` |
| `merged_activation.geojson` | `merged_peak.geojson` | `merged_peak.csv` |

### 和名の変更

| 変更前 | 変更後 |
|---|---|
| per-mesh アクティベーションゾーン GeoJSON | per-mesh ピーク候補 GeoJSON |
| 統合済みピーク域 GeoJSON | 統合ピーク候補 GeoJSON |

### 据え置き

`feature_type` の値（`activation_zone` / `delete_zone`）はゾーン**種別**を表す正当な名前であり、ファイル名の変更に追随して変更する必要はない。据え置く。

## Alternatives

| 案 | 却下理由 |
|---|---|
| `peak_zone.geojson` / `merged_peak_zone.geojson` にする | CSV（`merged_peak.csv`）と basename が一致しない。「ゾーン」が付くことで再び AZ 専用と誤読される可能性がある |
| 現状維持（`_activation.geojson`） | ファイル内容と名前の乖離が続く。CSV との basename 不揃いも解消されない |
| `activation.geojson` の `activation` をゾーン種別の代表として維持 | `activation_zone` は GeoJSON 内の1種類の `feature_type` 値に過ぎない。ファイル名がその1種類を代表するのは不正確 |

## Consequences

1. **SRS・関連 ADR の記述を更新**: ファイル名・和名の全参照箇所を新命名に置換する
2. **[FR-018](../20_SRS.md#fr-018-per-mesh-ピーク候補-geojson-統合) の SRS 見出しが変わる**: 「[FR-018](../20_SRS.md#fr-018-per-mesh-ピーク候補-geojson-統合): per-mesh activation.geojson 統合」→「[FR-018](../20_SRS.md#fr-018-per-mesh-ピーク候補-geojson-統合): per-mesh ピーク候補 GeoJSON 統合」（Markdown アンカーの更新が必要）
3. **`feature_type` は変更なし**: `activation_zone` / `delete_zone` という値は生きており、SRS・コードで引き続き使用する
4. **実装への影響**: コード内のファイル名ハードコードを変更が必要。詳細は `mgmt/todo.md`
