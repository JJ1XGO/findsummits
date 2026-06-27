# 計画: master を fork元と同期し、独自作業を devel に分離

## Context（なぜ）

このリポジトリ（`jj1xgo/sotlas-frontend`）は本家 `manuelkasper/sotlas-frontend` の fork。
独自の地図ラベル言語切り替え改良（コミット3つ＋マージ）が `master` に乗っており、
本家（`upstream/master`）の更新を素直に取り込めない状態。

**目的**: `master` を `upstream/master` と完全一致させ、本家追従可能にする。
独自作業は `devel` ブランチへ退避して保全する。

前回セッション（handover 2026-06-27_1344）で `devel` 作成・バックアップまで完了。
本セッションは「master 巻き戻し → push」の残作業を、認証復旧の上で実行する。

## 現状（確認済み・2026-06-27）

- `master` = `fac54a5`（独自作業マージ済み、未巻き戻し）`[origin/master]` に push 済み
- `devel` = `fac54a5`（独自作業を完全保持）★退避先
- `backup/master-before-upstream-sync` = `0f1429f`（独自3コミットのみ、develに包含）
- `upstream/master` = `ca646a9`（同期目標）
- 独自コミット: `4676ec7`, `930da28`, `0f1429f`（＋マージ `fac54a5`）
  → **devel と origin/master の両方に保全済み**。消失リスクなし。
- **`gh` 未ログイン**（再起動で失効）。push 前に認証復旧が必須。

## 実装手順

### Step 0: GitHub 認証復旧（ユーザー操作・最優先）
push にはログインが必須。Claude からは実行できないため、ユーザーに依頼する。
- プロンプトで `!gh auth login`（GitHub.com / HTTPS）を実行してもらう
- 続けて `gh auth setup-git`（成功時は無出力が正常 — lessons 2026-05-16）
- 確認: `gh auth status` で Logged in を確認

### Step 1: upstream 最新化・状態再確認
```
git fetch upstream
git log --oneline -1 upstream/master   # ca646a9 から進んでいないか確認
```
upstream が進んでいた場合は同期目標が変わるため、その時点でユーザーに報告。

### Step 2: devel を origin へ退避 push
独自作業を GitHub 上にも残してから master を巻き戻す（安全のため先に実施）。
```
git push -u origin devel
```

### Step 3: master を upstream/master に巻き戻し
```
git checkout master
git reset --hard upstream/master       # fac54a5 → ca646a9
```

### Step 4: origin/master を force push（★破壊的操作）
GitHub 上の自分の fork の master から独自作業マージが外れ、upstream と一致する。
独自作業は Step 2 の origin/devel に残るため問題なし。
```
git push origin master --force-with-lease
```
`--force-with-lease` により、想定外のリモート更新があれば安全に失敗する。

### Step 5: backup ブランチの扱い（任意）
`backup/master-before-upstream-sync` は devel に包含され冗長。
当面は安全のためローカルに残す（削除はユーザー確認後に別途）。

## 検証

1. `git log --oneline -1 master` と `git rev-parse upstream/master` が一致（= `ca646a9`）
2. `git cherry -v upstream/master master` の出力が空（master に独自コミットなし）
3. `git log --oneline -1 devel` = `fac54a5`（独自作業保持）
4. `git ls-remote --heads origin` で `origin/master` = `ca646a9`、`origin/devel` 存在を確認
5. `git status` がクリーン

## 注意

- Step 4 は履歴を書き換える **force push**。独自作業は devel(ローカル/origin) に保全済みのため安全だが、実行前に再確認する。
- 今後の運用: `master` は upstream 追従専用、独自開発は `devel`。
  upstream 更新時は `master` を fetch→reset し、`devel` を rebase する想定（今回スコープ外）。
