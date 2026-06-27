# 環境不安定（挙動不安定・指示無視）の原因除去

## Context（なぜこの作業をするか）

ユーザーから「最近、環境がおかしい事がある」「挙動が不安定・指示を無視する」との報告。
調査の結果、`/home/node/.claude/commands/` に**本プロジェクトと無関係の外部
「エージェント・オーケストレーター」系コマンドが7個**混入していることが判明した。

これらはいずれも強い命令調のメタプロンプト（`SYSTEM:`/`<<SYSTEM>>`/`IMPERATIVE`/
`MANDATORY`/`zero-deviation`/`never halt`/`infinite-agentic-loop`/別エージェント spawn・
tmux send-keys 等）で、

- 短い名前（`/taskflow` `/adaptive` `/controlflow` 等）でうっかり呼ぶと発動し、
  プロジェクト CLAUDE.md のルール（計画優先・1問ずつ・指示遵守・Sonnet 切替等）を上書きする
- 呼ばなくても各コマンドの description（`<<SYSTEM>>` 等）が毎セッションのスキル/コマンド
  一覧に注入され、文脈を汚染・矛盾指示を持ち込む

これが「挙動不安定・指示無視」の直接原因。**狙う結果＝この7個を無効化し、挙動を安定させる。**

## 対象（退避する7ファイル）

`/home/node/.claude/commands/` 配下:

| ファイル | 中身（問題箇所） |
|---|---|
| `adaptive.md` | infinite-agentic-loop / endless loop |
| `agentflow.md` | `<System><Mandate>` tmux send-keys 強制 |
| `context_pipeline.md` | 全 system 命令末尾に "think" 強制注入 |
| `controlflow.md` | `SYSTEM: ... (IMPERATIVE VERSION)` MANDATORY |
| `taskengine.md` | `<<SYSTEM>>` zero-deviation / never halt |
| `taskflow.md` | `taskengine.md` と同一内容 |
| `devops.md` | zero tolerance / Loop until DEVIATIONS==Ø |

**残す（正規・触らない）**: `handover.md` `pr-draft.md` `pr-review.md` `refactor.md` `tdd.md`
（いずれも Claude Code の正規スキル一覧と一致）

## 方針

- **削除ではなく退避**（reversible）。ユーザーが作ったファイルであり、後で戻せる形にする。
- 退避先: `/home/node/.claude/commands_disabled/`（新規作成）へ7ファイルを `mv`。
  - commands/ 直下から外れれば、コマンド/スキル一覧に載らず発動もしなくなる。

## 手順

1. `mkdir -p /home/node/.claude/commands_disabled`
2. 上記7ファイルを `mv` で `commands_disabled/` へ移動（1件ずつ明示指定。ワイルドカード一括は避ける）
3. `ls /home/node/.claude/commands/` で残りが正規5ファイルのみになったことを確認

## 検証

- `ls /home/node/.claude/commands/` → `handover/pr-draft/pr-review/refactor/tdd` の5つだけ
- `ls /home/node/.claude/commands_disabled/` → 退避した7つが揃っている
- 新しいセッション（または `/status` 再表示）で、スキル/コマンド一覧から
  `adaptive/agentflow/context_pipeline/controlflow/taskengine/taskflow/devops` が消えていること
- 以後、挙動が安定し指示無視が再発しないかをユーザーが実利用で確認

## 補足（今回の対象外・別件メモ）

調査中に見つかった別の不整合（今回の症状とは別。希望があれば別途対応）:

1. **clangd-lsp プラグインのパス不整合**
   - `installed_plugins.json` / `known_marketplaces.json` が旧ホストユーザー `/home/tsu/...`
     を指したまま（現在の HOME は `/home/node`、`/home/tsu` は存在しない）
   - `clangd-lsp@claude-plugins-official` は settings.json で有効だが、
     `/home/node/.claude/plugins/cache/` は空・`clangd` も PATH になし
   - → C ファイル編集時の LSP（補完・診断）が効かない可能性。対処は `/plugin` で
     clangd-lsp を入れ直し（メタデータとキャッシュを現 HOME で再生成）
2. **`/workspace` ディスク使用率 88%（残 47G）** — 即危険ではないが要監視。
   `/data`（タイル本体）は 10% で余裕あり。
