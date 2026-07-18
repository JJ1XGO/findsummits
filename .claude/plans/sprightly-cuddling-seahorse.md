# findsummits#29: 本体/ops リポジトリ分離（findsummits-ops 新設）

## Context

`jj1xgo/findsummits` は **public** リポジトリであり、メンテナ個人の開発運用ファイル
（`CLAUDE.md`・`.claude/` 配下）がプロダクト本体と混在して公開・追跡されている
（総コミット 868 件中 `CLAUDE.md` 81 件・`.claude/` 126 件）。claude-container・
sotlas-frontend で確立済みの標準形「`.claude/` を独立 git リポジトリ（nested repo）化し、
private の `jj1xgo/findsummits-ops` で管理する」を findsummits にも適用する。

手順の正はプレイブック `~/.claude/reference/ops-separation-notes.md`（§2 実施手順・
§4 Phase 4 検証チェックリスト・§5 ハマりどころ・§6 findsummits 棚卸し）。

## 決定事項

**issue #29 で決定済み（2026-07-17）**:

- ops リポジトリ: `jj1xgo/findsummits-ops`（private）、ブランチ名 `main`
- `lessons.md`・`handovers/`・`incidents/` は ops で git 追跡する（cc 方式、区切りでまとめてコミット）

**本セッションでユーザー確認済み（2026-07-18）**:

- 分離は `devel` ブランチで実施（`CLAUDE.md`・`.claude/` は devel にしか存在せず、main はほぼ空）
- `mgmt/` は本体に残す（docs/ 仕様駆動プロセスと密結合。リリース時の `git rm -r mgmt/` フローが main への露出を既に防止）
- `.claude-container.d/` の選択追跡を本体で新設（`allowed-domains.txt`・`packages.txt`・`requirements.txt` の3ファイル。`env` は除外維持）
- session-start.sh への UPSTREAM_BASE マーカー整備は今回見送り（`todo.md` へ追記し別セッションで実施）

## 事前確認済みの事実（2026-07-18 実測）

- リポジトリ可視性: PUBLIC（`gh repo view`）
- 分離対象の追跡ファイル: 14 件（`.claude/` 13 + ルート `CLAUDE.md`）
- hooks 5 本は全て `$CLAUDE_PROJECT_DIR` 基準または自己位置解決で、nested repo 化の影響なし（Phase 4 で実機確認はする）
- `CLAUDE.md` の `@.claude/best_practices.md` インポートは 175 行目（要書き換え）。本文中の `.claude/...` 参照はプロジェクトルート基準の記述のため原則据え置き
- `make lint` の md/py/geojson/html target は `git ls-files` 依存 → 分離後 `.claude/` 配下 `.md` が対象から抜ける（cc と同型。`.claude/` 配下に py/geojson/html は無いため **md target のみ拡張**）
- 履歴の秘密情報 予備スキャン（ghp_/github_pat_/sk-/PRIVATE KEY/password= パターン）: 検出なし
- ops へ新規追跡する管理外ファイルの分量: handovers/ 382 件 2.8MB・incidents/ 292KB・lessons.md 52KB
- ルート `CLAUDE.md` への path 参照で要修正: `.claude/todo.md:6`（`/workspace/CLAUDE.md` 絶対パス）・`.claude/README.md:127`（`../CLAUDE.md` リンク）

## 実行モデル

全タスクは確立済み手順（プレイブック）の機械的実施のため **Sonnet 既定**（上位モデル委譲対象なし）。
計画承認後、Fable → Sonnet への `/model` 切替を提案する。

## 手順

### Phase 0: 準備 [container]

1. 本計画ファイルを devel へコミット（承認後の必須手順）。このコミットの SHA を**ロールバック起点**として記録する
2. `git status` 確認（未追跡の `analysis/スクリーンショット_*.png` は分離と無関係のため放置可。追跡ファイルに未コミット差分が無いことを確認）
3. 履歴の秘密情報 本スキャン: `git log -p --all -- .claude CLAUDE.md` を拡張パターン（AKIA/ssh-rsa/token= 実値等を追加）で走査。加えて ops へ新規コミットする `lessons.md`・`handovers/`・`incidents/` の現物も同スキャン（private だが衛生確認）。検出時は停止しユーザーへ報告
4. `/handover` を実行（分離直前の状態保全）

### Phase 1: ops リポジトリ作成 [host・ユーザー手動]

- `gh repo create jj1xgo/findsummits-ops --private` をユーザーへ依頼（push はまだしない）。コマンドは 1 つずつ提示する

### Phase 2: 本体側の分離処理 [container]

1. `.gitignore` 編集:
   - `.claude/` 個別除外 8 行（settings.local.json / conversations/ / sessions/ / skills/.cache/ / handovers/ / incidents/ / bash_history / lessons.md）を削除し、理由コメント＋ `CLAUDE.md` / `.claude/` の 2 行に置換（「メンテナ個人の開発運用ファイル」コメントは cc/sotlas の原文流用）
   - `.claude-container.d/` 行を `.claude-container.d/env` に変更（理由コメントも「env はホスト固有パス含むランタイム設定のため除外、他は第三者にも有用な環境情報として追跡」へ更新）
   - `mgmt/spec-findings/` 等、本体側の他エントリは据え置き
