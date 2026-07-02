#!/usr/bin/env bash
# spec-panel-gate.sh — PreToolUse hook (ExitPlanMode)
# CLAUDE.md「計画の自動レビュー」節: docs/ 配下ファイルの作成・更新を含む計画は
# ExitPlanMode 前に /spec-panel を実施し mgmt/spec-findings/ に指摘記録が
# 作成されている必要がある。未実施の兆候があれば ask で一度だけ確認を挟む
# （ブロックはしない。誤検知時のコストを低く保つため）。
set +e
if ! command -v jq >/dev/null 2>&1; then
  printf '{"hookSpecificOutput":{"hookEventName":"PreToolUse","permissionDecision":"ask","permissionDecisionReason":"[hook警告] jq が見つからないため spec-panel-gate.sh のゲート判定が機能していません。環境異常の可能性があります。/spec-panel 実施済みか確認してください。"}}'
  exit 0
fi
in=$(cat)
plan=$(printf '%s' "$in" | jq -r '.tool_input.plan // empty')
[ -z "$plan" ] && exit 0

# docs/ 配下ファイルへの言及がない計画は対象外
printf '%s' "$plan" | grep -qE '`?docs/' || exit 0

# mgmt/spec-findings/ に直近(2時間以内)の指摘記録があれば /spec-panel 実施済みとみなす
recent=$(find "$CLAUDE_PROJECT_DIR/mgmt/spec-findings" -name '*.md' -mmin -120 2>/dev/null | head -1)
[ -n "$recent" ] && exit 0

jq -n --arg r "計画が docs/ 配下ファイルの作成・更新に言及していますが、mgmt/spec-findings/ に直近の指摘記録が見つかりません。/spec-panel によるセルフレビューを実施済みか確認してください（CLAUDE.md「計画の自動レビュー」節）。" \
  '{hookSpecificOutput:{hookEventName:"PreToolUse",permissionDecision:"ask",permissionDecisionReason:$r}}'
