# Statusline に時間枠・週間枠のリソース使用率を追加

## Context

現在 statusline はモデル名とコンテキスト使用率のみを1行表示している。
ユーザーは、これと同じ行に **時間枠（5時間ローリング）** と **週間枠（7日）** の
リソース使用率も並べて常時確認したい。

公式ドキュメント（https://code.claude.com/docs/en/statusline）で確認した事実:
- statusline に渡される JSON には `rate_limits.five_hour.used_percentage`（時間枠）と
  `rate_limits.seven_day.used_percentage`（週間枠）が含まれる（0〜100）
- statusline はアシスタントの新メッセージごとに再実行され、そのつど直近 API 応答の値で
  **毎メッセージ更新される**（最初の1回だけの固定値ではない）
- `rate_limits` は Claude.ai サブスク（Pro/Max）で、かつセッション最初の API 応答後に出現。
  それ以前・対象外では各枠が独立して欠落しうる

## 決定事項（ユーザー確認済み）

- 表示は現状どおり **1行**
- 欠落時の表示は **`—`（ダッシュ）**（`0%` ではなく未取得が分かるようにする）
- ラベルは **英語**: `Context` / `5h`（時間枠）/ `7d`（週間枠）

## 変更内容

対象ファイル: `~/.claude/settings.json` の `statusLine.command` の1箇所のみ。

変更前:
```
jq -r '"[\(.model.display_name)] コンテキスト \(.context_window.used_percentage // 0 | floor)%"'
```

変更後（jq 式。settings.json 内では二重引用符・バックスラッシュを JSON エスケープして格納）:
```
jq -r '"[\(.model.display_name)] Context \(.context_window.used_percentage // 0 | floor)% | 5h \(.rate_limits.five_hour.used_percentage | if . == null then "—" else (.|floor|tostring) + "%" end) | 7d \(.rate_limits.seven_day.used_percentage | if . == null then "—" else (.|floor|tostring) + "%" end)"'
```

ポイント:
- 各枠は `if . == null then "—" else (.|floor|tostring) + "%" end` で独立フォールバック
- コンテキストは従来どおり `// 0 | floor`

## 検証（mock 入力で確認済み）

```bash
cmd='"[\(.model.display_name)] Context \(.context_window.used_percentage // 0 | floor)% | 5h \(.rate_limits.five_hour.used_percentage | if . == null then "—" else (.|floor|tostring) + "%" end) | 7d \(.rate_limits.seven_day.used_percentage | if . == null then "—" else (.|floor|tostring) + "%" end)"'
echo '{"model":{"display_name":"Opus"},"context_window":{"used_percentage":12.4},"rate_limits":{"five_hour":{"used_percentage":23.5},"seven_day":{"used_percentage":41.2}}}' | jq -r "$cmd"
# => [Opus] Context 12% | 5h 23% | 7d 41%
echo '{"model":{"display_name":"Opus"},"context_window":{"used_percentage":8}}' | jq -r "$cmd"
# => [Opus] Context 8% | 5h — | 7d —
```

3パターン（両方あり／両方欠落／片方欠落）で期待どおり動作することを確認済み。

実機での最終確認: 設定反映後、次のメッセージ送信で statusline に
`Context X% | 5h Y% | 7d Z%` が表示され、メッセージごとに数値が更新されることを目視確認する。
