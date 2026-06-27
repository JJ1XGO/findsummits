# 計画: 外部コマンド侵入元調査のクローズと handover 作成

## Context（なぜこの作業か）

「先日隔離した外部オーケストレーターコマンド（ユーザー表現では実行ファイル）がどこから侵入したか」を確定したい、というのが本セッションの依頼。
コンテナ内から可能な範囲で調査した結果、**到達可能範囲（コンテナ）では発生源の確定的特定は不可能**と判明。ユーザーはホストのバックアップから restore し別の解析者に依頼する方針を決定。
本計画は、調査結果と反省点を handover ノートとして残してクローズすることが目的。

## 実施事項（このplan承認で行うこと）

1. plan mode を抜ける
2. `.claude/handovers/2026-06-19_2325.md` を**新規作成**（下記内容）
3. **コミットはしない**（`.claude/handovers/` は gitignore 済み・トラッカー変更も無し）
4. それ以外のファイル変更・実行はしない

## 作成する handover ノートの内容（確定稿）

---

# Handover: 2026-06-19_2325

セッションID: `592efbe6-2816-4006-8a0b-17fcc1b84d36`
日時: 2026-06-19_2325

## 今回やったこと
- 「先日隔離した外部コマンドの侵入元」調査を実施
- best-practices ドキュメント確認 → 外部コード混入経路（commands/skills/agents/hooks/MCP/plugins/CLAUDE.md import）を整理
- 隔離物の正体特定 → バイナリではなく7個の自律オーケストレーター meta-prompt（`.md` スラッシュコマンド）
- タイムスタンプ解析・マウント判定・Web検索・ユーザー提示記事2件の評価

## 決定事項（＝確実に判明したこと）
- 隔離コマンド7件（adaptive/agentflow/controlflow/context_pipeline/devops/taskengine/taskflow）の実体は **ホスト側** `/home/tsu/.claude/commands_disabled/`。コンテナの `~/.claude` は `/dev/nvme0n1p5[/tsu/.claude]` の bind mount。**コンテナは無関係**。
- 11ファイル（上記7 + pr-draft/pr-review/refactor/tdd）は mtime `2026-04-23 15:15:33`、同一ミリ秒で一括生成。
- `history.jsonl`（最古 2026-04-20）にコマンド名・導入プロンプトの痕跡ゼロ。`/plugin` インストール記録なし。04-23 のシェルスナップショットなし。

## ユーザーの記憶（裏付け・ただし未証明）
- 最初のインストール時に「追加で何かインストールしますか?」という確認プロンプトが出て、その後 warning のような表示が出ていた。ユーザーはこの時に混入したと認識。
- → インストーラがオプション部品（=7オーケストレーター）を同梱提案し取り込んだ、という像と整合。具体パッケージ名・配布元はホスト側ログで確定要。

## 未確定のこと（誤った前提で進めない）
- 何が（どの npm/ツール/操作が）書いたか＝未特定
- 配布元サイト・パッケージ名＝未特定。ユーザー提示の2記事（note.com/kuroiosarusan, maasaablog）はいずれも正規 CLAUDE.md ガイドで7コマンドを配布していない
- 悪意の有無＝未判定（マルウェアと確認されたわけではない）

## 捨てた選択肢と理由
- 「npx インストーラ説」を**確定として表(✓)で提示** → 誤り。証拠と整合する仮説ではあるがコンテナ内から証明不能。確定扱いしてはならない（ユーザーから「曖昧なまま先へ進める」と正当な叱責）。

## ハマりどころ / 反省
- 不確実な再構成を確信度高く提示してしまった。仮説は「仮説」と明示し確定事項と分離すべきだった。
- 環境調査では最初に「到達可能範囲（コンテナからはホストの npm ログ・bash 履歴・backup に届かない）」を明言すべきだった。

## 学び
- 不確実な再構成は確信度を演出せず、確定/未確定を分けて提示する。
- 調査着手時に観測可能範囲を確定しユーザーへ共有する。

## 次にやること（ホスト側・コンテナ外。別解析者向け）
- `~/.npm/_logs/` の 2026-04-23 前後を確認（インストール時の optional dependency / warning 痕跡）
- `grep -E 'npx|npm i' ~/.bash_history | grep -iE 'claude|agent|flow|orchestrat'`
- ホストの `~/.claude/commands_disabled/` 7ファイル本体を解析（mtime `2026-04-23 15:15:33`）
- ユーザー: ホストバックアップから restore して別 LLM に解析依頼予定

## 関連ファイル
- `/home/node/.claude/commands_disabled/*.md`（実体=ホスト `/home/tsu/.claude/commands_disabled/`）
- `/home/node/.claude/commands/{pr-draft,pr-review,refactor,tdd}.md`
- `/home/node/.claude/history.jsonl`
- 既存メモリ `env_foreign_orchestrator_commands.md`（要追記: ホスト側 bind mount・侵入元未特定・ユーザー記憶=追加install確認+warning）

---

## 検証
- 書き出し後 `ls -la ~/.claude/handovers/2026-06-19_2325.md` で生成を確認
- git status は変更なし（gitignore 済み）であることを確認
