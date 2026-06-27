# 全文書「ユーザ」→「ユーザー」・「キャッシュ」→「ローカルキャッシュ」表記統一

## Context

FR-002 の SRS レビュー（2026-06-06 1154 セッション）で、用語表記の揺れが2点合意され、別 ISSUE として残された：

1. **「ユーザ」と「ユーザー」の混在**
   - 「ユーザ入力」分類名・本文の地の文ともに揺れている
   - 「ユーザー」が一般的（JIS Z 8301）かつプロジェクト内多数派（63件 vs 42件）
2. **「キャッシュ」が指す対象の曖昧さ**
   - プロジェクトの標高タイルキャッシュ・ブラウザ HTTP キャッシュ・JavaScript Map キャッシュ・分類概念としての「永続キャッシュ」が同じ語で混在
   - 最低限、本プロジェクトのタイルキャッシュは「ローカルキャッシュ」に統一し、他のキャッシュ概念と区別できるようにする

**ゴール**: 公式文書（docs/, CLAUDE.md, ref/SOURCES.md）と管理文書（mgmt/）の用語を統一し、読者が同じ概念に同じ語を期待できる状態にする。

**ユーザー確認済の方針（決定事項）**:
- スコープ: 公式文書 + 管理文書（**コードは対象外** — src/, scripts/, analysis/, tests/, docs/mockup/ は触らない）
- キャッシュ: **プロジェクトの標高タイル**を指す箇所のみ「ローカルキャッシュ」に置換。ブラウザ HTTP キャッシュ・Map キャッシュ・分類概念「永続キャッシュ」は据え置き
- 「ユーザ入力」セクション名・分類名も「ユーザー入力」に変更し、アンカーリンクも同時更新

---

## 作業手順

### Step 1: 「ユーザ」→「ユーザー」全件置換（42件）

全件目視確認済。**すべて機械置換で問題なし**（意図的な「ユーザ」なし、外部引用・固有名詞なし）。

対象ファイル別件数:
| ファイル | 件数 |
|---|---|
| `docs/02_SRS.md` | 14 |
| `docs/decisions/ADR-SRS-016-data-classification-external-user-internal.md` | 12 |
| `mgmt/plan.md` | 6 |
| `CLAUDE.md` | 4 |
| `docs/00_GLOSSARY.md` | 2 |
| `mgmt/tracker/data/issues.json` | 1（ISSUE-076 タイトル） |
| `mgmt/lessons.md` | 1 |
| `docs/decisions/ADR-SRS-017-internal-transaction-category.md` | 1 |
| `docs/decisions/ADR-SRS-014-srs-parameter-description-convention.md` | 1 |

**注**: ADR-SRS-016 のファイル名 `ADR-SRS-016-data-classification-external-user-internal.md` は英語スラッグなので変更しない（タイトルの「ユーザ入力」のみ「ユーザー入力」に変更）。

### Step 2: SRS リンクアンカー追従

「ユーザ入力」→「ユーザー入力」によりアンカーが変わる箇所を本文と同時に更新する。

`docs/02_SRS.md`:
- 行74 `[ユーザ入力と内部データ](#7-ユーザ入力と内部データ)` → `[ユーザー入力と内部データ](#7-ユーザー入力と内部データ)`
- 行75 `[7.1 ユーザ入力一覧](#71-ユーザ入力一覧)` → `[7.1 ユーザー入力一覧](#71-ユーザー入力一覧)`
- 行107 `[7.1 ユーザ入力一覧](#71-ユーザ入力一覧)` → 同上

他にアンカー参照箇所が無いことを Step 4 で `grep -rnP "#7-?ユーザ(?!ー)|#71-?ユーザ(?!ー)"` で確認。

### Step 3: 「キャッシュ」文脈別置換

全件文脈確認済。以下の分類で個別に処理する。

#### 3a. 「ローカルキャッシュ」へ置換する箇所（プロジェクトの標高タイルキャッシュ）

