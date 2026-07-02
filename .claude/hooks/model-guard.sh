#!/usr/bin/env bash
# model-guard.sh — PreToolUse hook (Write|Edit)
# 実装ファイル（.c/.h/.py/.html）を Opus/Fable で編集しようとした場合に警告し承認を求める
# （CLAUDE.md 原則#4: 実装は Sonnet 推奨）。
set +e
if ! command -v jq >/dev/null 2>&1; then
  printf '{"hookSpecificOutput":{"hookEventName":"PreToolUse","permissionDecision":"ask","permissionDecisionReason":"[hook警告] jq が見つからないため model-guard.sh のモデルガードが機能していません。環境異常の可能性があります。続行してよいか確認してください。"}}'
  exit 0
fi
in=$(cat)
f=$(printf '%s' "$in" | jq -r '.tool_input.file_path // empty')
case "$f" in
  *.c|*.h|*.py|*.html) ;;
  *) exit 0 ;;
esac
tp=$(printf '%s' "$in" | jq -r '.transcript_path // empty')
[ -z "$tp" ] && exit 0
[ -f "$tp" ] || exit 0
m=$(tac "$tp" | jq -rc 'select(.type=="assistant" and .message.model)|.message.model' 2>/dev/null | head -1)
case "$m" in
  *opus*|*fable*)
    jq -n --arg r "Opus/Fable で実装ファイル ($f) を編集しようとしています（CLAUDE.md 原則#4: 実装は Sonnet 推奨）。計画セッションと実装セッションを分けている場合は計画のモデル指定を確認し、Sonnet 指定なら /model 切替をユーザーに促してください。Opus 継続が正当なら理由を述べた上で承認してください。" \
      '{hookSpecificOutput:{hookEventName:"PreToolUse",permissionDecision:"ask",permissionDecisionReason:$r}}'
    ;;
  *) exit 0 ;;
esac
