# 計画: markdown lint の全カバー化・既存違反の一括掃除・恒久ルール化

## Context（なぜ）

ユーザー要望「markdown lint で見つかった違反は全て修正する様にしたい」。現状の課題:

- `make lint-md` の対象は `docs/` のみ。docs/ はクリーンだが、それ以外（mgmt/・ref/・README.md・CLAUDE.md・.claude/）に違反が残る
- `pymarkdown fix` の自動修正は MD012 等ごく一部のみ。頻出の MD022/MD032/MD034 は未対応（公式に MD032 は「v1.0.0 後対応予定」と確認）。よって手作業で直すしかない

ゴール: (1) lint 対象を現用 markdown 全体へ拡張、(2) 既存違反を一括掃除、(3)「md 編集時は lint を通してからコミット」を CLAUDE.md に明文化して再発防止。

## スコープ決定（ユーザー確認済み）

- lint 対象 = リポジトリ内の全 tracked `.md`。ただし **`mgmt/archive/` は除外**（凍結された過去スナップショット。約101件の違反はここに集中するが、履歴文書を今更整形しない方針）
- enforcement = **CLAUDE.md ルール**（pre-commit フックは作らない）

## 対象ファイルと変更内容

### 1. lint 対象の拡張（Makefile）

`Makefile` の `lint-md` ターゲットを `docs/` 固定から「archive を除く全 tracked md」へ変更する。

- 既定対象を `git ls-files '*.md' ':!:mgmt/archive/**'` から得る（新規 md も自動対象化・gitignore 尊重・archive 除外）
- pymarkdown と自作 `scripts/lint_docs.py` の双方へ同じファイル列を渡す
- 先頭コメントと `LINT_MD_PATHS` 既定値を更新（明示指定時は従来どおり上書き可）

### 2. 既存違反の掃除（現用ドキュメント 計41件）

判断系は本計画で方針確定済みのため、全て機械的なテキスト編集として適用できる。

機械的修正（空行・タブ・URL）:

- `ref/SOURCES.md`（MD034 ×11）: 裸 URL を `<...>` で囲む
- `mgmt/tracker/CLAUDE.md`（MD032 ×4: L26/L33/L39/L84）: リスト前後に空行
- `README.md`（MD022 ×2: L5/L70 / MD032 ×1: L6）: 見出し下・リスト前後に空行
- `mgmt/todo.md`（MD012 ×1: L172 / MD022 ×1: L173）: 連続空行の解消・見出し周りの空行
- `mgmt/lessons.md`（MD022 ×1: L55 `## Patterns to Avoid` 直後）: 見出し下に空行

判断確定済み（Sonnet が機械適用可）:

- `.claude/rules/architecture.md`
  - MD041（L12）: 先頭見出しが H1 でない → 見出しを1段昇格する。`## アーキテクチャ` を `# アーキテクチャ` にし、配下の全 `### X` を `## X` へ（7件）
  - MD040（L16 データフロー図・L68 ディレクトリツリー）: 言語 `text` を付与
- `mgmt/tracker/CLAUDE.md`
  - MD040（L48 ディレクトリツリー・L62 ステータス遷移図）: 言語 `text` を付与

### 3. mgmt/plan.md の置換

本計画は承認後 `mgmt/plan.md` へ移動し旧内容を置換する（プロジェクト規約）。これにより現 `mgmt/plan.md` の違反（MD022/MD031/MD010 計15件）と壊れリンクは同時に解消する。

### 4. 恒久ルールの明文化（/workspace/CLAUDE.md）

「ドキュメント更新時のルール」付近へ簡潔に追記する。

- markdown を編集したら、コミット前に `make lint-md` を実行し、検出された違反は編集対象外でも全て修正してからコミットする
- lint 対象は archive を除く現用 md 全体（`mgmt/archive/` は凍結のため除外）

## モデル運用

判断要素は本計画で解決済み。実行は全て機械的なテキスト編集のため **Sonnet 推奨**。

## 検証

1. `make lint-md` が exit 0（pymarkdown と自作 lint の両方が通る）
2. `git ls-files '*.md' ':!:mgmt/archive/**'` の各ファイルを scan し違反ゼロを確認
3. 主要編集ファイルの `git diff` を目視し、整形で本文が壊れていないか確認
4. `/claude-md-panel`（引数なし）で /workspace/CLAUDE.md の新ルールを確認
5. Conventional Commits 形式・本文日本語でコミット（ドキュメント更新即コミット規約に従う）