2. `git rm -r --cached .claude` ＋ `git rm --cached CLAUDE.md`（ワークツリー維持のまま追跡解除）
3. `.claude-container.d/` の 3 ファイルを `git add`。`git check-ignore .claude-container.d/env` で env が確実に無視されることを実物確認
4. 本体コミット×2: (a) `.gitignore`＋追跡解除、(b) `.claude-container.d/` 選択追跡新設
5. `mv CLAUDE.md .claude/CLAUDE.md`（追跡解除済みのため `mv` のみ）。内容調整:
   - 175 行目 `@.claude/best_practices.md` → `@best_practices.md`
   - 「課題管理ルール」節へ振り分け基準を追記: 変更対象ファイルの所属リポジトリで機械判定（本体配下 → `jj1xgo/findsummits`、`.claude/` 配下 → `jj1xgo/findsummits-ops`、`~/.claude/` → `jj1xgo/dotclaude-ops`）。署名は現行グローバル規約に従う
   - 「計画ファイル・handover の扱い」節へ追記: `.claude/plans/` は物理的に ops 配下のため git 操作（承認後コミット・完了時 `git rm`）は常に `git -C .claude` で ops 側に対して行う
   - 「ドキュメント更新時のルール」対象リストの `.claude/plans/*.md` にコミット先が ops である旨を注記
6. `git -C /workspace/.claude init -b main`（cwd 相対パスの二重解決事故防止のため絶対パス・`-C` 形式を使う）
7. ops 用 `.claude/.gitignore` 新規作成: settings.local.json / conversations/ / sessions/ / skills/.cache/ / bash_history（**lessons.md・handovers/・incidents/ は追跡対象のため含めない**）
8. ops fresh initial commit: `.claude/` 配下全対象（既存 13 ファイル＋CLAUDE.md＋lessons.md＋handovers/＋incidents/＋本計画ファイル）。コミットメッセージに移管元 `jj1xgo/findsummits@<Phase 0 の SHA>` を明記
9. `Makefile` markdown target 拡張: `git ls-files '*.md'` の結果に `git -C .claude ls-files '*.md'`（`.claude/` プレフィックス付与）を連結（ops 側 md を lint 対象に維持。py/geojson/html は `.claude/` に存在しないため md のみ、コメントで明記）→ 本体コミット
10. `.claude/README.md` 更新: CLAUDE.md の位置（ルート → `.claude/CLAUDE.md`）、`../CLAUDE.md` リンク修正、nested repo 構成の説明、役割分担表の更新 → ops コミット
11. `.claude/todo.md` へ UPSTREAM_BASE マーカー整備タスクを追記 → ops コミット
12. `make lint` 全体実行で警告ゼロ確認（拡張後の lint が ops 側 md を拾うことも実物確認）

### Phase 3: push [host・ユーザー手動]

1. push 前に `git status`・`git log`（本体・ops 両方）をユーザーへ提示して確認
2. ops → 本体の順で push をユーザーへ依頼（1 コマンドずつ提示）:
   - ops: remote 追加 → `push -u origin main`
   - 本体: `git push origin devel`
3. コンテナ内から ops の push 反映確認は構造的に不可（プレイブック §5-5）のため、ホスト側 push 出力のハッシュ遷移を実物確認として採用

### Phase 4: 実機検証 [新規セッション]

hook 設定はセッション開始時スナップショットのため**必ず新規セッションで**実施:

1. `.claude/CLAUDE.md` が「project instructions」としてシステムプロンプトに注入されているか（最重要）
2. `@best_practices.md` インポートの解決（不可なら `@.claude/best_practices.md` へ戻す）
3. session-start hook の自動注入（handover・issue 一覧）が通常どおりか
4. `plansDirectory`（`.claude/plans/` へ生成。`~/.claude/plans/` フォールバックしていないか）
5. サンドボックス書込保護パスの挙動が分離前と同じか
6. findsummits 固有: hooks 5 本の発火確認（model-guard / spec-panel-gate / lint-posttool / docs-date-check / session-start）、特に lint-posttool の `.claude/` 配下ファイル編集時動作と spec-panel-gate の `mgmt/spec-findings/`（本体側）参照
7. `make lint` 警告ゼロ（ops 側 md を含む）

### Phase 5: クローズ

1. Phase 4 全項目の確認結果を issue #29 へコメント（署名 `— <モデル名> (jj1xgo/findsummits)`）し、findsummits 側で `gh issue close`（issue 記載の完了条件どおり）
2. 本計画ファイルを `git -C .claude rm` で削除し ops へコミット
3. `/handover`

## ロールバック

- push 前: 全てローカル操作。本体は `git reset --hard <Phase 0 の SHA>`、ネスト化の取り消しは `rm -rf /workspace/.claude/.git`（**`git clean -fd .claude` は handovers/incidents も消すため使用禁止**）
- push 後: force push せず修正コミットを積む

## 検証（要約）

- Phase 2 完了時: `git status`（本体・ops 双方）と `make lint` 警告ゼロを実物確認
- Phase 3: push 出力のハッシュ遷移を実物確認
- Phase 4: 上記チェックリスト全項目を新規セッションで消化し、結果を issue #29 へ記録
