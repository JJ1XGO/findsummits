# ISSUE-014 関連: dominant peak タイブレーク変更 + フラグ名整理 + フォールバック廃止

## Context

ISSUE-014（削除候補サミットの最近接検出ピーク特定）の議論で、以下 3 点を新方針として決定:

1. **複数包含時のタイブレーク**: ADR-008 の `Haversine 最近接` → `最小プロミネンス` に変更（地形学的に「親に最も近い局所的隆起」を表現できるため）
2. **フォールバック廃止**: 「どのコル等高線ポリゴンにも AZ にも入らない既存 SOTA サミット」は FR-014 が N=6 までエスカレーションした後の状況では理論上稀（陸地最高峰のみ）。発生したら無理矢理紐付けず、ログ警告 + 処理中止で人手判断
3. **フラグ `is_tile_top` → `key_col_unresolved` リネーム**: 「タイル（256×256 px）のトップ」と読める誤解を解消し、状態の本意（コル未確定）を表現。既存の `area_truncated` と並びが揃う

陸地最高峰の機械的特定（陸地ポリゴン整備）は、利用機会が稀かつ N03 前処理拡張コストが大きいため YAGNI で却下。

## 対象文書と変更内容

### 1. `docs/decisions/ADR-008-dominant-peak-identification.md`（中核）

- 最終更新日: `2026-05-19`
- **Decision section** を新方針で書き直し:
  - タイブレーク規則を「最小プロミネンスのピークを採用」に変更
  - フォールバック規定を削除し、「該当ケースはログ警告 + 処理中止」を追記
  - データ構造の補足（merged.csv に検出ピーク行と既存 SOTA サミット行が混在する旨）
  - 処理方向の補足（delete サミット側を外側ループ）
  - 実装方針の補足（`shapely.strtree.STRtree` を AZ・コル等高線の両判定で利用）
- **Alternatives section** に却下案を追記:
  - `Haversine 最近接`（地形的根拠が弱い、最小プロミネンスより精度劣る）
  - `陸地ポリゴンによる陸地最高峰フォールバック`（YAGNI、稀少ケースのために前処理拡張は過剰）
- **Consequences**:
  - `dominant_peak_dist_m` は引き続き Haversine で計算し人手確認用に出力
  - フォールバック該当ケース発生時は ISSUE 起票して個別対応

### 2. `docs/02_SRS.md`

- **FR-005/FR-006/FR-007**: 出力カラム `is_tile_top` → `key_col_unresolved` にリネーム
- **FR-008/FR-018**: 統合キー・重複排除ロジック内のカラム名更新（重複排除ロジック `(is_tile_top=0, ...)` → `(key_col_unresolved=false, ...)`）
- **FR-009 主ピーク特定**:
  - タイブレーク規則を「最小プロミネンス」に変更
  - フォールバック規定を削除し処理中止に変更
  - `merged.csv` の 2 種類の行構造（検出ピーク + 既存 SOTA サミット）を入力セクションに補足
- **FR-013**: GeoJSON フィーチャ構成の参照名（あれば）更新
- **FR-014**: トリガー条件のカラム名 `key_col_unresolved` に更新（既存 `is_tile_top=1` 表記を全置換）
- 目次・本文内の他の `is_tile_top` 出現箇所も漏れなく置換

### 3. `docs/decisions/ADR-004-level14-max-pooling-isolated-peaks.md`

- トリガー条件のカラム名 `is_tile_top=1` → `key_col_unresolved=true` に更新
- 最終更新日: `2026-05-19`

### 4. `docs/00_GLOSSARY.md`

- `key_col_unresolved` の用語定義を追加（旧 `is_tile_top` の意味を継承）
- 既存に `is_tile_top` の項目があれば削除または「旧称」として残す（読み手の混乱回避）

### 5. `mgmt/tracker/data/issues.json`

- **ISSUE-014**: ステータス `対応完了` → `対応中`
  - `resolution` を新方針で書き直し
  - `notes` に「設計変更経緯（最小プロミネンス採用・フォールバック廃止・フラグ名変更）」を追記
  - 変更履歴に「設計再変更により再対応」のエントリを追加
- **ISSUE-033 関連**: 既に解決済のため変更不要（前回更新済）

### 6. `mgmt/tracker/reports/issues_export.xlsx`

- ISSUE-014 更新後に `track.py issue export --if-changed` で再生成

### 7. `mgmt/lessons.md`

- 「実装由来のフラグ名は仕様読者を混乱させる」教訓 1 件追記
  - 例: `is_tile_top` は「タイル（256×256px の地理院タイル）のトップ」と誤読され得る。実体は「3×3 メッシュ解析範囲内で key col 未確定」であり、命名は目的・状態を反映すべき

## 実装側（src/, scripts/）への影響

仕様優先原則のため、本タスクは **SRS/ADR レベルの仕様変更が中心**。実装コード（`mesh_analyze.c` 等で `is_tile_top` を出力）の追従は **別タスク**（実装フェーズで対応）。

- 実装追従用の新規 ISSUE を起票するか、ISSUE-014 のスコープに含めるかは plan 実行時にユーザー確認

## 検証

1. `grep -rn "is_tile_top" docs/` で SRS/ADR/GLOSSARY 内に残存がないことを確認
2. `venv/bin/python3 mgmt/tracker/track.py issue show ISSUE-014` で備考欄・ステータスが想定通りか確認
3. `mgmt/tracker/reports/issues_export.xlsx` を開いて ISSUE-014 行の内容確認
4. ADR-008 の Decision・Alternatives・Consequences の論理整合（特に「フォールバック削除」と「処理中止」が一貫しているか）

## 実行手順

1. `docs/decisions/ADR-008-dominant-peak-identification.md` 編集（中核の設計変更）
2. `docs/decisions/ADR-004-level14-max-pooling-isolated-peaks.md` 編集（カラム名追従）
3. `docs/02_SRS.md` 編集（FR-005/006/007/008/009/013/014/018・目次）
4. `docs/00_GLOSSARY.md` 編集（用語追加）
5. `mgmt/lessons.md` 追記（ネーミング教訓）
6. `track.py issue update ISSUE-014 --status 対応中 --resolution "..." --notes "..."`
7. `track.py issue export --if-changed` で Excel 再生成
8. `git status` で確認 → 個別 `git add <files>` → Conventional Commits 形式（本文日本語）で commit

## 関連ファイル

- `docs/decisions/ADR-008-dominant-peak-identification.md`
- `docs/decisions/ADR-004-level14-max-pooling-isolated-peaks.md`
- `docs/02_SRS.md`
- `docs/00_GLOSSARY.md`
- `mgmt/lessons.md`
- `mgmt/tracker/data/issues.json`
- `mgmt/tracker/reports/issues_export.xlsx`
