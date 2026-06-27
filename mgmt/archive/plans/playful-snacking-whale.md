# 計画: ~/.claude/plans/ 計画残骸を git 履歴に保存して削除

## Context

`~/.claude/plans/` に `/plan` コマンドが生成した計画ファイルが 86 本蓄積している（git 管理外）。
本プロジェクトの運用ルールでは承認後に `mgmt/plan.md` へ mv するはずが、移動し忘れて蓄積したもの。
git 管理外のため紛失リスクがあるため、一度 git 履歴に取り込んでから削除し、
`git show <hash>` で内容を参照できる状態にする。

前回セッション（b4ce88bd）でインジェクション検知のため作業を中断。
環境確認チェックリスト通過済み・tool_result 正常。

## 前提確認（調査済み）

- `mgmt/archive/` は git 追跡対象・lint 除外済み → 退避先として問題なし
- `mgmt/archive/plans/` サブディレクトリは未作成（新規作成する）
- `~/.claude/plans/` には `.md` と `.done` 拡張子のファイルが混在する可能性あり
- Stop hook・インシデント対策フックはすべて実装済み → 86 本はすべて完了済み計画の残骸

## 作業手順【Sonnet】

### タスク 1: ファイルをコピーして git に追加

```bash
mkdir -p /workspace/mgmt/archive/plans
cp /home/node/.claude/plans/* /workspace/mgmt/archive/plans/
ls /workspace/mgmt/archive/plans/ | wc -l   # 86 本確認
```

### タスク 2: add コミット（履歴に残す）

```bash
cd /workspace
git add mgmt/archive/plans/
git status   # 86 本が staged されていることを確認
git commit -m "chore: ~/.claude/plans/ 計画残骸86本を git 履歴に保存"
```

### タスク 3: git rm して削除コミット

```bash
git rm -r mgmt/archive/plans/
git status   # 86 本が deleted としてリストされることを確認
git commit -m "chore: mgmt/archive/plans/ を削除（git 履歴は保持）"
```

### タスク 4: オリジナルファイルを削除

```bash
rm -rf /home/node/.claude/plans/*
ls /home/node/.claude/plans/   # 空になったことを確認
```

## 検証手順

```bash
# 追加コミットの内容確認（86 本が含まれるか）
git log --oneline -3
git show HEAD~1 --stat | tail -5   # add コミットに 86 本あるか

# git 履歴からファイルを参照できるか確認
git show HEAD~1 -- mgmt/archive/plans/snappy-dreaming-nebula.md | head -5

# plans/ が空になったか確認
ls /home/node/.claude/plans/ | wc -l   # 0
```

## 注意事項

- `mgmt/archive/` は lint 除外済みのため、`make lint` への影響なし
- タスク 4（rm）はタスク 3 のコミット完了後に実行すること
- `cp` でコピーされないドットファイル（`.gitignore` 等）が plans/ にある場合は個別対応
