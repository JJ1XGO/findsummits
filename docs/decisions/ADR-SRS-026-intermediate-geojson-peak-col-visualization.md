# ADR-SRS-026: 中間 GeoJSON へのピーク・コル可視化フィーチャ追加

| 状態 | 採用・未実装 |
| 決定日 | 2026-06-18 |

## Context

[UR-013](../10_URD.md#ur-013)（[ADR-URD-016](ADR-URD-016-observability-intermediate-visualization-ur.md)）は、解析パイプラインの
中間成果物 GeoJSON を生成タイミングで地図上に可視化し、**ピーク・コル・ゾーンの対応関係**を目視確認できることを要求する。
これを受けた [NFR-009](../20_SRS.md#nfr-009-観測可能性中間成果物の可視化) は中間 GeoJSON を地理院地図へのドラッグ&ドロップで確認できることを仕様化したが、
「ピーク・コルの対応関係の精密な可視化（コル位置の地図表示）は保証範囲外」と明示し、本 ADR に委ねていた（[ADR-SRS-025](ADR-SRS-025-observability-nfr-ur013-srs-scope.md) 参照）。

現行の中間 GeoJSON（ADR-SRS-022 の join 方式）は以下のフィーチャのみを持つ:

- Polygon: activation_zone / delete_zone（ゾーンの形状）
- properties: `peak_lat`/`peak_lon`/`feature_type`/`area_complete`（join キー 4 フィールド）

コル座標・ピーク位置・接続線を持たないため、地図に表示しても**どのピークとどのコルが対応するかが判別できない**。

[UR-013](../10_URD.md#ur-013) が要求する可視化の具体的な内容（ユーザー確認済み）:

- **[FR-016](../20_SRS.md#fr-016-ピーク域ポリゴン生成) 出力（per-mesh）**: [FR-007](../20_SRS.md#fr-007-per-mesh-csv-出力プロミネンス閾値適用) で検出したピーク位置・（コル確定済みの場合）コル位置・ゾーンポリゴン
- **[FR-018](../20_SRS.md#fr-018-per-mesh-ピーク候補-geojson-統合) 出力（統合）**: merged_peak.csv のピーク位置・（col_lat/col_lon があれば）コル位置・ゾーンポリゴン
- 確認方法: 地理院地図へのドラッグ&ドロップ（[NFR-009](../20_SRS.md#nfr-009-観測可能性中間成果物の可視化) 方式を維持）

## Decision

### 追加フィーチャ

[FR-016](../20_SRS.md#fr-016-ピーク域ポリゴン生成)（per-mesh GeoJSON）・[FR-018](../20_SRS.md#fr-018-per-mesh-ピーク候補-geojson-統合)（merged_peak.geojson）の両出力に、既存のゾーンポリゴンに加えて
以下のフィーチャを**加算的に追加**する（既存の 4 プロパティ・ポリゴン設計は変更しない）。

| feature_type | geometry | 追加条件 |
|---|---|---|
| `"peak"` | Point（ピーク座標） | 常に生成（全採用ピーク） |
| `"key_col"` | Point（コル座標） | key_col_resolved=true のピークのみ（[FR-016](../20_SRS.md#fr-016-ピーク域ポリゴン生成)）／col_lat が存在するピークのみ（[FR-018](../20_SRS.md#fr-018-per-mesh-ピーク候補-geojson-統合)） |
| `"peak_col_link"` | LineString（ピーク→コル） | key_col がある場合に生成 |

凡例は [ADR-SRS-013](ADR-SRS-013-merged-geojson-as-central-data.md)（`merged_summit.geojson`）の規約を踏襲する
（summit Point・peak→summit 線は SOTA 突合専用のため中間段階は対象外）。

### コル座標の入手元

- **[FR-016](../20_SRS.md#fr-016-ピーク域ポリゴン生成)**: [FR-007](../20_SRS.md#fr-007-per-mesh-csv-出力プロミネンス閾値適用)「フィルタ後ピーク候補・コル情報リスト」の `col_lat`/`col_lon`（`key_col_resolved=true` のピークのみ。独立峰は key_col Point・LineString なし）
- **[FR-018](../20_SRS.md#fr-018-per-mesh-ピーク候補-geojson-統合)**: merged_peak.csv の `col_lat`/`col_lon`（広域解析で確定した独立峰のコルも含む）

### 地理院地図スタイル属性

地理院地図は GeoJSON Feature の `properties` に含まれるスタイル属性（`_`プレフィックス系、例: `_color`・`_opacity`・`_weight` 等）を描画に反映する。
各フィーチャにこれらを埋め込み、ピーク/コル/接続線を色・形状で区別する。
`▲`（caret-up）・`▽`（caret-down）の厳密再現は地理院地図では保証しない（FontAwesome 非対応）。
中間段階は SOTA ポイント未割当のため `merged_summit.geojson` の `pointsToColor` スケールは使えない。
**色のマッピングは HLD で規定**（標高ベース等の代替スキーム）。

### 1 ステージ 1 ファイル方針

可視化フィーチャ（点・線）とゾーンポリゴンを**同一 GeoJSON ファイルに同梱**する（別ファイル分離しない）。
ドラッグ&ドロップ 1 回で全体を確認できるようにする。

### FR-009 への帰結

merged_peak.geojson が非ポリゴンフィーチャ（Point・LineString）を含むようになるため、
[FR-009](../20_SRS.md#fr-009-sotaリスト突合match_status-判定) の point-in-polygon 処理は `feature_type ∈ {activation_zone, delete_zone}` のポリゴンフィーチャに
絞って実行する（Point・LineString を誤って評価しない）。

### ADR-SRS-022 との関係

本 ADR は ADR-SRS-022（join 方式）と矛盾しない。追加する point/LineString フィーチャは**幾何・表現**のための
フィーチャであり、プロミネンス・標高等の**データ属性の二重管理**ではない。
ADR-SRS-022 の 4 プロパティ（`peak_lat`/`peak_lon`/`feature_type`/`area_complete`）の設計は不変。

## Alternatives

**A: HTML ビューアで中間 GeoJSON を描画（アイコン厳密一致）**

merged_viewer.html を拡張し、[FR-016](../20_SRS.md#fr-016-ピーク域ポリゴン生成)/[FR-018](../20_SRS.md#fr-018-per-mesh-ピーク候補-geojson-統合) の中間 GeoJSON を読み込んで
`peakIcon`/`colIcon` で描画する案。`▲`/`▽` が最終成果物と完全一致する。
ビューア拡張コストが高い上、ユーザーはドラッグ&ドロップを選択したため却下。

**B: 可視化専用に別ファイルを分離**

merged_peak.geojson はポリゴン専用に維持し、点・線を別ファイル（例: merged_peaks.geojson）として出力する案。
ファイルが増えてドラッグ&ドロップ回数が増える。ユーザーは 1 ファイル確認を希望したため却下。

**C: コル座標を属性のみで付与（幾何なし）**

GeoJSON の properties に `col_lat`/`col_lon` を追加するだけで点・線は生成しない案。
地図上に点として出ないため、ピーク↔コル対応を目視できず目的を満たさない。却下。

## Consequences

- **[FR-016](../20_SRS.md#fr-016-ピーク域ポリゴン生成)**: per-mesh ピーク候補 GeoJSON（`3-<meshcode>.geojson`）に peak/key_col Point・peak_col_link LineString を追加
  （コルは key_col_resolved=true のみ。地理院地図スタイル属性付き）。コード追従: mesh_analyze.c の GeoJSON 出力拡張（HLD/COD）
- **[FR-018](../20_SRS.md#fr-018-per-mesh-ピーク候補-geojson-統合)**: merged_peak.geojson に merged_peak.csv 由来の peak/key_col Point・peak_col_link LineString を追加
  （独立峰の確定コルも含む）。コード追従: merge.py の GeoJSON 統合処理追従（HLD/COD）
- **[FR-009](../20_SRS.md#fr-009-sotaリスト突合match_status-判定)**: point-in-polygon を `feature_type ∈ {activation_zone, delete_zone}` のポリゴンに絞る注記を追加。コード追従: HLD/COD
- **[NFR-009](../20_SRS.md#nfr-009-観測可能性中間成果物の可視化)**: 「保証範囲外」の記述を削除し、本 ADR を参照してスコープ内に改訂
- **ADR-SRS-022**: Consequences に本 ADR への前方リンクを追加
- コード追従（mesh_analyze.c・merge.py・[FR-009](../20_SRS.md#fr-009-sotaリスト突合match_status-判定) のポリゴン絞り込み）は HLD/COD ステージ（todo.md 転記済み）
