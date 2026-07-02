#!/usr/bin/env bash
# docs-date-check.sh — PostToolUse hook (Write|Edit)
# docs/ 配下（3階層まで）の *.md 編集時に「| 最終更新日 |」行が当日付か検証し、
# 古ければ Edit での更新を Claude へ指示する（hook 自身はファイルを書き換えない）。
set +e
if ! command -v jq >/dev/null 2>&1; then
  printf '{"hookSpecificOutput":{"hookEventName":"PostToolUse","additionalContext":"[hook警告] jq が見つからないため docs-date-check.sh の検証をスキップしました。環境異常の可能性があります。"}}'
  exit 0
fi
f=$(jq -r '.tool_input.file_path // empty')
case "$f" in
  "$CLAUDE_PROJECT_DIR"/docs/*.md|"$CLAUDE_PROJECT_DIR"/docs/*/*.md|"$CLAUDE_PROJECT_DIR"/docs/*/*/*.md)
    grep -q '| 最終更新日 |' "$f" 2>/dev/null || exit 0
    today=$(date '+%Y-%m-%d')
    grep -qF "| 最終更新日 | $today |" "$f" && exit 0
    jq -n --arg ctx "$f の「| 最終更新日 |」行が当日付ではありません。このターン内に Edit で | 最終更新日 | $today | へ更新してください（hook はファイルを直接書き換えません）。" \
      '{"hookSpecificOutput":{"hookEventName":"PostToolUse","additionalContext":$ctx}}'
    ;;
esac