| ファイル:行 | 旧 | 新 |
|---|---|---|
| `ref/SOURCES.md:53` | タイルを**ローカルにキャッシュ**して | タイルを**ローカルキャッシュとして保存**して |
| `CLAUDE.md:82` | キャッシュ済みタイル | ローカルキャッシュ済みタイル |
| `CLAUDE.md:83` | 未キャッシュ時 | ローカルキャッシュ未取得時 |
| `CLAUDE.md:101` | キャッシュ参照のみ | ローカルキャッシュ参照のみ |
| `CLAUDE.md:162` | ダウンロード済みタイルのキャッシュ | ダウンロード済みタイルのローカルキャッシュ |
| `docs/02_SRS.md:154` | 国土地理院から取得・キャッシュ / キャッシュ済み PNG タイル | 国土地理院から取得・ローカルキャッシュ / ローカルキャッシュ済み PNG タイル |
| `docs/02_SRS.md:156` | キャッシュ済み PNG タイル | ローカルキャッシュ済み PNG タイル |
| `docs/02_SRS.md:240` | ローカルにキャッシュする | ローカルキャッシュとして保存する |
| `docs/02_SRS.md:254` | 標高タイル（キャッシュ） / キャッシュ保存先 | 標高タイル（ローカルキャッシュ） / ローカルキャッシュ保存先 |
| `docs/02_SRS.md:261` | キャッシュ済みタイル | ローカルキャッシュ済みタイル |
| `docs/02_SRS.md:1009` | キャッシュタイルの mtime | ローカルキャッシュタイルの mtime |
| `docs/02_SRS.md:1186` | 同一タイルキャッシュ | 同一ローカルキャッシュ |
| `docs/02_SRS.md:1416` | 標高タイル（キャッシュ） | 標高タイル（ローカルキャッシュ） |
| `docs/decisions/ADR-URD-005-northern-territories-exclusion.md:97` | タイルキャッシュ | ローカルキャッシュ |
| `docs/decisions/ADR-SRS-015-contour-overlay.md:21` | プロジェクトの prefetch キャッシュ | プロジェクトの prefetch ローカルキャッシュ |
| `docs/decisions/ADR-SRS-016-...md:32` | キャッシュ保存先 | ローカルキャッシュ保存先 |

#### 3b. 据え置く箇所（別概念のキャッシュ）

| ファイル:行 | 文字列 | 据え置き理由 |
|---|---|---|
| `docs/decisions/ADR-SRS-015:21` | ブラウザ HTTP キャッシュ | ブラウザ標準のHTTP層キャッシュ |
| `docs/decisions/ADR-SRS-015:29` | Map キャッシュ（上限 300 件） | JavaScript の `Map` オブジェクト |
| `docs/decisions/ADR-SRS-015:50` | ブラウザ標準 HTTP キャッシュ | 同上 |
| `docs/00_GLOSSARY.md:122` | 永続キャッシュ・ブラウザ永続化 | 分類概念（ローカルキャッシュを含む上位カテゴリ） |
| `CLAUDE.md:215` | 永続キャッシュ・ブラウザ永続化 | 同上 |
| `docs/02_SRS.md:1401` | 永続キャッシュ・ブラウザ永続化 | 同上 |
| `docs/decisions/ADR-SRS-014:83` | 永続キャッシュ・ブラウザ永続化 | 同上 |
| `docs/decisions/ADR-SRS-016:22` | 永続キャッシュ・ブラウザ永続化 | 同上 |
| `mgmt/plan_v1.md:72` | タイルキャッシュディレクトリ | 旧計画の履歴ファイル（_v1 接尾辞）- 触らない |
| `docs/mockup/viewer_mockup.html:770` | キャッシュ（JS コメント） | コード扱い・対象外 |
| `docs/figures/phases_overview.drawio*` | キャッシュ | 図のラベル。SVG/drawio両方の整合が必要なので別 ISSUE で扱う |

**mgmt/plan.md と mgmt/tracker/data/issues.json の扱い**:
- `mgmt/plan.md:193-196` は本タスクの計画記述そのもの → 履歴として残す（実施時に「完了」記述に書き換える等は別途）
- `mgmt/tracker/data/issues.json` のキャッシュ言及3件は、本文内容として「ローカルキャッシュ」へ置換

### Step 4: 整合性確認

