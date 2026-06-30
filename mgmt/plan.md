# `.claude/` カスタマイズ整理 README 作成計画

## Context

Claude Code を長期間カスタマイズして使ってきた結果、ある機能が「グローバル（全プロジェクト共通）」のものか「このプロジェクト固有」のものか把握しづらくなっている。グローバル `/home/node/.claude/` とプロジェクト `/workspace/.claude/` の両方に commands・hooks・settings・rules 等が分散しており、対応関係が一覧化されていない。

そこで、両ディレクトリ直下にそれぞれ `README.md` を新設し、カスタマイズ要素のインベントリと「グローバル ↔ プロジェクトの役割分担」を可視化する。新規ユーザーや将来の自分が構造を素早く追えるようにすることが目的。

## 方針（確定事項）

- 配置: 両方に1つずつ（グローバル `/home/node/.claude/README.md` ＋ プロジェクト `/workspace/.claude/README.md`）
- プロジェクト側 README.md は git コミット対象に含める（グローバル側はホーム配下のため git 対象外）
- `incidents/` の記述は `handovers/` と同様に「概要＋件数のみ」とし、個別ファイル名は列挙しない
- 実装モデル: **Sonnet**

## 調査で判明した現状インベントリ（README 記載の元データ）

### グローバル `/home/node/.claude/`

- `commands/`（3件）
  - `claude-md-panel.md` — CLAUDE.md を4視点でレビューし指摘を一覧化
  - `handover.md` — セッション引き継ぎノート生成
  - `log-incident.md` — 環境異常を `.claude/incidents/` へ即時記録
- `hooks/`（4件）
  - `injection-guard.sh` — SessionStart で commands 許可リスト確認・注入署名スキャン
  - `edit-pre.sh` — Edit 前処理（CLAUDE.md 編集時に panel pending マーカー設定）
  - `edit-verify.sh` — Edit 後にファイル実在・サイズ検証
  - `write-verify.sh` — Write 後にファイル実在・サイズ検証
- `settings.json` — permissions（defaultMode: default）＋ hooks 設定 ＋ statusLine ＋ language(日本語) ＋ effortLevel(high) ＋ theme(auto)
  - hooks: PreToolUse(Edit), PostToolUse(Write|Edit / Write / Edit), Stop(claude-md-panel 強制), SessionStart(injection-guard)
- `CLAUDE.md` — 全プロジェクト共通ガイドライン（計画優先・検証徹底・モデル使い分け・メモリ無効化 等）
- `plugins/` — インストール済みプラグインなし

### プロジェクト `/workspace/.claude/`

- `commands/`（1件）
  - `spec-panel.md` — 仕様文書を4視点でレビューし指摘を一覧化
- `rules/`（1件）
  - `architecture.md` — 本プロジェクト（標高解析 SOTA 申請支援ツール）のアーキテクチャ・モジュール設計
- `incidents/` — 環境異常記録（日時タイムスタンプのファイル群、`/log-incident` で自動生成）※ handovers と同様に概要＋件数のみ記述
- `handovers/` — セッション引き継ぎノート（日時タイムスタンプのファイル群、`/handover` で自動生成、git 管理外）
- `settings.json` — 空（`{}`、設定は settings.local.json へ委譲）
- `settings.local.json` — permissions.allow リスト ＋ hooks
  - hooks: PreToolUse(Write|Edit / 実装ファイルの Opus 編集ガード), PostToolUse(Write|Edit / Lint 実行), SessionStart(handover+lessons 注入)
- プロジェクト CLAUDE.md は `.claude/` 直下に無く、リポジトリルートの `/workspace/CLAUDE.md` に存在

## 作業タスク

すべて **Sonnet**。

### T1. グローバル README 作成 — `/home/node/.claude/README.md`

構成:

1. 冒頭1〜2行: このファイルが何か（グローバル `.claude/` カスタマイズの目録）
2. **役割分担マトリクス**（グローバル vs プロジェクトの対比表）: CLAUDE.md / settings / commands / rules / hooks / incidents / handovers の各行で「グローバルに何があるか」「プロジェクトに何があるか」「使い分けポリシー」を示す
3. グローバル要素インベントリ: `commands/`(3)・`hooks/`(4)・`settings.json`・`CLAUDE.md`・`plugins/` を上記調査データのとおり記述
   - `commands/` の説明に「`.md` はスラッシュコマンド（`/<name>` で起動する skill）として現れる」旨を1行添え、commands と skills が別物と誤読されるのを防ぐ
4. 冒頭に「この環境のスナップショット（更新日: 作成時に `date` で取得）」を1行記し、global README が git 管理外で追従が必要な点を明示
5. プロジェクト側 README (`/workspace/.claude/README.md`) への参照リンク

### T2. プロジェクト README 作成 — `/workspace/.claude/README.md`

構成:

1. 冒頭1〜2行: このファイルが何か（このプロジェクトの `.claude/` カスタマイズの目録）
2. **役割分担マトリクス**: T1 と同じ対比表を共有（プロジェクト視点の導線として冒頭に再掲）
3. プロジェクト要素インベントリ: `commands/`(1=spec-panel)・`rules/`(1=architecture)・`settings.local.json`(permissions/hooks)・`settings.json`(空) を記述
   - `commands/` の説明に「`.md` はスラッシュコマンド（`/<name>` で起動する skill）として現れる」旨を1行添える（T1 と同趣旨）
4. `incidents/` と `handovers/` は **同じ書式**で「概要＋件数＋生成元コマンド＋git 管理状況」のみ記述（個別ファイル名は列挙しない）
5. プロジェクト CLAUDE.md は `/workspace/CLAUDE.md`（ルート）にある旨を明記
6. グローバル側 README への参照

### T3. lint・コミット

- `make lint`（特に `lint-md`）でプロジェクト README の警告ゼロを確認
- グローバル側 README はプロジェクト Makefile の lint 対象外（手動で Markdown 体裁を整える）
- プロジェクト README を `git add` → Conventional Commits でコミット（例: `docs: .claude/ カスタマイズ整理の README を追加`）
- グローバル側 README はホーム配下のためコミット不要

## 検証

- `make lint` が警告ゼロで通る（`lint-md` がプロジェクト README を検査）
- 両 README の役割分担マトリクスが調査インベントリと一致しているか目視確認（commands 件数・hooks 種別・settings 構成）
- `incidents/` と `handovers/` が同一書式・個別ファイル非列挙になっているか確認
- 相互参照リンク（グローバル ⇄ プロジェクト）が双方向に張られているか確認
- `git status` で意図したファイルのみがステージされているか確認

## 備考

- マトリクスは「コードや git で追える事実の複製」ではなく「分散した構造の地図」が目的。各要素の中身を全文転記せず、役割1行＋件数に留める
- 役割分担マトリクスは両 README に掲載するが、グローバル側 README を正本とし、プロジェクト側はプロジェクト視点で必要な範囲に絞りつつ正本へ相互参照を張る（重複による不整合を抑える）
- グローバル `.claude/` は他プロジェクトにも影響するため、README はこの環境のスナップショットである旨を1行添える（将来 commands を増減したら追従が必要）
