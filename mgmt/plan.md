# todo.md 運用ルールが守られない問題の原因と修正計画

## Context

2026-06-07（コミット `fc9cb8c`）に「仕様議論を伴わない作業は issue ではなく `mgmt/todo.md` で管理する」ルールを制定したが、翌 06-08 の FR-015 レビューで派生タスク ISSUE-082〜086 が一括 issue 登録され、ルールが守られなかった。どこを直せば再発を防げるかを調査した。

## 調査結果

### 違反の実態（ルール制定 06-07 以降の登録分）

| ID | 内容 | 判定 |
|---|---|---|
| ISSUE-086 | mesh_analyze.h のコメント修正 | **明確な違反**。tracker/CLAUDE.md の「登録すべきでない例（コメント整理）」に該当 |
| ISSUE-082 | SRS/URD 裸参照の一括リンク化 | **違反**。リンク化方針は CLAUDE.md に追記済みで、残作業は機械的変換のみ |
| ISSUE-085 | elev_to_rgb() ハードコード外出し | issue 維持が妥当（外出しの形式は HLD/LLD の設計判断を伴う） |
| ISSUE-083/084/087〜089 | SRS レビュー・設計検討 | issue で正しい |

### ルールが守られなかった構造的原因

ルール自体は 3 箇所（`/workspace/CLAUDE.md`・`mgmt/tracker/CLAUDE.md`・`mgmt/todo.md` 冒頭）に記載済みで、記載漏れではない。原因は以下:

1. **登録手順に判定ステップがない**: `/workspace/CLAUDE.md`「課題管理ルール」の手順 1 が「新規の課題が発生したら issue add で登録する」で始まり、判定基準は手順の外（前置き）にある。登録の瞬間に判定が強制されない
2. **「出自」で誤判定しやすい**: レビュー派生タスクは「仕様レビュー由来＝仕様議論を伴う」と誤判定される。実際は仕様議論はレビューで完了済みで、残作業（コメント追従・リンク化）は機械的。判定は「出自」ではなく「残作業」で行うべきことが明文化されていない
3. **tracker/CLAUDE.md のコマンド集にリマインダーがない**: `issue add` のコマンド例を参照して登録する瞬間に判定基準が目に入らない（概念定義は同ファイル冒頭にあるが、コマンド節から遠い）
4. **handover テンプレの矛盾で todo.md の存在感が薄い**: `/workspace/CLAUDE.md`「ToDo リスト運用ルール」は「handover には todo.md と issue の未対応分を抜粋して載せる」と定めるが、「handover 実行時のルール」のサマリーテンプレは bug/issue のみ。実際の handover（例: 2026-06-12_1015.md）にも todo.md 残件が載っておらず、セッションを跨ぐと todo.md が想起されない
5. **lessons.md に未記録**: この運用変更に関する学びが `mgmt/lessons.md` にない

## 修正計画（ユーザー確認済み: ルール文面の修正のみ。hook は今回見送り）

### 1. `/workspace/CLAUDE.md` の修正

- **「課題管理ルール」手順 1 を判定込みに書き換え**:
  - 登録前に必ず判定基準「**残作業に**文書・仕様の議論が必要か？」を適用し、No なら todo.md へ追記（issue add しない）
  - 「判定は出自ではなく残作業で行う。レビューで仕様が確定済みで、残りが実装側追従（コメント修正・命名追従・機械的変換等）だけなら todo.md」を明記
  - 「レビュー派生タスクを複数件まとめて登録するときも 1 件ずつ判定する」を明記
- **「handover 実行時のルール」のサマリーテンプレに todo.md セクションを追加**（「ToDo リスト運用ルール」との矛盾解消）:
  ```
  ### todo.md 残件（高=x / 中=y / 低=z）
  - 高優先のみタイトルを抜粋
  ```

### 2. `mgmt/tracker/CLAUDE.md` の修正

- 「課題管理コマンド」節の `issue add` コマンド例の直前に登録前チェック 1 行を追加:
  「**登録前チェック**: 残作業に文書・仕様の議論が必要か？ No なら `mgmt/todo.md` へ（冒頭の『登録すべきでない例』参照）」
- 「判定基準」を「残作業に議論が必要か（出自がレビューでも、議論済みで作業だけなら todo.md）」に明確化

### 3. `mgmt/lessons.md` に学びを記録

- 「レビュー派生タスクは出自で issue と誤判定しやすい。判定は残作業で行う」を追記

### 4. 既存違反の是正

- ISSUE-086・ISSUE-082 の内容を `mgmt/todo.md` の該当優先度セクションへ転記（既存の「(元 ISSUE-XXX)」形式に倣う）
- `venv/bin/python3 mgmt/tracker/track.py issue close ISSUE-082 --actor "Fable" --comment "todo.md に移行"`（086 も同様）
- ISSUE-085 は issue のまま維持
- `track.py issue export --if-changed` で xlsx 更新

### 5. コミット

- 変更ファイルを個別指定で `git add`（CLAUDE.md / mgmt/tracker/CLAUDE.md / mgmt/todo.md / mgmt/lessons.md / mgmt/tracker/data/issues.json / mgmt/tracker/reports/issues_export.xlsx）
- `chore(mgmt): todo.md 運用ルールの判定ステップ組み込みと違反 issue の移行` 形式でコミット（本文日本語）

## 検証

- `venv/bin/python3 mgmt/tracker/track.py issue list --open` に ISSUE-082/086 が出ないこと（対応完了になっていること）
- `mgmt/todo.md` に 2 件が転記されていること
- CLAUDE.md 内で「ToDo リスト運用ルール」と「handover 実行時のルール」の記述が矛盾しないこと
- `git status` がクリーンであること
