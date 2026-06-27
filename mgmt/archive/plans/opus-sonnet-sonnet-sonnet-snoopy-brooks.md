# 対策: Opus が「Sonnet 推奨」しながら自分で実装を始める問題

## Context（なぜこの変更が必要か）

- **事象**: 前回・前々回セッションで、Opus が CLAUDE.md 原則 #5 に従い「実装は Sonnet 推奨」と述べた直後に、自分（Opus）で実装を開始した。
- **問題**: Opus は高コスト。推奨を出した本人が無視して実装するのは (a) リソースの無駄、(b) 推奨の自己矛盾。
- **ユーザーの論点**: 「Sonnet に切り替えるコストと変わらないなら、Sonnet を推奨すること自体がおかしい」。
  - **結論（この対策の前提）**: `/model` 切替自体はほぼ無コスト（コマンド1つ）で、Sonnet 実装は Opus より大幅に安価。よって**推奨自体は経済的に正しい**。欠陥は「推奨の不遵守」であって推奨の存在ではない。
  - したがって対策は「推奨を任意 → 強制」に変える。口頭指示（CLAUDE.md）は2回破られた実績があるため、**機械的ゲート**で担保する。

## 技術的前提（検証済み・根拠主義）

- PreToolUse hook は `model` を直接受け取れない（公式 doc: 「model は SessionStart のみ・保証なし、`$CLAUDE_MODEL` も無い」）。
- ただし hook は `transcript_path` を受け取る。トランスクリプト JSONL はアシスタント発話ごとに `message.model` を記録（実データで `claude-opus-4-8` を確認）。
- PreToolUse 発火時点で現ターンの発話は既に永続化済み → ライブ session 上で
  `tac "$transcript" | jq -rc 'select(.type=="assistant" and .message.model)|.message.model' | head -1`
  が正しく現モデルを返すことを本 session で実証。
- ツール呼び出し単位でモデルが記録されるため、編集を行う主体を per-call で判定できる（前々回は Opus と Haiku が混在していた実データで確認）。

## 対策（2層: 機械的ゲート + 指示の明確化）

### 確定事項（ユーザー選択）

- **ゲート強度**: `permissionDecision: "ask"`（確認プロンプト）。
- **適用範囲**: このプロジェクトのみ → `/workspace/.claude/settings.local.json`。

### 層1: 機械的ゲート（PreToolUse hook）— 主対策

- **設置先**: `/workspace/.claude/settings.local.json` の `hooks.PreToolUse`（既存 PostToolUse lint hook と同じファイル。additive で既存を壊さない）。
- **matcher**: `Write|Edit`
- **ロジック（bash + jq, stdin から hook 入力 JSON を受ける）**:
  1. `tool_input.file_path` と `transcript_path` を取得。
  2. file_path が実装ファイル（`*.c` / `*.h` / `*.py`）でなければ即 allow（docs/spec/ADR/tracker/plan/設定の Opus 作業は素通り）。
  3. 実装ファイルなら、上記 extractor で現モデルを取得。
  4. モデルに `opus` または `fable` を含む → `permissionDecision: "ask"`（または `deny`）を返し、理由メッセージを付与。
  5. それ以外（sonnet/haiku/不明/抽出失敗）→ allow。**fail-open**（誤ブロックより保護喪失を選ぶ安全側）。
- **理由メッセージ案**:
  「Opus/Fable で実装ファイル ($file) を編集しようとしています（CLAUDE.md 原則#5: 実装は Sonnet 推奨）。Sonnet を推奨したなら手を止め `/model` 切替を促してください。Opus 継続が正当なら理由を述べた上で承認してください。」
- **scope の根拠**: 原則 #5 は「Opus=調査・設計・判断、Sonnet=ファイル編集中心の実装」。docs/`.md`/mgmt/設定は Opus 適正作業なので非ゲート。`.c/.h/.py`（コード）のみゲート。

#### hook コマンド草案（実装時に settings.local.json へ追加）

```jsonc
// hooks.PreToolUse に追加するエントリ
{
  "matcher": "Write|Edit",
  "hooks": [{
    "type": "command",
    "command": "in=$(cat); f=$(printf '%s' \"$in\" | jq -r '.tool_input.file_path // empty'); case \"$f\" in *.c|*.h|*.py) ;; *) exit 0;; esac; tp=$(printf '%s' \"$in\" | jq -r '.transcript_path // empty'); [ -z \"$tp\" ] && exit 0; [ -f \"$tp\" ] || exit 0; m=$(tac \"$tp\" | jq -rc 'select(.type==\"assistant\" and .message.model)|.message.model' 2>/dev/null | head -1); case \"$m\" in *opus*|*fable*) jq -n --arg r \"Opus/Fable で実装ファイル ($f) を編集しようとしています（CLAUDE.md 原則#5: 実装は Sonnet 推奨）。Sonnet を推奨したなら手を止め /model 切替を促してください。Opus 継続が正当なら理由を述べた上で承認してください。\" '{hookSpecificOutput:{hookEventName:\"PreToolUse\",permissionDecision:\"ask\",permissionDecisionReason:$r}}';; *) exit 0;; esac"
  }]
}
```

- `cat` で stdin を一度受けて `$in` に保持（jq を2回呼ぶため）。
- 実装ファイル以外・transcript 不在・モデル抽出失敗はすべて `exit 0`（fail-open）。
- 実装時に `jq . /workspace/.claude/settings.local.json` で JSON 構文を検証する。

### 層2: CLAUDE.md 原則 #5 の文言強化 — 補助（defense in depth）

- 対象: `/home/node/.claude/CLAUDE.md` の「### 5. モデルを使い分ける」。
- 追記する拘束:
  - 「Sonnet を推奨したら、その推奨は**そのターンの終端**である。ユーザーがモデルを確認・切替するまで実装ファイル（コード）の編集を一切行わない。」
  - 「Opus のまま実装を始めるのは推奨の自己矛盾であり禁止。」
  - 「Opus 継続が正当と判断する場合は、Sonnet を推奨**せず**最初から『Opus 継続』を理由付きで申し出る（二者択一を曖昧にしない）。」

## 変更ファイル

- `/workspace/.claude/settings.local.json` — `hooks.PreToolUse` ブロックを追加（層1）。
- `/home/node/.claude/CLAUDE.md` — 原則 #5 に拘束文言を追記（層2）。
  - 注: グローバル CLAUDE.md 編集は PostToolUse hook が claude-md-panel レビューを促す。レビュー提示 → ユーザー承認後に確定。

## 検証

1. **正常系（Opus×docs）**: Opus のまま `.md` を Edit → ゲート素通り（プロンプト無し）を確認。
2. **発火系（Opus×コード）**: Opus のまま `.py`/`.c` を Edit → 確認プロンプト（または deny）が出ることを確認。
3. **解除系（Sonnet×コード）**: `/model` で Sonnet に切替後に同じコード編集 → 素通りを確認。
4. **fail-open**: 不正な transcript_path 等でも allow になり既存作業を妨げないことを確認。
5. 既存の PostToolUse lint hook が引き続き動作することを確認（JSON 構文を `jq . settings.local.json` で検証）。

## モデル運用メモ

- 本対策は hook 設定（JSON）と CLAUDE.md 編集が中心の単純作業。設計判断は本計画で完了済み。
- 実装着手時は **Sonnet 推奨**（まさに本件が示す通り）。…ただし本タスク自体が「Opus が実装すべきでない例」なので、承認後は `/model` で Sonnet 切替を促す。
