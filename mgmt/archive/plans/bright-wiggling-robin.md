# ベースラインドキュメント採番ルールの整備

## Context

ベースラインドキュメント（仕様基準として参照される土台文書）に体系的な番号を割り当て、将来のドキュメント追加（予定／予定外）にも対応できる枠組みを整備する。

### 現状の問題

- `docs/environment.md` のみ番号が無く、他のベースラインドキュメント（`00_GLOSSARY`, `01_URD`, `02_SRS`）と命名が不揃い。
- 採番ルールが暗黙的で、将来ステージ別ドキュメントに付随ドキュメント（例: SRS の RTM、HLD の方式調査資料）が増えた場合の置き場と命名が定まらない。
- カテゴリが未確定の「予定外」ドキュメントの暫定配置場所がない。

### 範囲

`docs/` 配下の常駐ドキュメントとステージ別ドキュメントのみ。
対象外: `docs/decisions/ADR-*.md`（別命名規則）、`docs/decisions/research/`、`docs/figures/`、`docs/mockup/`、`ref/SOURCES.md`（`ref/` 配下で役割が異なる）。

---

## 採用ルール（10 番台カテゴリ方式）

### カテゴリ区分

| 範囲 | カテゴリ | 用途 |
|---|---|---|
| 00 ～ 09 | 共通（常駐） | ステージに紐づかない参照基盤 |
| 10 ～ 19 | URD ファミリ | URD 本体 + 付随ドキュメント |
| 20 ～ 29 | SRS ファミリ | SRS 本体 + 付随ドキュメント |
| 30 ～ 39 | HLD ファミリ | HLD 本体 + 付随ドキュメント |
| 40 ～ 49 | LLD ファミリ | LLD 本体 + 付随ドキュメント |
| 50 ～ 59 | UT ファミリ | UT 本体 + 付随ドキュメント |
| 60 ～ 69 | IT ファミリ | IT 本体 + 付随ドキュメント |
| 70 ～ 79 | ST ファミリ | ST 本体 + 付随ドキュメント |
| 80 ～ 89 | OPS ファミリ | OPS 本体 + 付随ドキュメント |
| 90 ～ 99 | 予備（予定外） | カテゴリ未確定の暫定配置・後でリネーム |

各カテゴリの **X0 を本体**、**X1〜X9 を付随ドキュメント**に割り当てる（例: `20_SRS.md` 本体、`21_SRS_RTM.md` のように付随）。

### 既存ファイルの新番号

| 現在 | 新規 | 操作 |
|---|---|---|
| `docs/00_GLOSSARY.md` | `docs/00_GLOSSARY.md` | 変更なし |
| `docs/environment.md` | `docs/01_environment.md` | リネーム（新規番号付与） |
| `docs/01_URD.md` | `docs/10_URD.md` | リネーム |
| `docs/02_SRS.md` | `docs/20_SRS.md` | リネーム |

### 採番運用ルール（CLAUDE.md に明文化）

1. ベースラインドキュメント = `docs/` 配下の常駐 + ステージ別。ADR・research・figures・mockup・`ref/SOURCES.md` は対象外。
2. カテゴリ範囲（10 番台単位）に従って採番する。
3. 本体は X0、付随ドキュメントは X1〜X9（カテゴリ内連番）。
4. カテゴリが確定しないドキュメントは **90 番台**に暫定配置し、所属が決まった時点で該当カテゴリへリネームする。
5. ファイル名は `NN_<TITLE>.md`（共通は `01_environment.md` のように小文字、ステージ系は `10_URD.md` のように大文字略号を踏襲）。
6. リネーム時は参照箇所を全て更新する（`grep` で旧パス残存ゼロを確認）。

---

## 実装手順（実行は別ターン）

### 1. ファイルリネーム（`git mv` で履歴保持）

```
git mv docs/environment.md docs/01_environment.md
git mv docs/01_URD.md      docs/10_URD.md
git mv docs/02_SRS.md      docs/20_SRS.md
```

### 2. 参照箇所の一括更新

旧パス → 新パスの置換対象:

- `environment.md` → `01_environment.md`
- `01_URD.md` → `10_URD.md`
- `02_SRS.md` → `20_SRS.md`

更新対象ファイル（`grep -rn` で抽出済み）:

