# SuperClaude の良いアイデアを取り入れる

## Context（なぜやるか）

ユーザーは SuperClaude_Framework の導入を検討したが、調査の結果、**本体（pipx install）導入は非推奨**と結論。
理由: 本フレームワークは「Claude Code に読み込ませる .md 指示ファイル群＋MCP」であり、その主要機能（plan/task/knowledge ドキュメント、記憶、課題管理、思考深度、要件深掘り）は、本プロジェクトが既に `mgmt/plan.md`・`todo.md`・`lessons.md`・`tracker/track.py`・handover・`memory/`・CLAUDE.md 運用として**自前でより厳密に**保持している。本体導入は二重管理・指示衝突を招き、過去に挙動不安定の原因となった外部コマンド群（`commands_disabled/` へ退避済み）と同じ轍になる。

そこで「考え方」だけを軽量に取り入れる。ユーザー選択により対象は次の2点に確定:

1. **Evidence-based（根拠主義）原則** — 明文化されていない開発規律を追加
2. **多視点パネルレビュー** — 仕様レビューが重い本PJ向けの自前 skill を新設

不採用（参考）: Token-Efficiency（出力圧縮）は素人ユーザー向けの明快な日本語説明と相反するため見送り。reflect/introspect は既存 handover/lessons ルーティンで代替済み。

## 作業1: Evidence-based 原則の追記（完了）

`/home/node/.claude/CLAUDE.md` の「## コア原則」に原則6として追記済み。
グローバルファイルのため Git 管理外。

## 作業2: 多視点パネルレビュー skill の新設（完了）

`/workspace/.claude/commands/spec-panel.md` を新規作成済み。
プロジェクトローカルに置くことで Git 管理・GitHub レビュー可能。

### 4つの視点（本PJの関心事に直結）
- アーキテクト: モジュール分担・ADR 整合・設計の一貫性・拡張性
- 仕様レビュアー: URD↔SRS↔HLD↔LLD トレーサビリティ・曖昧さ・抜け漏れ・矛盾
- データ/アルゴリズム: 境界条件・NODATA・サンプル代表性・性能/メモリ
- 申請者/エンドユーザー: SOTA 申請 O/P 要件・エビデンス妥当性

### 本PJのルール遵守（skill 本文に明記済み）
- 仕様優先原則: コードを正として参照しない
- 欠陥/課題フロー: 指摘は全件洗い出し後にまとめて提示、登録はユーザー承認後
- 自動 spawn 禁止: 既定はインライン実施

## 作業3: コンテキストウィンドウ使用率をステータスラインに表示（完了）

`~/.claude/settings.json` に `statusLine` フィールドを追加。グローバル設定のため Git 管理外。
表示例: `[Sonnet] コンテキスト 25%`

## 検証
- `/spec-panel FR-013` 等で動作確認（4視点の指摘一覧が出力され tracker 自動登録が起きないこと）
- コミット: `.claude/commands/spec-panel.md` をコミット（`git add -A` → Conventional Commits）
- statusLine: jq モック入力テスト・JSON 妥当性確認 → 完了

## やらないこと（スコープ外）
- SuperClaude 本体・MCP サーバのインストール
- Token-Efficiency / 思考深度フラグ / reflect の導入
- 既存 tracker・handover・lessons の改変
