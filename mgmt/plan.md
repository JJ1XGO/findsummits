# コル（key_col）への municipality 付与

## Context

突合済み統合 GeoJSON（`merged_summit.geojson`）の Point フィーチャのうち、
サミット（matched/delete/unmatched）とピーク（matched/new/dominant）には
`municipality`（市区町村名）が付与される仕様だが、**コル（`key_col`）だけが
properties 定義から漏れている**。担当者の所在確認に有用なため、コルにも
`municipality` を持たせたい。

調査結果（`scripts/merge.py` / `docs/20_SRS.md`）:

- 現状 `municipality` の付与処理自体が **未実装**。`merge.py` は現在 `merged.csv` を
  出力する段階で、`merged_summit.geojson` 生成・市区町村判定（point-in-polygon）・
  前段の市区町村 GeoJSON 生成（`preprocess_pref_boundaries.py` は地域レベルのみ）が
  いずれも未実装。
- したがって本タスクの実体は **SRS（FR-009）仕様の改訂**。実装は将来の FR-009
  municipality 付与実装時に、ピーク/サミットと同じロジックをコル座標へ適用して
  一括対応する（前段未実装のためコルだけ先行実装はできない）。

判定方針（ユーザー確認済み）: コルは対応ピークと別の市区町村にまたがりうるため、
**コル自身の座標で point-in-polygon 判定する**（親ピーク継承ではない）。

## スコープ

- **対象**: `docs/20_SRS.md` の FR-009 仕様改訂のみ
- **対象外**: `merge.py` 等の実装（前段未実装のため将来 FR-009 実装時に対応）

## 変更内容（全タスク: Sonnet）

### 1. コル properties 定義に `municipality` を追加

`docs/20_SRS.md` の「Point: コル」テーブル（`col_margin_px` 行の付近、現状の
`feature_type`/`category`/`summit_code`/`col_elev`/`points`/`col_margin_px` に続けて）
へ 1 行追加する。

- 追加する行（プロパティ名 `municipality`、説明）: 「市区町村名（例: "根室市"）。
  コル自身の座標を N03 前処理済み市区町村 GeoJSON と照合して取得（親ピークの継承
  ではない）。未存在時は空文字」
- ピーク（現状の `municipality` 行）・サミット行の文言（「未存在時は空文字」）と
  表現を揃える。コルは `key_col_resolved=false` のとき Point 自体が出力されない点は
  テーブル冒頭の既存注記（`key_col_resolved=false` の場合は含めない）でカバー済み。

### 2. FR-009 本文の市区町村判定記述を更新

`docs/20_SRS.md` の FR-009 本文「市区町村判定」の記述（現状「各ピーク
（matched/new/dominant）および全サミット（delete・unmatched を含む）の座標を …
照合し、`municipality` を取得する」）に、判定対象として **コル（Key コル確定時）** を
追加する。

- 例: 「各ピーク（matched/new/dominant）・**各コル（`key_col_resolved=true` のもの）**・
  全サミット（delete・unmatched を含む）の座標を N03 前処理済み市区町村 GeoJSON と
  照合し、`municipality` を取得する。」

### 3. 整合性確認

- FR-009 入力テーブル（市区町村 GeoJSON）・出力テーブルは変更不要（入力データ・
  成果物は同一）。コル分の追加判定があるだけ。
- `municipality` を rationale テキストへは含めない（GeoJSON プロパティのみ。コルは
  rationale 対象フィーチャではない）。
- `grep -n municipality docs/20_SRS.md` で他にコル言及の追従漏れがないか確認する。

## 課題管理

SRS 本体（FR-009）の仕様追加にあたるため、`mgmt/tracker/` の issue として記録する
（type は 設計 または 改善）。判定方針は本計画で決着済みのため、残作業は SRS への
反映と記録のみ。登録フロー・コマンドは `mgmt/tracker/CLAUDE.md` 参照。

## 検証

- `make lint`（特に `lint-md`）で警告ゼロを確認。
- SRS の「Point: コル」テーブルに `municipality` 行が追加され、ピーク/サミットと
  文言が揃っていることを目視確認。
- FR-009 本文の判定対象記述にコルが含まれることを確認。
- **ドキュメント更新ルール**: 更新完了ターン内に `make lint` →（issue 記録）→
  Conventional Commits でコミット（push は別途指示まで不要）。
