# CLAUDE.md 課題管理ルール強化 ＋ 対応完了課題の verify

## Context
前セッション完了の SRS レビュー反映 9 件を本セッションで再レビュー（全件 SRS 反映済み・指摘なし）。
レビュー中にユーザーから課題管理の根本的な是正を受領（2026-06-13 合意）。
lessons/memory ではなく `/workspace/CLAUDE.md` に明文化して確実に遵守させる方針（ユーザー指定）。
ルール明文化を最優先で行い、その後に全 9 件を verify する。

## A. /workspace/CLAUDE.md「課題管理ルール」の強化（最優先）

### A-1. 課題と作業の線引き（厳格版）— セクションに以下の趣旨を追記
- **載せるのは「課題そのもの」**＝仕様検討・設計判断を行う必要のある**未解決の問い**。
  判断を文書へ反映する作業（SRS/ADR への反映・記述整理・注記追加・リンク化等）は、
  仕様判断を伴っていても課題ではなく `todo.md`。
- **回顧テスト**: 「解決後に振り返ってもこれは課題だと思えるか？」完了後に TODO に見えるものは元から TODO。
- **レビューの扱い**: 「FR-XX をレビューする／レビュー反映する」は作業 → `todo.md`。
  レビューで出た各設計論点（判断が要るもの）を **1 論点 1 課題**で個別に issue 登録する。
- **1 項目 1 課題**: 複数の課題を 1 項目に詰め込まない。あれば数だけ個別登録。
- **issue のスコープとクローズ**: issue は「問い＋その決着（決定＋ADR/SRS への記録）」まで。
  記録完了 = `対応完了`（Claude 視点のクローズ）。`解決済`（verify）はユーザー確認後。
  下流の機械作業（実装追従・命名・リンク化・コメント修正）は別 `todo.md`。

### A-2. AI 由来登録の名義ルール（既存 actor 行の近くに追記）
- AI（Claude / 各モデル）が登録・判断した課題・バグは、`報告者`・`--actor` ともにモデル名
  （Fable / Opus / Sonnet 等）を記入する。ユーザー名を充ててはならない（impersonation 防止）。

### A-3. コミット
- CLAUDE.md 編集後そのターン内で個別指定 commit（Conventional Commits・本文日本語）。

## B. トラッカー名義訂正
- ISSUE-088 の `報告者` を JJ1XGO → Fable に訂正（track.py での更新手段を確認。
  報告者を更新する引数が無ければ JSON 直編集の可否をユーザーに確認）。

## C. verify バッチ（全 9 件・ユーザー承認済み）
```
issue update ISSUE-086 --stage SRS --actor Opus   # COD → SRS
issue verify ISSUE-082 --actor Opus
issue verify ISSUE-086 --actor Opus
issue verify ISSUE-075 --actor Opus
issue verify ISSUE-088 --actor Opus
issue verify ISSUE-089 --actor Opus
issue verify ISSUE-090 --actor Opus
issue verify ISSUE-091 --actor Opus
issue verify ISSUE-051 --actor Opus
issue verify ISSUE-052 --actor Opus
```
- export 条件付き再生成: `issue export --if-changed`

## 検証
- `issue list --status 解決済` で 9 件が反映されたことを確認
- `git log --oneline -1` で CLAUDE.md コミットを確認
- `issue show ISSUE-088` で報告者が Fable になったことを確認

## フォローアップ（本バッチ外・別途ユーザー判断）
- **既存「レビュー反映」系 issue の棚卸し**: 新基準では ISSUE-087「FR-005 ピーク候補検出のレビュー反映」は
  *作業* なので todo 降格候補。他の同型 issue も含めて棚卸しする（方針確定済み・実施はユーザー指示後）。
- 091 残: 実装後に石鎚山が海面確定規則で自動確定するか要検証（リスト追加判断）。
- 051/052 は新基準では TODO 相当だったが既に完了済みのため verify でクローズ。
