# 管理系ドキュメント・設定の改善プラン

## Context

Opus/Sonnet（および Fable）が効率良く作業できるよう、CLAUDE.md・管理文書・許可設定を整備する。調査の結果、(a) モデルが誤コマンドを打つ原因になる古い記述、(b) 毎セッションのコンテキストを無駄に消費する構造、(c) 許可プロンプトの頻発、(d) モデル世代の表記遅れ、の4種の問題を確認した。ユーザーは全項目の実施を承認済み。

## 1. 実害修正

### 1-1. `mgmt/tracker/CLAUDE.md` のコマンド修正
- 冒頭の `cd /workspace/.claude/mgmt/tracker` 行を削除
- 全コマンド例（バグ管理・課題管理・自然言語依頼例テーブル）の `python3 track.py` を `venv/bin/python3 mgmt/tracker/track.py` に統一（/workspace から実行する前提）

### 1-2. `mgmt/lessons.md` の整理（内容はアーカイブに全保全、消失させない）
- `mgmt/archive/lessons_archive.md` を新設し、以下を移動:
  - 解決済みバグの詳細記録（dem10b 座標変換、13b1f72、コル検出、dem10b URL 誤設定、terrain PNG 格子バグ）
  - パフォーマンス実績（9メッシュ解析メモリ実測）→ 教訓1行だけ lessons.md に残す
  - 「未解決の課題」セクション全体（BUG-001〜005 相当はトラッカー登録済みで二重管理。dem10b 404 = GSI 未整備の事実は知識として lessons.md 設計判断セクションに1〜2行で残す）
- lessons.md に残すもの: アルゴリズム・実装の現役教訓、設計判断、プロセス、設計・仕様、**Patterns to Avoid（全文維持）**
- 冒頭に「履歴・解決済み詳細は archive/lessons_archive.md」の注記を追加

### 1-3. 旧 plan ファイルのアーカイブ
- `mgmt/archive/` を作成し `git mv` で移動:
  - `plan_v1.md`, `plan_urd_draft.md`, `plan_2026-05-14_issues_review.md`, `plan_2026-05-14_match_status.md`, `plan_2026-05-20_issue-020-discussion.md`, `plan_2026-05-20_kyushu-shikoku-analysis.md`, `keycol_analysis.md`
- 現 `mgmt/plan.md`（ISSUE-075・対応完了済み）もアーカイブへ移し、本プランを `mgmt/plan.md` に配置（グローバル CLAUDE.md の plan.md 置き場ルールに従う）
- `grep -rn "plan_v1\|plan_2026\|keycol_analysis" docs/ mgmt/tracker/ CLAUDE.md` で参照残存ゼロを確認（ADR は mgmt/ 参照禁止ルールなので原則ないはず）

### 1-4. 残骸削除
- `rm -rf mgmt/bug-system mgmt/issue-system`（git 未管理・旧 xlsx 2個のみ）

## 2. 許可リスト整備

`/workspace/.claude/settings.local.json` を編集:
- 削除: 一回限りの古い許可（`git mv` 4件、`mkdir -p /workspace/.claude/mgmt/...`、awk 2件）
- 維持: `Bash(make *)`、`WebFetch(domain:cyberjapandata.gsi.go.jp)`
- 追加（読み取り系・定常作業系）:
  - `Bash(venv/bin/python3 mgmt/tracker/track.py *)` ※実行タイミングは欠陥管理フロー等のプロセスルールが引き続き拘束
  - `Bash(git status*)`, `Bash(git log*)`, `Bash(git diff*)`, `Bash(git show*)`, `Bash(git ls-files*)`
  - `Bash(date *)`（handover ファイル名取得用）
  - `Bash(ls *)`, `Bash(grep *)`, `Bash(find *)`, `Bash(wc *)`

## 3. ルーティン軽量化 + CLAUDE.md 分割

### 3-1. セッション開始ルーティンの緩和（`/home/node/.claude/CLAUDE.md`）
- 手順1を変更: 「`.claude/handovers/` の**最新1件**を読む。継続作業で文脈が不足する場合のみ過去分（最大1週間）を古い順に遡る」

### 3-2. 文書系ルールを `docs/CLAUDE.md` へ分離（`/workspace/CLAUDE.md` 427行の常駐削減）
- `docs/CLAUDE.md` を新設し、以下のセクションを移動:
  - ベースラインドキュメント採番ルール / 各ステージの成果物 / GLOSSARY と SOURCES の役割分担 / 外部I/F・ユーザー入力・内部データ・内部トランザクションの分類ルール / ドキュメントフォーマット標準 / ADR 管理ルール
- ルート CLAUDE.md には1行ポインタを残す: 「docs/ 配下の編集時は採番・フォーマット・ADR ルールを `docs/CLAUDE.md` で確認」
- ルートに残す: 仕様優先原則、ドキュメント・実装整合性原則、ドキュメント更新時のルール（即commit）、欠陥/課題/ToDo/handover/ブランチ運用、ビルド・アーキテクチャ
- 効果: docs/ を触るときだけ自動ロードされ、常駐コンテキストが約半減

## 4. Fable 表記追従

- `/home/node/.claude/CLAUDE.md` コア原則5: 「Opus は調査・設計・判断フェーズ用」→「Opus / Fable は調査・設計・判断フェーズ用」等、Fable を含めた表現に更新
- `/workspace/CLAUDE.md` の `--actor` 表記ルール: 「Sonnet」「Opus」に「Fable」を追加

## 検証

1. `grep -rn "python3 track.py" mgmt/tracker/CLAUDE.md` → `venv/bin/python3` 以外ゼロ
2. `grep -rn "/workspace/.claude/mgmt" mgmt/ CLAUDE.md` → ゼロ
3. lessons.md + lessons_archive.md の合計内容に欠落がないこと（移動のみ・削除はトラッカー重複分のみと明示確認）
4. 旧 plan ファイルへの参照残存ゼロ（grep）
5. `docs/CLAUDE.md` とルート CLAUDE.md で重複・欠落セクションがないこと
6. `venv/bin/python3 mgmt/tracker/track.py issue list --open` が新コマンド表記どおり動くこと
7. `git status` で意図したファイルのみ変更されていること

## コミット

- ユーザーの方針（コミットは最後にまとめて）に従い、作業完了後にまとめてコミット
- 個別ファイル指定で `git add`（`git add .` 禁止）。Conventional Commits・本文日本語
  - 例: `chore(mgmt): トラッカー運用ガイドのパス修正・lessons整理・旧planアーカイブ`、`docs: 文書系ルールを docs/CLAUDE.md に分離`
- グローバル CLAUDE.md（`/home/node/.claude/CLAUDE.md`）と settings.local.json は git 対象外（リポジトリ外/gitignore）
