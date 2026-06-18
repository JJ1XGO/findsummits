# ADR-SRS-022: per-mesh GeoJSON プロパティ設計（join 方式）と FR-007→FR-016 データフロー

| 状態 | 採用・未実装 |
| 決定日 | 2026-06-15 |

## Context

FR-016（ピーク域ポリゴン生成）レビューで以下の2つの設計問題が発見された。

**問題1: per-mesh GeoJSON の出力プロパティ未定義**

FR-016 が生成する per-mesh アクティベーションゾーン GeoJSON（`3-<meshcode>_activation.geojson`）の
Feature properties が未定義だった。FR-009（SOTA リスト突合）は merged_activation.geojson と
merged_peak.csv の2入力を受け取るが、両者をどのキーで紐付けるかが規定されていなかった。

**問題2: FR-016 の入力ソースが CSV と不整合を起こす構造だった**

FR-016 が FR-006（コル検出・プロミネンス計算）の出力（フィルタ前ピーク候補リスト）を直接受け取る
設計だった。一方、per-mesh CSV（FR-007）は FR-006 出力に一次フィルタ（プロミネンス閾値）と
地理的範囲フィルタを適用したフィルタ後集合を出力する。

この構造では CSV（フィルタ後）と GeoJSON（フィルタ前）のピーク集合が一致しなくなり、
後段 FR-009 の join 時に孤立ポリゴン（CSV に対応する CSV 行がない GeoJSON フィーチャ）が
生じうる。また FR-016 が独自のフィルタを重複実装するとロジック不整合のリスクが高まる。

## Decision

### ① join 方式（GeoJSON と CSV の役割分担）

per-mesh GeoJSON の各 Feature `properties` には以下の4フィールドのみを格納する:

- `peak_lat`（float）: join キー。当該ピークの緯度（ピクセル→緯度経度の決定論的変換値）
- `peak_lon`（float）: join キー。当該ピークの経度（同上）
- `feature_type`（str）: `"activation_zone"` または `"delete_zone"`
- `area_complete`（bool）: ポリゴンが解析範囲内で完結しているか

プロミネンス・コル標高・ピーク標高・`key_col_resolved` 等の属性は per-mesh CSV（FR-007）側に
集約する。FR-009 は `peak_lat`/`peak_lon` をキーに merged_peak.csv と merged_activation.geojson を
join して属性を参照する（形状＝GeoJSON / 属性＝CSV の役割分担）。

join キーはピクセル→緯度経度の決定論的変換により CSV と GeoJSON の値が完全一致する
（[NFR-003](../20_SRS.md#nfr-003-再現性決定論的出力) が担保）。

### ② FR-016 の入力を FR-007 のフィルタ後リストに変更

FR-016 の入力ピーク集合を「FR-006 の生出力（フィルタ前）」から「FR-007 が出力する
フィルタ後ピーク候補・コル情報リスト」に変更する。

FR-007 を「採用ピーク確定」工程として位置づけ、一次フィルタ＋地理的範囲フィルタ適用後の
集合を FR-016 が受け取る。これにより per-mesh CSV と per-mesh GeoJSON の対象ピーク集合が
構造的に一致し、FR-009 の join で孤立ポリゴンが生じない。

フロー上の順序（FR-007→FR-016）は変更前から正しい。変更点は「FR-016 が FR-006 の生リストを
並列消費していた」を「FR-007 のフィルタ後リストを直列受け取る」に改めることである。

FR-016 を呼ぶ/呼ばないの制御はオーケストレーター（FR-004/FR-014）の責務。FR-007 は
フィルタ後リストを出力するだけであり、呼び出し制御を持たない（責務分離）。

## Alternatives

**A: self-contained 方式（GeoJSON に全属性を複製）**

GeoJSON の各 Feature properties にプロミネンス・コル標高等の属性も含める案。
CSV と GeoJSON で同一データを二重管理することになり、更新時の不整合リスクが高い。却下。

**B: FR-016 が独自にフィルタを再適用**

FR-016 が FR-006 生リストを受け取り、内部で FR-007 相当のフィルタを再実行する案。
フィルタロジックの重複実装となり、FR-007 と FR-016 のフィルタ条件が乖離したとき
両者のピーク集合が不一致になるリスクがある。却下。

**C: FR-006 生リストを使用し FR-009 で孤立チェック**

FR-016 が FR-006 生リストを受け取る現行設計を維持し、FR-009 が join 後に孤立ポリゴンを
検出してエラー停止する案。孤立を事後検出するのみで根本的な不整合は残る。
FR-007 のフィルタ変更が FR-016 に自動伝播しないため保守性も低い。却下。

## Consequences

- **FR-016**: 入力をフィルタ後ピーク候補・コル情報リスト（FR-007 出力）に変更。
  Feature properties を `peak_lat`/`peak_lon`/`feature_type`/`area_complete` の4フィールドに限定。
- **FR-007**: 出力にフィルタ後ピーク候補・コル情報リスト（内部トランザクション）を追加。
  FR-004 が FR-016 に渡す旨を注記（FR-007 自身は呼び出し制御を持たない）。
- **FR-018**: 統合キー = `peak_lat`/`peak_lon` を明記。
- **FR-009**: merged_peak.csv と merged_activation.geojson の join キー = `peak_lat`/`peak_lon` を明記。
- **コード追従（別タスク）**: mesh_analyze.c の GeoJSON 出力プロパティ・merge.py の join 実装を本 ADR に合わせて修正する。
- **可視化フィーチャ追加（[ADR-SRS-026](ADR-SRS-026-intermediate-geojson-peak-col-visualization.md)）**: UR-013 を満たすため、ゾーンポリゴンに加えてピーク/コル Point・peak→col LineString を中間 GeoJSON に追加する。本 ADR の join 方式（4 プロパティ・ポリゴン設計）は不変で、フィーチャを加算的に追加する。
