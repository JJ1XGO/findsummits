# FR-012 spec-panel 指摘対応 計画

## Context

`spec-panel` で FR-012（サミット一覧（申請内容反映版）生成）をレビューした。当初 5 件の指摘を出したが、
計画着手前の再精査でうち 2 件の前提が変わった。

- **指摘②（`sota_lat`/`sota_lon` をスキーマ正本に追加）は取り下げ**。`peak_lat`/`peak_lon`・`col_lat`/`col_lon`
  と同様に各 Point の **geometry 座標**から取得する値であり（SRS 681行・725行が `peak_lat`/`peak_lon` を
  geometry 変換値と定義）、FR-009 スキーマ正本のプロパティ表に列挙されないのは正常。`sota_lat`/`sota_lon` は
  対応する既存 SOTA サミット Point の geometry から取得できる。
- **真の論点（指摘③' に置換）**: FR-009 の `merged_summit.xlsx`（サミット一覧（突合後））と
  FR-012 の `merged_summit_revised.xlsx` は、`merged_summit.geojson` の複数フィーチャ（ピーク Point／
  コル Point／AZ Polygon／対応サミット Point／LineString）を **1 行に集約**して生成するが、その
  **行集約モデル**（集約単位・結合キー・各カラムの取得元フィーチャ）が SRS 未定義。FR-012 概要は
  「Point フィーチャのみが行に変換される」(1298行) としか書かず、集約方法が欠落している。
  FR-012 は「FR-009 と同じ作り方＋ユーザー編集反映」なので、**FR-009 側の行集約モデルが決まれば従属的に決まる**。

**本計画の方針**: 行集約モデル（指摘③'）は仕様議論を伴うため **issue 登録して後日検討**。本セッションでは
行集約モデルに依存せず独立に直せる指摘①④⑤のみ SRS に反映する。

## タスク一覧

### タスク1: 行集約モデル未定義を issue 登録（指摘③'）　[Sonnet]

`venv/bin/python3 mgmt/tracker/track.py issue add` で登録する。

- `--title`: 「FR-009/FR-012 サミット一覧 XLSX の行集約生成モデルが SRS 未定義」
- `--priority 高` `--type 設計` `--stage SRS` `--category その他`
- `--description`: `merged_summit.geojson` は複数フィーチャ（ピーク Point／コル Point／AZ Polygon／
  既存サミット Point／LineString）を持つ。FR-009 の `merged_summit.xlsx` と FR-012 の
  `merged_summit_revised.xlsx` はこれらを 1 行（＝1 ピーククラスタ想定）に集約してカラム化するが、
  (a) 行の単位（集約単位か Point 毎か）、(b) 結合キー（`summit_code` 等）、(c) 各カラムの取得元フィーチャ
  （`peak_*`/`col_*`/`sota_*`＝各 Point geometry、`area_complete`＝AZ Polygon properties、dominant の
  `sota_*`＝`dominant_peak_code` 経由）が SRS 未定義。FR-012 概要は「Point フィーチャのみが行に変換される」
  としか書かず集約モデルが欠落している。
- `--resolution`: FR-009 を行集約モデルの正本として XLSX 行生成モデルを定義し、FR-012 はそれを参照する。
  必要なら ADR 化（ADR-SRS-041「geojson スキーマ拡張で全カラム生成可能」の前提を補完）。

登録後に発番される ISSUE-NNN を控える（コミットメッセージ・todo 連携には docs へ ID を書かない規約に注意。
docs 本文には ISSUE-ID を記載しない＝今回 SRS 本文には書かない）。

### タスク2: §8.1 No.12 台帳の参照 FR に FR-012 を追加（指摘①）　[Sonnet]

`docs/20_SRS.md` 1707行（§8.1 内部データ一覧 No.12 `localStorage 編集内容`）の参照 FR 欄。

- 現状: 参照 FR が `FR-011 / FR-019 / FR-020 / FR-021`
- 修正: FR 番号順に `FR-011 / FR-012 / FR-019 / FR-020 / FR-021` とし FR-012 を挿入
- 根拠: FR-012 の入力テーブル（1287行）が `localStorage 編集内容` を「必須」参照しているのに台帳側に
  未登録（前セッションの FR-011 §8.1 修正の横展開漏れ）

### タスク3: FR-012 `summit_name_jp` カラム説明に localStorage 編集値優先を追記（指摘④）　[Sonnet]

`docs/20_SRS.md` 1310行。

- 現状: 「日本語山岳名（FR-009 が merged_summit.geojson に格納済みの値を引き継ぐ。未取得時は空文字）」
- 修正案: 「日本語山岳名。localStorage に編集値があればそれを優先し（FR-019 が管理）、なければ FR-009 が
  merged_summit.geojson に格納済みの値を引き継ぐ。未取得時は空文字」
- 根拠: FR-012 概要(1280行)は「FR-019 でユーザーが編集した山岳名を反映」と明示しているが、カラム説明は
  引き継ぎのみで編集値優先が読み取れない

### タスク4: FR-012 `match_status` カラム説明の不正確な記述を削除（指摘⑤）　[Sonnet]

`docs/20_SRS.md` 1306行。

- 現状: 「…不備調査用に残存（`unmatched`・`area_complete=false`・`key_col_resolved=false` の per-row 参照に使用）」
- 修正案: 「…`category=review`（`unmatched`）の per-row 確認に使用」（`area_complete=false`・
  `key_col_resolved=false` への言及を削除）
- 根拠: FR-012 はビューア到達後に生成され、`area_complete=false`/`key_col_resolved=false` 行は FR-009 の
  不備ゲート(940〜943行)で除去済みのため FR-012 では出現しない。`merged_summit.xlsx`（突合後）の用途説明を
  転記した形跡
- **スコープ限定**: 「ピーク行/既存サミット行」という行モデル依存の表現は触らない（指摘③' の issue で扱う）

### タスク5: lint・検証・コミット　[Sonnet]

1. `make lint` 警告ゼロ確認
2. `grep` で①の参照 FR に FR-012 反映・④⑤の文言反映・旧記述消失を機械確認
3. `git add docs/20_SRS.md` → コミット（Conventional Commits・日本語本文）
   - 例: `fix(srs): FR-012 spec-panel 指摘①④⑤を修正（行集約モデルは issue 化）`
4. `venv/bin/python3 mgmt/tracker/track.py issue export --if-changed` で Excel 同期

## スコープ外（issue で後日検討）

- 指摘③': FR-009/FR-012 の行集約生成モデル定義（タスク1 で issue 登録のみ）
- 指摘②: 取り下げ（対応不要）

## 検証

- `make lint` 警告ゼロ
- `grep -n "FR-012" docs/20_SRS.md` で §8.1 No.12 行に FR-012 が入ったことを確認
- `grep -n "area_complete=false" docs/20_SRS.md` で FR-012 `match_status` 行（1306行付近）から
  当該記述が消えたことを確認（他箇所の `area_complete=false` は残ってよい）
- `venv/bin/python3 mgmt/tracker/track.py issue show ISSUE-NNN` で登録内容を確認

## モデル運用

全タスク **Sonnet**。設計判断は本計画で完了済みで、残りは SRS 本文の局所修正と issue 登録コマンドの
ファイル編集中心作業のため。実装セッションが Opus の場合は着手前に `/model` で Sonnet 切替を促す。
