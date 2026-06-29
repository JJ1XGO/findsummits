# FR-011 spec-panel 指摘対応 計画

## Context

spec-panel で `docs/20_SRS.md` の FR-011（申請書 XLSX 生成）をレビューし、4 件の指摘を抽出した。

- **指摘②（本丸）**: FR-011 ※6 は申請書 B列の県名（add 行）を「N03 前処理済み市区町村 GeoJSON から取得する」と規定するが、申請書は HTML ビューア（FR-019）の「申請書」ボタンで**ブラウザ内で生成**され、ビューアは N03 GeoJSON を同梱しない。よって現仕様では B列県名を取得できず**実行不可**。解決には `merged_summit.geojson`（ビューア埋め込み済み）の Point プロパティに県名を載せる必要があり、FR-009（スキーマ正本）のプロパティ追加を伴う。ユーザー承認で**全フィーチャ一貫**の付与範囲を採用。
- フィールド名は `region_name` とする。申請書 B列は「都道府県名（北海道は振興局名）」を要求するため、市区町村 GeoJSON の `prefecture`（都道府県のみ・北海道は「北海道」）では振興局名が取れない。地域 GeoJSON 由来の `region_name`（都道府県名/振興局名）が正で、これは `docs/20_SRS.md` 878 行の rationale ※2 と同じ出所。
- **指摘①**: `§8.1` 内部データ一覧 No.12（localStorage 編集内容）の参照 FR 欄に FR-011 が欠落（前セッションの FR-019 台帳漏れと同パターンの再発）。
- **指摘③**: FR-011 本体に UR-004「その他」アクション（名称変更・座標変更等）が対象外である旨の記載がなく、トレーサビリティが不完全（FR-019 1227 行には記載済み）。
- **指摘④（対応不要と判断）**: 変更前標高 `sota_alt_m` のデータ型未定義。`sota_alt_m` は SOTA リスト CSV／FR-009 で定義済みの整数 m 値で、FR-011 で再定義不要。本計画では**対応しない**。

対象は `docs/20_SRS.md` 単一ファイル。設計判断は計画段階で確定済みのため、実装はドキュメント編集が中心。

## 実装モデル

全タスク **Sonnet**（設計判断は計画で完了、機械的なドキュメント修正のため）。

## タスク

### タスク0（前段）: 計画ファイルを所定の場所へ移動

- `mv` で本計画を `mgmt/plan.md` へ移動する（プロジェクト運用ルール）。

### タスク1 [Sonnet]: FR-009 に県名プロパティ `region_name` を追加（スキーマ正本）

`docs/20_SRS.md` の FR-009 各 Point プロパティ表へ `region_name` 行を追加する（`municipality` の近傍）。

- **Point: ピーク**（プロパティ表・`municipality` 行付近）に `region_name`（都道府県名、北海道は振興局名。N03 前処理済み地域 GeoJSON の `region_name` から取得。未取得時は空文字）を追加。
- **Point: コル**（同上）に `region_name` を追加。
- **Point: 既存 SOTA サミット**（同上）に `region_name` を追加。
- **市区町村判定の説明**（878 行）を修正: 現状「取得した各値を merged_summit.geojson の各 Point プロパティ（`municipality`）および merged_summit.xlsx に付与する」を、`region_name`（地域 GeoJSON から取得済みの値）も同様に Point プロパティ・`merged_summit.xlsx` に付与する旨へ拡張する。

### タスク2 [Sonnet]: FR-012 反映版カラム表に `region_name` 列を追加

`docs/20_SRS.md` FR-012 の出力カラム表（`municipality` 行付近）に `region_name`（都道府県名/振興局名）列を追加し、`merged_summit.xlsx`（Point 転記）と粒度を揃える。

### タスク3 [Sonnet]: FR-011 ※6 の取得経路を修正（指摘②本体）

`docs/20_SRS.md` FR-011 ※6 を修正する。

- 「[FR-009] の rationale ※2 フォーマットと同じデータソース（N03 前処理済み市区町村 GeoJSON）から取得する」を、「`merged_summit.geojson` の `region_name` プロパティ（FR-009 が格納）を転記する」へ変更する。
- ブラウザ内ビューアが N03 GeoJSON に依存しない経路へ正す。仮サミットコード非出力・MT 使用欄の既存記述は維持。

### タスク4 [Sonnet]: §8.1 台帳の参照 FR 欄に FR-011 を追加（指摘①）

`docs/20_SRS.md` §8.1 内部データ一覧 No.12（localStorage 編集内容）の参照 FR 欄を `FR-011 / FR-019 / FR-020 / FR-021` に修正する。

### タスク5 [Sonnet]: FR-011 に UR-004「その他」対象外の注記を追加（指摘③）

`docs/20_SRS.md` FR-011 のアクション別カラムマッピング表の直後（`no_change`・`review` 非出力の記述付近）に、「名称変更・座標変更等（UR-004 の「その他」アクション）は自動識別対象外（UR-003）のためエクスポート対象外」の 1 文を追記する。FR-019 1227 行の既存記述と整合させる。

### タスク6 [Sonnet]: 検証・コミット

- `make lint` 警告ゼロを確認（`lint-md` のリンク健全性・`scripts/lint_docs.py` 含む）。
- `grep` で確認:
  - FR-011 ※6 から「N03 前処理済み市区町村 GeoJSON から取得」の旧記述が消えていること。
  - FR-009 各 Point（ピーク/コル/サミット）に `region_name` が入っていること。
  - §8.1 No.12 参照 FR に FR-011 が入っていること。
- ドキュメント更新ルールに従い同一ターン内で Conventional Commits でコミット（push は別途指示まで不要）。

## 検証

- `make lint` を実行し警告ゼロ（特に `lint-md` のアンカーリンク切れがないこと）。
- 上記 `grep` チェックで旧記述残存ゼロ・新記述反映を機械確認。
- 指摘②の核心（ブラウザ内で B列県名が取得可能になったか）を、FR-011 ※6 → `merged_summit.geojson` の `region_name` → FR-009 が格納、の参照チェーンが閉じていることで確認する。
