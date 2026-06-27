# 計画: claude-md-panel をターン終了時に確実に実行させる（Stop hook）

## Context（なぜやるか）

- 現状: CLAUDE.md 編集時に PostToolUse hook が claude-md-panel 実行を **促すだけ**。
  Claude は実装優先でスキップでき、そのまま忘れられる
  （本セッションで実際に発生。ユーザーが「通った?」と確認して気付いた）。
- 要望: 実装中のスキップは許容。ただし **CLAUDE.md を編集したターンの最後** に
  claude-md-panel が確実に走るようにする（ユーザー確認済み）。
- 機構: Claude Code の **Stop hook**（`decision:block`）でターン終了をブロックし、
  パネル未実施なら強制する。`stop_hook_active` で無限ループを防ぐ
  （公式 docs で確認済み: `{"hookSpecificOutput":{"hookEventName":"Stop","decision":"block","reason":...}}`）。

## 設計（マーカーファイル方式・3点連携）

Skill 実行を hook で検知できるかは公式 docs 上не明確なため、検知に依存しない
ファイルマーカー方式を採る。

1. **PostToolUse**（CLAUDE.md 編集時）: 保留マーカー `/tmp/claude-md-panel-pending-<key>` をセット
2. **Stop hook**: 保留マーカーがあればターン終了を `decision:block` し、パネル実行を強制
3. **claude-md-panel skill**: レビュー提示後に自分の保留マーカーを解除

`<key>` は既存 hook と同じ `md5sum | cut -c1-16`（ファイルパスから算出）。

## 対象ファイル（すべてグローバル層・git 管理外＝コミット不要）

### 1. `~/.claude/settings.json`

**(a) 既存 PostToolUse（claude-md-panel リマインダー）に保留マーカー設定を1行追加**

`key=...` 算出の直後（既存のデバウンス判定より前）に常時マーカーを書く:

```sh
printf '%s\n' "$f" > "/tmp/claude-md-panel-pending-${key}";
```

既存のデバウンス付きリマインダー（additionalContext）はそのまま残す
（文言に「ターン終了時に Stop hook が強制する」旨を一言追記）。

**(b) Stop hook を新設**（POSIX sh 安全・bash 配列不使用）

```sh
in=$(cat); active=$(printf '%s' "$in" | jq -r '.stop_hook_active // false');
set -- /tmp/claude-md-panel-pending-*; [ -e "$1" ] || exit 0;
if [ "$active" = "true" ]; then rm -f /tmp/claude-md-panel-pending-*; exit 0; fi;
files=$(cat /tmp/claude-md-panel-pending-* 2>/dev/null | sort -u | tr '\n' ' ');
jq -n --arg r "CLAUDE.md ($files) を編集しましたが claude-md-panel レビューが未実施です。ターンを終える前に対象ファイルごとに claude-md-panel スキルを実行してください。レビュー提示でマーカーは自動解除されます。" '{"hookSpecificOutput":{"hookEventName":"Stop","decision":"block","reason":$r}}'
```

- `set -- glob; [ -e "$1" ]`: マーカー無し（glob 不一致）なら即 `exit 0`（許可）
- `active=true`（＝直前のブロックでパネルを実行済み）なら全マーカー解除して許可（ループ防止）

### 2. `~/.claude/commands/claude-md-panel.md`

「手順」末尾に最終ステップを追加する。内容:
「レビュー提示後、対象ファイルの保留マーカーを解除する（Stop hook の自動強制を解く）。
対象ファイルの絶対パス ABS について
`rm -f /tmp/claude-md-panel-pending-$(printf '%s' "ABS" | md5sum | cut -c1-16)` を実行。」

（マーカー解除はレビュー提示時点で行う。ユーザーが指摘を適用するか否かに関わらず「レビューは実施済み」のため。）

## 動作フロー（例: CLAUDE.md を1行編集して応答を終える）

1. 編集 → PostToolUse が保留マーカーをセット
2. Claude が実装を続け、ターンを終えようとする → Stop hook 発火（active=false）→ **block**
3. Claude が claude-md-panel を実行 → レビュー提示 → skill がマーカー解除
4. 継続応答が終了 → Stop hook 再発火（active=true）→ マーカー無し → 許可

## 既知の挙動・トレードオフ

- パネルの指摘を「適用」して CLAUDE.md を再編集すると、そのターン末で **再度1回**
  レビューが走る（適用結果の確認になる。指摘が無くなれば収束する）。
- 無限ループは `stop_hook_active` により「1ターン最大1ブロック」に制限。
- Stop hook は全ターン末に発火するが、マーカー無し時は jq 1回＋glob 判定で即 exit。負荷は無視できる。
- `/tmp` 上のマーカーは再起動で消える。孤児マーカーが残っても active=true 経路で1回で解消。

## 検証

1. **hook 単体**: 偽の Stop 入力でブロック/許可を確認
   - マーカー作成 → `echo '{"stop_hook_active":false}' | <stop-hook-cmd>` で `decision:block` JSON が出ることを確認
   - 同入力で `"stop_hook_active":true` → 出力なし（exit 0）＆マーカー削除を確認
   - マーカー無し → 出力なし（exit 0）を確認
2. **PostToolUse**: CLAUDE.md を1行 Edit → `/tmp/claude-md-panel-pending-*` が生成されることを確認
3. **実地**: CLAUDE.md を編集してターンを終える → Stop hook が claude-md-panel を強制 →
   パネル実行でマーカー解除 → 通常終了、を1サイクル確認
4. JSON 構文: 編集後 `jq . ~/.claude/settings.json` で settings.json が妥当な JSON であることを確認

## モデル

- 全タスク: **Sonnet**（shell-in-JSON の編集中心。設計は本計画で完了済み）。
  引用符・jq が壊れやすいので、編集後に上記「検証1」の hook 単体テストを必ず実施する。