| ファイル | 出現箇所の種類 |
|---|---|
| `CLAUDE.md` | 「各ステージの成果物」表 / 「ディレクトリ構成」コードブロック / 「常駐ドキュメント」リスト |
| `docs/00_GLOSSARY.md` | URD/SRS への参照 |
| `docs/10_URD.md`（リネーム後） | environment.md への自己参照 |
| `docs/20_SRS.md`（リネーム後） | environment.md への自己参照（4 箇所） |
| `docs/decisions/ADR-URD-005-northern-territories-exclusion.md` | URD/SRS 参照 |
| `docs/decisions/ADR-URD-009-takeshima-exclusion.md` | URD 参照 |
| `docs/decisions/ADR-URD-014-gsi-tile-attribution-policy.md` | URD/SRS 参照 |
| `docs/decisions/ADR-SRS-010-cpp-opencv-migration.md` | SRS 参照 |
| `docs/decisions/ADR-SRS-014-srs-parameter-description-convention.md` | SRS 参照 |
| `docs/decisions/ADR-SRS-015-contour-overlay.md` | SRS 参照 |
| `docs/decisions/ADR-SRS-016-data-classification-external-user-internal.md` | SRS 参照 |
| `docs/decisions/ADR-SRS-017-internal-transaction-category.md` | SRS 参照 |
| `docs/decisions/research/cpp-opencv-migration-research.md` | SRS 参照 |
| `ref/SOURCES.md` | URD/SRS への参照 |
| `mgmt/plan.md`・`mgmt/todo.md`・`mgmt/plan_*.md` | 旧パス参照（devel 専用、参考リンクとして更新） |

`mgmt/tracker/data/issues.json` に旧パスが含まれる場合は触らない方針（履歴は当時の状態を保持。新規記載分のみ新パスを使う）。

### 3. `CLAUDE.md` に採番ルール節を新設

「### 各ステージの成果物」節の直前または直後に、以下の節を追加する:

- 節タイトル例: 「### ベースラインドキュメント採番ルール」
- 内容: カテゴリ区分表（10 番台ベース）、本体 X0 / 付随 X1〜X9 の運用、予備 90 番台の使い方、対象範囲の定義
- 既存の「各ステージの成果物」表は新番号（`docs/10_URD.md`・`docs/20_SRS.md`・以降の HLD = `docs/30_HLD.md` …）に書き換え
- 「常駐ドキュメント」リストの `docs/environment.md` を `docs/01_environment.md` に修正

### 4. 検証

- `grep -rn 'environment\.md' --include='*.md' /workspace` の結果が `docs/01_environment.md` への参照のみであること。
- `grep -rn '01_URD\.md\|02_SRS\.md' --include='*.md' /workspace` がヒットしないこと（旧番号残存ゼロ）。
- `ls docs/*.md` に新番号のファイルが揃う: `00_GLOSSARY.md`, `01_environment.md`, `10_URD.md`, `20_SRS.md`。
- `CLAUDE.md` の「各ステージの成果物」表が新番号に更新され、採番ルール節が追加されている。
- 主要 ADR の内部リンクをいくつか開いて切れていないことを目視確認。

### 5. コミット

ドキュメント整備一式を 1 コミットにまとめる（個別ファイル指定で `git add`）:
- 変更内容: ファイルリネーム 3 件 + 参照更新 + CLAUDE.md 採番ルール追加
- メッセージ例: `docs: ベースラインドキュメント採番ルール整備（environment 等を新体系へ）`

---

## 影響範囲まとめ

| 項目 | 件数 |
|---|---|
| ファイルリネーム | 3 |
| 参照更新対象ファイル | 約 15 |
| `CLAUDE.md` 新規追加節 | 1 |
| 既存仕様への影響 | なし（純粋にドキュメント整備） |

## 将来拡張の例

| 番号 | 想定 |
|---|---|
| `02_xxx.md` | 共通の新規常駐ドキュメント（例: コーディング規約） |
| `11_URD_stakeholder.md` | URD 付随（ステークホルダー分析） |
| `21_SRS_RTM.md` | SRS 付随（要件追跡マトリクス、ISSUE-032 で独立ファイル化する場合） |
| `31_HLD_viewer_design.md` | HLD 付随（ビューア UI 詳細設計） |
| `90_<title>.md` | カテゴリ未確定の暫定配置 |
