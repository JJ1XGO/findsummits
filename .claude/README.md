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
| [**settings.local.json**](#settingslocaljson) | 存在しない | 個人環境の permissions allow リスト・`skipDangerousModePermissionPrompt`（git 管理外） |
| [**commands/**](#commands) | 汎用 skill（handover / log-incident / claude-md-panel / update-best-practices） | ドメイン固有 skill（spec-panel / claude-container-issue） |
| [**rules/**](#rules) | 存在しない | アーキテクチャ定義（`architecture.md`） |
| [**hooks/**](#hooks) | 汎用保護（Write/Edit 検証・注入防止） | `hooks/session-start.sh`・`hooks/model-guard.sh`・`hooks/lint-posttool.sh`・`hooks/docs-date-check.sh`・`hooks/spec-panel-gate.sh`（いずれも `settings.json` から呼び出し） |
| [**incidents/**](#incidents) | 存在しない | 環境異常記録（このプロジェクト配下・git 管理外） |
| [**handovers/**](#handovers) | 存在しない | セッション引き継ぎノート（このプロジェクト配下・git 管理外） |
| [**`plans/`・`todo.md`**](#planstodomd) | `~/.claude/plans/`（本プロジェクトでは不使用） | devel 運用ファイル（git 管理対象。plans/ は `plansDirectory` 設定による生成先） |

---

## プロジェクト要素インベントリ

### `commands/`

`commands/*.md` はスラッシュコマンド（`/<name>` で起動する skill）として Claude Code に現れます。

| No. | ファイル | スラッシュコマンド | 役割 |
|---|---|---|---|
| 1 | `spec-panel.md` | `/spec-panel` | 仕様文書（URD/SRS/HLD/LLD/ADR/テスト仕様）をアーキテクト・仕様レビュアー・データ/アルゴリズム・申請者の4視点でレビューし指摘を一覧化 |
| 2 | `claude-container-issue.md` | `/claude-container-issue` | claude-container への GitHub issue 起票・対応完了確認・クローズの詳細フロー（起票先の判定基準は `CLAUDE.md`「環境課題の連携」節） |

### `rules/`

Claude Code が自動的に読み込むプロジェクトルール定義です。

| No. | ファイル | 役割 |
|---|---|---|
| 1 | `architecture.md` | 本プロジェクトのアーキテクチャ・モジュール設計・ディレクトリ構成・`$DATA_DIR` 配置ルールを定義 |

### `hooks/`

`settings.json` の各フックから呼び出されるスクリプト群です（1フック1ファイル。
巨大なワンライナーを settings.json に直書きせず、外出しして可読性・diff の追いやすさを確保する）。

| No. | ファイル | 呼び出し元 | 役割 |
|---|---|---|---|
| 1 | `session-start.sh` | SessionStart hook | handover を注入（best_practices.md は CLAUDE.md の `@import` で常時注入、lessons.md は都度 Read）。`.claude/incidents/[0-9]*.md`（`known-patterns.md` 等の非インシデントファイルを除外するglob）を全件走査し、各ファイルの最後にマッチした状態行が「解決済」と明示検出できないものを未解決として件数・ファイル名一覧を常時表示（fail-closed。未知フォーマット・状態行欠落も警告側に倒す）。未解決が0件のときのみ、最新handoverの「環境異常・インシデント」セクションに「なし」以外の記録があれば追加で環境確認チェックリスト実行を指示（解決済みインシデントも直後の1セッションで要確認）。**handoverチェーン検証**: 最新handoverの本文に直前handoverのファイル名が含まれるかを確認し、含まれなければ「未消化」とみなして直前分（最大3世代）の「次にやること」を追加注入する（`/handover` 側で「引き継ぎ元handover」フィールドへの記載と前回分の消化を必須化しており、正しく運用されていればチェーンは途切れない）。プロジェクト外層（ホスト層/Anthropic層）の既知パターン台帳（`~/.claude/global-incidents/known-patterns.md`）が存在すればパターン見出し＋再発ログ件数＋最終再発日のダイジェストを注入（全文は注入しない。フェイルソフト。件数のみだと偏りで活性度が読めないため最終再発日を添える）。また handover の「## 学び」セクション項目を lessons.md と突き合わせ、未転記のものを全件初回返答時に追記するよう Claude へ指示。さらに lessons.md の件数増加をウォーターマーク（`best_practices_watermark`）と比較し、+10件以上で初回返答時に AskUserQuestion による `/update-best-practices` 実行可否確認を Claude へ指示。加えて `gh` があるコンテナ内セッションでは `jj1xgo/claude-container`・`jj1xgo/findsummits` それぞれの open issue 一覧を自動確認・注入（フェイルソフト。`gh` 不在・API 失敗時は一行メッセージのみでスキップ）。`jq` が使えれば各 issue に最終コメントの最終非空行（署名行想定）も添え、コンテナ内 gh が `comments` フィールド未対応の場合は comments 無しの従来クエリへ自動フォールバックする |
| 2 | `model-guard.sh` | PreToolUse(Write\|Edit) hook | 実装ファイル（`.c/.h/.py/.html`）を Opus/Fable で編集しようとすると警告・permission ask |
| 3 | `lint-posttool.sh` | PostToolUse(Write\|Edit) hook | `$CLAUDE_PROJECT_DIR` 配下のファイルのみ対象。ファイル種別に応じて Lint 実行（Markdown/Python/GeoJSON/HTML）し結果を返送 |
| 4 | `docs-date-check.sh` | PostToolUse(Write\|Edit) hook | `docs/` 配下（3階層まで）の `*.md` 編集時に `\| 最終更新日 \|` 行が当日付でなければ更新指示を additionalContext で返送（ファイルは直接書き換えない）。実ヘッダーを持たずテンプレート例示に誤検知する `docs/CLAUDE.md` は除外 |
| 5 | `spec-panel-gate.sh` | PreToolUse(ExitPlanMode) hook | 計画本文が `docs/` に言及し、かつ `mgmt/spec-findings/` に直近（2時間以内）の指摘記録がなければ `/spec-panel` 未実施の可能性ありとして permission ask（CLAUDE.md「計画の自動レビュー」節。ブロックはせず確認のみ） |

### `settings.json`

プロジェクト共通の hooks（5件）を定義します（git 管理対象。クローン先でも同じ自動化が効く）。
各 hook は対応する `hooks/*.sh` を呼び出すのみで、ロジック本体はスクリプト側に置く。
hook 内のパスは実行時に Claude Code が設定する `$CLAUDE_PROJECT_DIR` で解決し、
実行環境（ローカル / コンテナ）に依存しない。

| トリガー | Matcher | 呼び出すスクリプト |
|---|---|---|
| PreToolUse | Write\|Edit | `hooks/model-guard.sh` |
| PreToolUse | ExitPlanMode | `hooks/spec-panel-gate.sh` |
| PostToolUse | Write\|Edit | `hooks/lint-posttool.sh` |
| PostToolUse | Write\|Edit | `hooks/docs-date-check.sh` |
| SessionStart | startup/resume/clear/compact | `hooks/session-start.sh` |

`.claude/` 構成ファイル変更時の README.md 更新リマインドはグローバル hook（`~/.claude/settings.json`）で対応。

`"enabledMcpjsonServers": ["github"]` は `.mcp.json` の `github` サーバー（クロスリポジトリ issue
操作用 GitHub 公式 MCP、詳細は `CLAUDE.md`「環境課題の連携」節）を事前承認し、接続時の確認プロンプトを
機構的に省略する設定。

`permissions.deny` に `Bash(git push --force:*)` / `Bash(git push -f:*)` を設定し、force push を
実行前ブロックする（`GIT_PUSH_TOKEN` によりコンテナ内 push が可能になったことを受けた導入）。
プレフィックス一致のため `git push origin main --force` のような後置形や `git -C <path> push --force`、
`+refspec` 形式は素通りする既知の制約があり、これを補完する「force push 前は必ずユーザー確認する」
という文書ルールを `CLAUDE.md`「ルールと制約」節に併記している（二重化）。

### `settings.local.json`

個人環境の permissions allow リスト（実行許可ホワイトリスト）と `skipDangerousModePermissionPrompt` を定義します（git 管理外）。

`skipDangerousModePermissionPrompt` は claude-container 経由のコンテナ内実行（`--dangerously-skip-permissions` 前提）と整合させるための設定。Claude Code はこのキーを `userSettings`・`localSettings`・`flagSettings`・`policySettings` からのみ読み取り、`projectSettings`（`.claude/settings.json`、共有・git 管理下）は判定対象に含まれない（バイナリ実装で確認済み、2026-07-07）。git 管理下の設定だけで危険モードのダイアログを無効化できると悪用リスクがあるための意図的な仕様と考えられる。そのため `settings.local.json` に置く。

| 許可対象 | 目的 |
|---|---|
| `WebFetch(domain:cyberjapandata.gsi.go.jp)` | 国土地理院の標高タイル取得のみ許可 |
| `Bash(make *)` | Makefile 実行 |
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

### `plans/`・`todo.md`

Plan Mode の plan ファイル（`.claude/plans/<slug>.md`）と、仕様議論を伴わない実装タスクの管理リスト（`.claude/todo.md`）。

| ファイル | 役割 |
|---|---|
| `plans/<slug>.md` | Plan Mode の plan ファイルの生成先。`settings.json` の `plansDirectory: ".claude/plans"` により最初からリポジトリ内に生成される（以前はデフォルトの `~/.claude/plans/`（ホーム配下・グローバル）に生成され、承認後に `.claude/plan-<slug>.md` へ `mv` する運用だったが、mv 忘れが claude-container で実際に発生したため設定で根治した、2026-07-04）。セッションごとに一意な `<slug>` のため複数セッション同時実行でも衝突しない。作業完了時は `git rm` で削除しコミット、中断・持ち越し時は残す（CLAUDE.md「計画ファイル・handover の扱い」参照） |
| `todo.md` | 仕様議論を伴わない実装タスクの管理リスト（CLAUDE.md「ToDo リスト運用ルール」参照） |

いずれも git 管理対象。

---

## CLAUDE.md の位置

このプロジェクトの CLAUDE.md はリポジトリルート（[CLAUDE.md](../CLAUDE.md)）に配置されています。
`.claude/` 直下には置いていません。
