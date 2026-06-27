# 計画: main 文書からの内部トラッカー ID（ISSUE-XXX）排除

## Context（背景・目的）

`mgmt/tracker/`（ISSUE 実体 `issues.json`）は devel ブランチでのみ git 管理され、リリース時に
`git rm -r mgmt/` で main から除外される（`docs/CLAUDE.md:51`）。
一方、main にも持っていくベースライン文書（URD/SRS/ADR/research）には `ISSUE-094` のような
**内部トラッカー ID がテキストとして45行・18ファイルに散在**している。

第三者が main を見たとき、これらの ID は辿る先（トラッカー）が存在せず**意味を成さない**。
ユーザー方針: 「今のうちにベースライン文書から ISSUE-ID を排除する。mgmt の git 管理は維持する」。

ID は単なる経緯タグであり、決定内容そのものは ADR/SRS 本文に self-contained で記録済み
（課題運用ルール「issue のスコープ＝決着の ADR/SRS 記録まで」がこれを担保）。
よって ID を除去しても情報は失われない。日付・文脈は本文に残す。

### 確定事項（ユーザー合意済み）

- 排除対象: **main に出る docs 全体**（URD/SRS + ADR + research、ファイル名含む）
- mgmt/tracker の git 管理: **維持**（docs から ID を消せば main には出ない。devel 履歴・バックアップは保つ）

### 現状の問題2層（調査結果）

- **第1層（既存ルール違反）**: `docs/decisions/ADR-SRS-018-...:39` に
  `[ISSUE-056](../../mgmt/tracker/)`・`[ISSUE-079](../../mgmt/tracker/)` の Markdown リンク2件。
  `docs/CLAUDE.md:139`「`mgmt/` へのパス参照禁止」違反。`lint_docs.py` の broken-link 検査は
  `.md` 終端リンクのみ対象（`LINK_RE`）のため末尾 `/` のこれらを見逃している。
- **第2層（テキスト ID）**: 残り43件。リンクではない経緯タグ。lint 非検出。
  大半は括弧内の経緯タグだが、一部は main で意味を失う「生き依存」（後述）。

## 作業タスク

### タスク1: 方針の決定記録 ADR-SRS-040 作成（モデル: Sonnet）

- 新規 `docs/decisions/ADR-SRS-040-...md`（次番号は ADR-SRS-039 の次＝040）。
- 命名・フォーマットは `docs/CLAUDE.md`・`docs/00_GLOSSARY.md`「ADR 命名規約」に従う。
- 記録内容: 決定（main docs から内部 ID 排除・mgmt は git 管理維持）、
  代替案と却下理由（release 時機械除去／何もしない／正式仕様のみ排除）、影響。

### タスク2: 既存リンク違反の修正（モデル: Sonnet）

- `docs/decisions/ADR-SRS-018-...:39` の `[ISSUE-056](../../mgmt/tracker/)`・
  `[ISSUE-079](../../mgmt/tracker/)` を**リンクなしの文脈記述**へ書き換え
  （例: 「実装追従は別途トラッカーで管理」等、ID を出さず意味が通る形）。

### タスク3: docs 全体の ISSUE-ID 除去（モデル: Sonnet）

`grep -rEn "ISSUE-[0-9]+" docs/` の全45件を文脈別に処理:

- **経緯タグ系**（大半）: 括弧内 ID を削除し日付・文脈は残す。
  - 例 `FR-006 レビュー（ISSUE-094）で指摘された` → `FR-006 レビューで指摘された`
  - 例 `改訂注記（2026-06-14 ISSUE-094）` → `改訂注記（2026-06-14）`
  - 例 `（ISSUE-078、2026-06-05）` → `（2026-06-05）`
  - `ADR-SRS-004` 決定日欄の長大な経緯（ISSUE-090 等複数）も同様に ID のみ除去。
- **生き依存系**（内容確認して文言調整）:
  - `docs/20_SRS.md:1152`「生成実装は ISSUE-064 で管理」→ ADR-SRS-032(ISSUE-117) で
    生成 FR は確定済み。確定先 FR を本文参照に置換（要 SRS 該当箇所確認）。
  - `ADR-SRS-013:113-118` ISSUE-043/044「HLD/COD で継続」→ ID を出さず
    「後続ステージで対応」等に一般化。
  - `ADR-SRS-025`・`ADR-SRS-026` の ISSUE-106「で管理」→ 決着先 ADR-SRS-026 への
    本文リンクに置換（ADR 間は Markdown リンク可）。
- **research ファイル**:
  - 本文中の `ISSUE-020` 言及を除去（`issue-020-keycol-threshold-analysis.md`）。
  - ファイル名 `issue-020-keycol-threshold-analysis.md` → `keycol-threshold-analysis.md` に
    `git mv`（履歴維持）。**参照元 `docs/decisions/ADR-SRS-011-...` のリンク1件を更新**。
  - 本文中の `mgmt/archive/plan_2026-05-20_issue-020-discussion.md` 参照は mgmt パスのため除去。

### タスク4: 再発防止 — lint 検査C 追加（モデル: Sonnet）

- `scripts/lint_docs.py` に検査C を追加: 本文中の `ISSUE-\d+` パターンと
  `](.../mgmt/...)` 形式のパス参照を検出して違反報告。
- `mgmt/` 自身・`mgmt/archive` は lint 対象外（既存除外）なので誤検出しない。
  検査対象は `make lint-md` の docs/ スコープ。
- 既存の検査A/B と同じ violations リスト方式で実装。

### タスク5: ルール明文化（モデル: Sonnet）

- `docs/CLAUDE.md` の参照ルール（139行付近）に
  「docs 配下に内部トラッカー ID（`ISSUE-XXX`/`BUG-XXX`）を書かない。経緯は日付・文脈で残す」を明記。
- 既存「`mgmt/` へのパス参照禁止」と並べて整理。

### タスク6: 課題登録（モデル: Sonnet・作業前）

- 本作業は docs/CLAUDE.md 改訂＋ADR 作成（仕様議論）を伴うため `issue add` で登録。
  `venv/bin/python3 mgmt/tracker/track.py issue add ...`（actor はモデル名）。
- 着手時 `--status 対応中`、完了時 `issue close`。

## 検証

- `grep -rEn "ISSUE-[0-9]+" docs/` が **0 件**であること。
- `grep -rn "mgmt/" docs/` に新たなパス参照が残っていないこと。
- `make lint` 警告ゼロ（新検査C 含む）。
- 検査C の発火確認: docs に一時的に `ISSUE-999` を仕込み `make lint-md` で検出されることを確認 → 削除。
- ADR-SRS-040・research リネーム・ADR-SRS-011 参照更新後に broken-link が無いこと。

## コミット方針

- ドキュメント更新は作業ターン内に commit（push は別途指示まで不要）。
- `scripts/lint_docs.py` の変更は docs 修正と同一作業のため同梱可。
- Conventional Commits・本文日本語。

## モデル運用メモ

全タスク **Sonnet** 想定（方針は確定済み・残作業は文書編集と単純な lint 拡張）。
生き依存の言い換え（タスク3）で SRS/ADR の該当箇所確認が必要だが難解ではない。
ExitPlanMode 承認後、`/model` で Sonnet 切替を促してから着手する。
承認後この plan は `.claude/plans/` から `mgmt/plan.md` へ `mv` する。
