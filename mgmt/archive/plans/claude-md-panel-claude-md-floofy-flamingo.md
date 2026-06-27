# CLAUDE.md 変更時に claude-md-panel を自動起動する

## Context（なぜ）

ユーザーは「CLAUDE.md が変更されたら `claude-md-panel`（CLAUDE.md 多視点レビュー
スキル）が自動実行され、常に最適化される状態」を望んでいる。

現状:

- `claude-md-panel` はグローバルスキル（`~/.claude/commands/claude-md-panel.md`,
  opus）として存在するが **手動起動のみ**。
- 既存の PostToolUse hook（`/workspace/.claude/settings.local.json`）は `*.md` に
  pymarkdown lint を流すだけで、claude-md-panel とは無関係。

→ 「CLAUDE.md 編集 → 自動でレビュー起動」を hook で恒久化する。範囲はユーザー選択により
**全プロジェクト共通（グローバル設定）**。

## 仕組み上の前提（設計の土台）

- hook はシェルコマンドしか実行できず、スキル（Claude の判断を伴う処理）を直接起動
  できない。よって既存 lint hook と同じ方式で **`additionalContext` に「claude-md-panel
  を実行せよ」という指示を自動注入**し、Claude に次ターンで実行させる。
- 実現できるのは **「レビューの自動起動」**。スキル定義（「実ファイル編集はユーザー承認後」）
  および仕様優先原則に従い、**書き換えの自動適用はしない**（編集は毎回ユーザー承認を挟む）。
- 編集ループ防止のため **クールダウン（同一ファイル 600 秒に 1 回）** を入れる。
  claude-md-panel が承認後に CLAUDE.md を編集して hook が再発火しても、10 分以内は
  再提案を抑止する。

## 変更対象

`/home/node/.claude/settings.json`（グローバル設定）に `PostToolUse` hook を追加する。
現状このファイルに `hooks` キーは無いため新規追加。`permissions` 等の既存キーは保持。

追加する hook（`matcher: "Write|Edit"`）の command:

```bash
jq -r '.tool_input.file_path // empty' | {
  read -r f
  [ -z "$f" ] && exit 0
  [ "$(basename "$f")" != "CLAUDE.md" ] && exit 0
  key=$(printf '%s' "$f" | md5sum | cut -c1-16)
  marker="/tmp/claude-md-panel-review-${key}"
  if [ -f "$marker" ]; then
    age=$(( $(date +%s) - $(stat -c %Y "$marker") ))
    [ "$age" -lt 600 ] && exit 0
  fi
  touch "$marker"
  jq -n --arg f "$f" '{"hookSpecificOutput":{"hookEventName":"PostToolUse","additionalContext":("CLAUDE.md (" + $f + ") が変更されました。claude-md-panel スキルを引数 " + $f + " で実行し、最適化レビューを提示してください（実ファイル編集はユーザー承認後）。")}}'
} 2>/dev/null || true
```

設計メモ:

- 発火条件はファイル名が `CLAUDE.md`（任意パス）。グローバル `~/.claude/CLAUDE.md`、
  各プロジェクト直下、`docs/CLAUDE.md` 等いずれも対象になる。引数にフルパスを渡すため
  claude-md-panel はそのファイルを主対象としてレビューする。
- クールダウン marker は `/tmp`（セッション跨ぎで残るがエフェメラルで十分）。
- 既存の project 側 lint hook（`*.md`）も CLAUDE.md 編集時に併走するが、両者は独立
  （lint 違反提示 + パネル提案）で衝突しない。

## 実装手順

1. `/home/node/.claude/settings.json` を Read。
2. `update-config` スキル（settings.json の hook 設定の正規手段）または直接 Edit で、
   上記 `PostToolUse` ブロックを `hooks` キーとして追加。既存キーは温存。
3. JSON が妥当か `jq . /home/node/.claude/settings.json` で確認。

## 検証（end-to-end）

1. **発火確認**: hook command にダミー JSON を流し、指示文 JSON が出力されること。

   ```bash
   echo '{"tool_input":{"file_path":"/workspace/CLAUDE.md"}}' | <hook command>
   # → hookSpecificOutput.additionalContext を含む JSON が出る
   ```

2. **クールダウン確認**: 直後にもう一度同じコマンドを流す → 出力なし（600 秒抑止）。
3. **非対象確認**: `file_path` を `/workspace/README.md` にして流す → 出力なし。
4. **実地確認**: marker を消してから CLAUDE.md を実際に 1 行編集し、次ターンで
   claude-md-panel 実行を促す additionalContext が注入されることを確認。

## 留意点

- グローバル設定 `~/.claude/settings.json` はリポジトリ外（commit 対象外）。本変更は
  プロジェクトのコミットには含まれない。
- 「常に最適化」は **自動レビュー提示まで**。書き換え適用は従来どおりユーザー承認制。
