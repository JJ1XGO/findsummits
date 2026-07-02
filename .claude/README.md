# .claude/ カスタマイズ目録（このプロジェクト固有）

このプロジェクト（標高解析 SOTA 申請支援ツール）の `.claude/` に配置した
Claude Code カスタマイズの一覧です。

グローバル共通設定との役割分担は `~/.claude/README.md` を参照してください。

---

## 役割分担サマリー（プロジェクト視点）

| 要素 | グローバル `~/.claude/` | このプロジェクト `.claude/` |
|---|---|---|
| [**CLAUDE.md**](#claudemd-の位置) | 全プロジェクト共通ガイドライン | リポジトリルートに配置 |
| [**settings.json**](#settingsjson) | 基盤設定一式 | プロジェクト共通 hooks（Lint・model ガード・SessionStart、git 管理対象） |
| [**settings.local.json**](#settingslocaljson) | 存在しない | 個人環境の permissions allow リスト（git 管理外） |
| [**commands/**](#commands) | 汎用 skill（handover / log-incident / claude-md-panel / update-best-practices） | ドメイン固有 skill（spec-panel） |
| [**rules/**](#rules) | 存在しない | アーキテクチャ定義（`architecture.md`） |
| [**hooks/**](#hooks) | 汎用保護（Write/Edit 検証・注入防止） | `hooks/session-start.sh`（SessionStart）+ `settings.json`（Lint・model ガード） |
| [**incidents/**](#incidents) | 存在しない | 環境異常記録（このプロジェクト配下・git 管理外） |
| [**handovers/**](#handovers) | 存在しない | セッション引き継ぎノート（このプロジェクト配下・git 管理外） |
| [**`plan-*.md`・`todo.md`**](#plan-mdtodomd) | 存在しない | devel 運用ファイル（`.claude/` 直下・git 管理対象） |

---

## プロジェクト要素インベントリ

### `commands/`

`commands/*.md` はスラッシュコマンド（`/<name>` で起動する skill）として Claude Code に現れます。

| No. | ファイル | スラッシュコマンド | 役割 |
|---|---|---|---|
| 1 | `spec-panel.md` | `/spec-panel` | 仕様文書（URD/SRS/HLD/LLD/ADR/テスト仕様）をアーキテクト・仕様レビュアー・データ/アルゴリズム・申請者の4視点でレビューし指摘を一覧化 |

### `rules/`

Claude Code が自動的に読み込むプロジェクトルール定義です。

| No. | ファイル | 役割 |
|---|---|---|
| 1 | `architecture.md` | 本プロジェクトのアーキテクチャ・モジュール設計・ディレクトリ構成・`$DATA_DIR` 配置ルールを定義 |

### `hooks/`

`settings.json` の SessionStart フックから呼び出されるスクリプト群です。

| No. | ファイル | 呼び出し元 | 役割 |
|---|---|---|---|
| 1 | `session-start.sh` | SessionStart hook | handover + lessons を注入。以下2条件のいずれかで `/log-incident` の環境確認チェックリスト実行を Claude へ指示: ①最新インシデントファイルが「未解決」状態、②最新handoverの「環境異常・インシデント」セクションに「なし」以外の記録がある（解決済みインシデントも直後の1セッションで要確認）。また handover の「## 学び」セクション項目を lessons.md と突き合わせ、未転記のものを全件初回返答時に追記するよう Claude へ指示。さらに lessons.md の件数増加をウォーターマーク（`best_practices_watermark`）と比較し、+10件以上で `/update-best-practices` 実行を推奨 |

### `settings.json`

プロジェクト共通の hooks（4件）を定義します（git 管理対象。クローン先でも同じ自動化が効く）。
hook 内のパスは実行時に Claude Code が設定する `$CLAUDE_PROJECT_DIR` で解決し、
実行環境（ローカル / コンテナ）に依存しない。

| トリガー | Matcher | 役割 |
|---|---|---|
| PreToolUse | Write\|Edit | 実装ファイル（`.c/.h/.py/.html`）を Opus/Fable で編集しようとすると警告・permission ask |
| PostToolUse | Write\|Edit | ファイル種別に応じて Lint 実行（Markdown/Python/GeoJSON/HTML）し結果を返送 |
| PostToolUse | Write\|Edit | `docs/*.md` 編集時に `\| 最終更新日 \|` 行が当日付でなければ更新指示を additionalContext で返送（ファイルは直接書き換えない） |
| SessionStart | startup/resume/clear | `hooks/session-start.sh` を呼び出す。handover + lessons を自動注入し、未解決インシデントまたは前handoverに環境異常記録があれば環境確認チェックリスト実行を指示 |

`.claude/` 構成ファイル変更時の README.md 更新リマインドはグローバル hook（`~/.claude/settings.json`）で対応。

### `settings.local.json`

個人環境の permissions allow リスト（実行許可ホワイトリスト）のみを定義します（git 管理外）。

| 許可対象 | 目的 |
|---|---|
| `WebFetch(domain:cyberjapandata.gsi.go.jp)` | 国土地理院の標高タイル取得のみ許可 |
| `Bash(make *)` | Makefile 実行 |
| `Bash(venv/bin/python3 mgmt/tracker/track.py *)` | 課題管理スクリプト |
| `Bash(git status\|log\|diff\|show\|ls-files ...)` | Git 読み取り |
| `Bash(date\|ls\|grep\|wc *)` | 標準ツール（`find` は `-delete`/`-exec` が破壊的になり得るため許可しない） |

### `incidents/`

環境異常（Opus の捏造・hallucination・plan mode 動作不正など）を記録するファイル群。
`/log-incident` コマンドで自動生成される日時タイムスタンプ付きファイル、および
セッション異常時に生成されるセッションID付きファイル（`.md` + `.raw.txt` ペア）が混在。
`.gitignore` 対象（git 管理外）。

### `handovers/`

セッション終了時に `/handover` コマンドで自動生成される引き継ぎノートのファイル群。
日時タイムスタンプ付きファイル。
`.gitignore` 対象（git 管理外）。

### `plan-*.md`・`todo.md`

Plan Mode で承認された計画（`.claude/plan-<slug>.md`）と、仕様議論を伴わない実装タスクの管理リスト（`.claude/todo.md`）。ともに `.claude/` 直下に置かれている。

| ファイル | 役割 |
|---|---|
| `plan-<slug>.md` | Plan Mode で承認された計画の保存先。`/plan` コマンドがシステムの都合で `~/.claude/plans/<slug>.md`（ホーム配下・グローバル）に自動生成するため、`ExitPlanMode` 承認後に同じ `<slug>` を使って `mv` で `.claude/plan-<slug>.md` へ移動する（セッションごとに一意な `<slug>` のため複数セッション同時実行でも衝突しない）。作業完了時は `git rm` で削除しコミット、中断・持ち越し時は残す（CLAUDE.md「計画ファイル・handover の扱い」参照） |
| `todo.md` | 仕様議論を伴わない実装タスクの管理リスト（CLAUDE.md「ToDo リスト運用ルール」参照） |

いずれも git 管理対象。

---

## CLAUDE.md の位置

このプロジェクトの CLAUDE.md はリポジトリルート（[CLAUDE.md](../CLAUDE.md)）に配置されています。
`.claude/` 直下には置いていません。
