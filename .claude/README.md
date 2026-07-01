# .claude/ カスタマイズ目録（このプロジェクト固有）

このプロジェクト（標高解析 SOTA 申請支援ツール）の `.claude/` に配置した
Claude Code カスタマイズの一覧です。

グローバル共通設定との役割分担は `~/.claude/README.md` を参照してください。

---

## 役割分担サマリー（プロジェクト視点）

| 要素 | グローバル `~/.claude/` | このプロジェクト `.claude/` |
|---|---|---|
| [**CLAUDE.md**](#claudemd-の位置) | 全プロジェクト共通ガイドライン | リポジトリルートに配置 |
| [**settings.json**](#settingsjson) | 基盤設定一式 | 空 `{}`（`settings.local.json` へ委譲） |
| [**settings.local.json**](#settingslocaljson) | 存在しない | プロジェクト固有の permissions + hooks（git 管理外） |
| [**commands/**](#commands) | 汎用 skill（handover / log-incident / claude-md-panel） | ドメイン固有 skill（spec-panel） |
| [**rules/**](#rules) | 存在しない | アーキテクチャ定義（`architecture.md`） |
| [**hooks/**](#hooks) | 汎用保護（Write/Edit 検証・注入防止） | `hooks/session-start.sh`（SessionStart）+ `settings.local.json`（Lint・model ガード） |
| [**incidents/**](#incidents) | 存在しない | 環境異常記録（このプロジェクト配下・git 管理外） |
| [**handovers/**](#handovers) | 存在しない | セッション引き継ぎノート（このプロジェクト配下・git 管理外） |

---

## プロジェクト要素インベントリ

### `commands/`

`commands/*.md` はスラッシュコマンド（`/<name>` で起動する skill）として Claude Code に現れます。

| No. | ファイル | スラッシュコマンド | 役割 |
|---|---|---|---|
| 1 | `spec-panel.md` | `/spec-panel` | 仕様文書（URD/SRS/HLD/LLD/ADR/テスト仕様）をアーキテクト・仕様レビュアー・データ/アルゴリズム・申請者の4視点でレビューし指摘を一覧化 |
| 2 | `update-best-practices.md` | `/update-best-practices` | `lessons.md` の蓄積内容を Opus で再分析し `best_practices.md` を再合成する。ウォーターマーク（`best_practices_watermark`）を更新し lint・コミットまで実行 |

### `rules/`

Claude Code が自動的に読み込むプロジェクトルール定義です。

| No. | ファイル | 役割 |
|---|---|---|
| 1 | `architecture.md` | 本プロジェクトのアーキテクチャ・モジュール設計・ディレクトリ構成・`$DATA_DIR` 配置ルールを定義 |

### `hooks/`

`settings.local.json` の SessionStart フックから呼び出されるスクリプト群です。

| No. | ファイル | 呼び出し元 | 役割 |
|---|---|---|---|
| 1 | `session-start.sh` | SessionStart hook | handover + lessons を注入。以下2条件のいずれかで `/log-incident` の環境確認チェックリスト実行を Claude へ指示: ①最新インシデントファイルが「未解決」状態、②最新handoverの「環境異常・インシデント」セクションに「なし」以外の記録がある（解決済みインシデントも直後の1セッションで要確認）。また handover の「## 学び」セクション項目を lessons.md と突き合わせ、未転記のものを全件初回返答時に追記するよう Claude へ指示。さらに lessons.md の件数増加をウォーターマーク（`best_practices_watermark`）と比較し、+10件以上で `/update-best-practices` 実行を推奨 |

### `settings.local.json`

プロジェクト固有の permissions と hooks を定義します（`settings.json` は空）。

**permissions.allow リスト（実行許可ホワイトリスト）:**

| 許可対象 | 目的 |
|---|---|
| `WebFetch(domain:cyberjapandata.gsi.go.jp)` | 国土地理院の標高タイル取得のみ許可 |
| `Bash(make *)` | Makefile 実行 |
| `Bash(venv/bin/python3 mgmt/tracker/track.py *)` | 課題管理スクリプト |
| `Bash(git status\|log\|diff\|show\|ls-files ...)` | Git 読み取り |
| `Bash(date\|ls\|grep\|find\|wc *)` | 標準ツール |

**hooks（4件）:**

| トリガー | Matcher | 役割 |
|---|---|---|
| PreToolUse | Write\|Edit | 実装ファイル（`.c/.h/.py`）を Opus で編集しようとすると警告・permission ask |
| PostToolUse | Write\|Edit | ファイル種別に応じて Lint 実行（Markdown/Python/GeoJSON/HTML）し結果を返送 |
| PostToolUse | Write\|Edit | `docs/*.md` 編集時に `\| 最終更新日 \|` 行を当日付に自動書き換え |
| SessionStart | startup/resume/clear | `hooks/session-start.sh` を呼び出す。handover + lessons を自動注入し、未解決インシデントまたは前handoverに環境異常記録があれば環境確認チェックリスト実行を指示 |

`.claude/` 構成ファイル変更時の README.md 更新リマインドはグローバル hook（`~/.claude/settings.json`）で対応。

### `settings.json`

空（`{}`）。全設定は `settings.local.json` に記述。

### `incidents/`

環境異常（Opus の捏造・hallucination・plan mode 動作不正など）を記録するファイル群。
`/log-incident` コマンドで自動生成される日時タイムスタンプ付きファイル（現在 21 件）。
git 管理対象（リポジトリに含める）。

### `handovers/`

セッション終了時に `/handover` コマンドで自動生成される引き継ぎノートのファイル群。
日時タイムスタンプ付きファイル（現在 280 件）。
`.gitignore` 対象（git 管理外）。

---

## CLAUDE.md の位置

このプロジェクトの CLAUDE.md はリポジトリルート（[CLAUDE.md](../CLAUDE.md)）に配置されています。
`.claude/` 直下には置いていません。
