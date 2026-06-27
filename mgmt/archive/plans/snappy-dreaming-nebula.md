# 計画: インシデント対策フック追加

## Context

2026-06-27 に4件のインシデントが発生（`1153` / `1426` / `23f3fb37` / `ce9e311a`）。
共通の症状は **Write/Edit ツールが「成功」を返すがファイルが実際には変更されない**（ハルシネーション）。
現行の `injection-guard.sh` は SessionStart のみ稼働し、ツール実行後の検証は一切行っていない。

インジェクション（ユーザーチャネル経由・injection-guard パターン外）も発生しているが、
**最も頻発し実害リスクが高いのはツール結果の捏造**。

---

## 対策内容（全 Sonnet で実施可能）

### タスク1: Write 検証フック（最優先）【Sonnet】

- **`~/.claude/hooks/write-verify.sh`** を新設
- Writeツール実行後にファイルの実在・サイズを確認し、結果を `additionalContext` で Claude へ返す
- 存在しない場合は明示的な警告を出す

```bash
#!/usr/bin/env bash
set +e
f=$(jq -r '.tool_input.file_path // empty' 2>/dev/null)
[ -z "$f" ] && exit 0
if [ -f "$f" ]; then
  size=$(wc -c < "$f" 2>/dev/null || echo "?")
  msg="[Write検証] ✓ $f 実在確認 (${size}B)"
else
  msg="[Write検証] ✗ $f が存在しません。Write が失敗またはパス誤りの可能性があります。"
fi
jq -n --arg c "$msg" '{hookSpecificOutput:{hookEventName:"PostToolUse",additionalContext:$c}}'
```

- `settings.json` の `hooks.PostToolUse` に matcher `Write` で追加

---

### タスク2: Edit 検証フック【Sonnet】

- **`~/.claude/hooks/edit-verify.sh`** を新設
- Editツール実行後に `git status --short <path>` で変更を確認
- git 管理外のファイルは `ls -la` でフォールバック確認

```bash
#!/usr/bin/env bash
set +e
f=$(jq -r '.tool_input.file_path // empty' 2>/dev/null)
[ -z "$f" ] && exit 0
status=$(git -C /workspace status --short -- "$f" 2>/dev/null)
if [ -n "$status" ]; then
  msg="[Edit検証] ✓ $f 変更確認: $status"
else
  # git 管理外 or 変化なし
  if [ -f "$f" ]; then
    msg="[Edit検証] △ $f の git 差分なし（既 staged / git 管理外 / Edit 失敗の可能性）"
  else
    msg="[Edit検証] ✗ $f が存在しません。"
  fi
fi
jq -n --arg c "$msg" '{hookSpecificOutput:{hookEventName:"PostToolUse",additionalContext:$c}}'
```

- `settings.json` の `hooks.PostToolUse` に matcher `Edit` で追加
- **注**: 既存の `Write|Edit` CLAUDE.md レビューフックと共存。matcher を個別に追加する形にする

---

### タスク3: injection-guard.sh パターン拡張【Sonnet】

現行パターン（`ignore.*instructions` 等）は「I'm now the user」系の social engineering をカバーしない。
以下を追加:

```text
I'?m now the (user|operator|admin)|you are now|pretend you are|act as if|disregard.*previous|override.*system
```

対象スキャンファイルは現行のまま（CLAUDE.md + handover 最新1件）。
インシデントディレクトリはスキャン対象に**追加しない**（インシデント本文に injection 例文が含まれるため誤検知する）。

---

## 変更対象ファイル

| ファイル | 操作 |
|---|---|
| `~/.claude/hooks/write-verify.sh` | 新規作成 |
| `~/.claude/hooks/edit-verify.sh` | 新規作成 |
| `~/.claude/settings.json` | `hooks.PostToolUse` に 2 エントリ追加 |
| `~/.claude/hooks/injection-guard.sh` | パターン1行追加 |

---

## 検証手順

1. `cat ~/.claude/hooks/write-verify.sh` で内容確認、`chmod +x` 確認
2. Write ツールで試験ファイルを書き、`additionalContext` に `[Write検証] ✓` が出ることを確認
3. 存在しないパスを Write した場合（または捏造時）に `✗` が出ることを確認（手動確認困難なため省略可）
4. Edit ツールで既存ファイルを編集し、`[Edit検証] ✓` が出ることを確認
5. `bash ~/.claude/hooks/injection-guard.sh` を実行しエラーなし確認（dry run）

---

## 対象外（今回対応しない）

- **孤立サブエージェント出力の遅延配送**（1426 事象）: プラットフォーム側の問題。対策は「出自不明の出力は作業起点にしない」という運用ルールのみ。コードで防ぐ方法なし
- **ユーザーチャネル経由 injection の完全防止**（ce9e311a 事象）: ハーネスで防ぐ手段なし。injection-guard のパターン拡張（タスク3）が部分対策