```bash
# 「ユーザ」（直後ー無し）残存ゼロ確認
grep -rnP "ユーザ(?!ー)" docs/ CLAUDE.md mgmt/plan.md mgmt/lessons.md mgmt/tracker/data/issues.json ref/SOURCES.md

# 旧アンカーリンク残存ゼロ確認
grep -rnP "#7-?ユーザ(?!ー)|#71-?ユーザ(?!ー)" docs/

# 「キャッシュ」据え置き箇所が想定通りか確認
grep -rnP "(?<!ローカル)キャッシュ" docs/ CLAUDE.md mgmt/plan.md mgmt/lessons.md mgmt/tracker/data/issues.json ref/SOURCES.md
# 想定残存件数: 約9件（永続キャッシュ系 + ブラウザHTTP/Map系 + plan.md計画記述）
```

### Step 5: トラッカー・コミット

- `mgmt/tracker/data/issues.json` 直接編集後、`venv/bin/python3 mgmt/tracker/track.py issue export --if-changed` で xlsx 再生成
- 本作業を新規 ISSUE として `track.py issue add` で起票 → `issue close` → ユーザー verify 後 `issue verify`
- Conventional Commits（`docs:` プレフィックス）、本文日本語、ファイル個別指定で commit
- push は別途指示があるまで不要

---

## 対象ファイル一覧（編集確定）

- `docs/02_SRS.md`（ユーザ14 + キャッシュ9）
- `docs/00_GLOSSARY.md`（ユーザ2）
- `docs/decisions/ADR-SRS-014-srs-parameter-description-convention.md`（ユーザ1）
- `docs/decisions/ADR-SRS-015-contour-overlay.md`（キャッシュ1 ← 21行目のみ）
- `docs/decisions/ADR-SRS-016-data-classification-external-user-internal.md`（ユーザ12 + キャッシュ1 ← 32行目）
- `docs/decisions/ADR-SRS-017-internal-transaction-category.md`（ユーザ1）
- `docs/decisions/ADR-URD-005-northern-territories-exclusion.md`（キャッシュ1）
- `CLAUDE.md`（ユーザ4 + キャッシュ4）
- `ref/SOURCES.md`（キャッシュ1）
- `mgmt/plan.md`（ユーザ6 ← 計画記述部分はそのまま、対象は本文の地の文のみ）
- `mgmt/lessons.md`（ユーザ1）
- `mgmt/tracker/data/issues.json`（ユーザ1 + キャッシュ3）

**触らないファイル**:
- `src/*.c`, `src/*.h`, `scripts/*.py`, `analysis/*.py`, `tests/*.c`（コード対象外）
- `docs/mockup/viewer_mockup.html`（コード扱い）
- `docs/figures/phases_overview.drawio*`（図ファイル・別 ISSUE で対応）
- `mgmt/plan_v1.md`（履歴）
- `.claude/handovers/`（gitignore）

---

## Verification

1. **grep 残存ゼロ**: Step 4 の3コマンドを実行し、想定どおりであることを確認
2. **SRS リンク踏み確認**: `docs/02_SRS.md` を Markdown レンダリング（GitHub）で開き、目次「7. ユーザー入力と内部データ」をクリックして該当セクションへ正しくジャンプすることを確認
3. **GLOSSARY 整合性**: `docs/00_GLOSSARY.md` の「ユーザー入力」「ローカルキャッシュ」定義を本文と相互参照し、語の意味が一貫していることを確認
4. **トラッカー xlsx 整合性**: `venv/bin/python3 mgmt/tracker/track.py issue show 076` でタイトルが「外部I/F・ユーザー入力・内部データの三分類規約とSRS再編」になっていること

---

## 注意点（実装時に守る）

- 既存「ローカルキャッシュ」表記との重複（例: 「ローカルローカルキャッシュ」）を作らないため、置換時は前後文字列を確認する（grep `-P` の負の先読み `(?<!ローカル)` で安全に検出可能）
- 「ユーザ」一括置換ツールを使う場合は、Python の `re.sub(r"ユーザ(?!ー)", "ユーザー", text)` のように負の先読みで安全に処理する
- 「ユーザ入力」→「ユーザー入力」と「ユーザ」→「ユーザー」の置換順序は逆順（「ユーザ」を先にすると「ユーザー入力」が「ユーザーー入力」になる可能性）。負の先読みを使えば順序問題は発生しないが、念のため `(?!ー)` を使う
- `mgmt/tracker/data/issues.json` は JSON 構造を壊さないよう、Edit ツールで該当箇所のみピンポイント置換する
