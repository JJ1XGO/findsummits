# ADR-SRS-021: per-mesh CSV の provenance 列を解析識別子に集約・派生列 points を除外

| 状態 | 採用・未実装 |
| 決定日 | 2026-06-15 |

## Context

FR-007（per-mesh CSV 出力）レビューで、出力カラムに以下の3点の設計問題が発見された。

**問題1: `center_mesh` の名称誤りと広域非対応**

`center_mesh` は通常 per-mesh（`3-<meshcode>`）では3×3解析の中心メッシュコードとして有効だが、
広域 per-mesh（`4-<meshcode>-NW` 等）では4コーナー配置を採用するため「中心メッシュ」の概念が存在しない。
名称が実態と乖離しており、広域行の値の扱いが未定義である。

**問題2: `zoom_level` の冗長性**

`zoom_level` は解析識別子の接頭辞から一意に導出できる（`3` → zoom15、`4/5/6` → zoom14）。
独立列として保持する情報的価値がなく、将来の `col_margin_px` ズーム補正も
`zoom_level` 列ではなく解析識別子接頭辞からの導出で対応できる。

**問題3: `points` のデッドカラム化**

`points`（標高バンドに基づくポイント数）は `peak_elev` からの純粋な派生値である。
- FR-008（CSV統合）は `points` 列を読み込まず、`merged_peak.csv` に出力しない
- FR-009（SOTA突合）は `peak_points = band(floor(peak_elev))` として `peak_elev` から再計算する
- GeoJSON・申請書 xlsx の `points` 列もすべて `peak_elev` / `sota_alt_m` から算出する

FR-007 で書いても FR-008 で即座に捨てられるデッドカラムである。

## Decision

**`center_mesh` と `zoom_level` を廃止し、`analysis_id`（解析識別子）1列に集約する。**
**`points` を FR-007 出力カラムから削除し、消費する段（FR-009・出力生成）で `peak_elev` から算出する。**

`analysis_id` の値は FR-004/FR-014 が内部トランザクションとして FR-007 に渡す解析識別子と同じ文字列とする:
- 通常モード: `3-<中心メッシュコード>`（例: `3-5239`）
- 広域モード: `<N>-<対象メッシュコード>-<コーナー>`（例: `4-5239-NW`）

これにより mode（通常/広域）・meshcode・corner・zoom の全情報が1トークンに内包され、
後段が必要な情報を接頭辞・分解によって取得できる。
通常/広域の判別（接頭辞 `3` vs `4/5/6`）はすでに SRS FR-008 が規定する既存の規約であり、
追加の解釈コストはない。

## Alternatives

**A: 分解列案（mode/mesh/corner/zoom を個別列）**

`analysis_mode`・`target_mesh`・`corner`・`zoom_level` の4列で保持する案。
直感的に読みやすいが、列数増加・FR ごとの列定義増加・解析識別子との二重管理が発生する。
解析識別子は SRS が既定する正準 provenance トークンであり、その冗長な分解は避けるべきと判断し却下。

**B: `center_mesh` 維持 + 広域行は空欄**

現行列名を維持し、広域行は `center_mesh` を空欄とする案。
変更量が最小だが「空欄 = 広域」という暗黙ルールが生まれ、
欠損データの空欄と区別がつかなくなる。
また `zoom_level` の冗長性は解消されない。却下。

**C: `zoom_level` のみ残す（col_margin_px 補正用）**

col_margin_px はズームレベル依存（通常=L15px・広域=L14px）であるため、
後段での補正に `zoom_level` が便利という観点。
ただし zoom は解析識別子接頭辞から導出できるため独立列の必要性はない。
`analysis_id` 列を採用すれば `zoom_level` は派生可能。却下。

## Consequences

- FR-007 出力カラム表: `points`・`center_mesh`・`zoom_level` を削除、`analysis_id` (str) を追加
- FR-007 説明: 広域モード出力動作の `analysis_id` 参照箇所を整合
- FR-008 説明: `analysis_id` 接頭辞で通常/広域を判別する旨・広域行の `expected_count` スキップを追記
- データ表（7.2節）: per-mesh CSV カラム定義への参照箇所を更新
- コード追従（別タスク）: `mesh_analyze.c` CSV 出力・`merge.py` の `analysis_id` 対応
