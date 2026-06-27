# コミット運用の見直し：`git add .` 禁止の撤廃（列挙漏れ対策）

## Context

前回セッションで `mgmt/plan.md` のコミット漏れが発生した。原因調査の結果：

- 直接原因は「`git add .` 禁止・個別ファイル指定」運用での **列挙漏れ**（`docs/` は add したが `mgmt/plan.md` を列挙し忘れた）。
- このルールの目的（機密・個人情報の誤コミット防止）は、**`.gitignore` が既に完全カバー済み**であることを確認した：
  - `params/config.ini`・`params/fetch_config.ini`・`.claude-container`・`.claude/settings.local.json`・`.claude/handovers/` 全て ignore 済み。
  - `git ls-files` で機密候補を探しても **リポジトリには1件も入っていない**。
- つまりルールが守るべきものは `.gitignore` が二重に守っており、ルールは「手動列挙 → 列挙漏れ」という **副作用（今回の事故）だけ** 残していた。

→ hook で事後検出する対症療法ではなく、**原因（手動列挙の強制）そのものを撤廃**する。守りは `git status` の全体目視＋`.gitignore` に一本化する。

## 変更内容

`git add .`/`-A` 禁止を撤廃し、「コミット前に `git status` 全体を目視 → 過不足なければ `git add -A`」へ。これで列挙漏れ（過少）も野良ファイル混入（過剰）も `git status` 目視の1ステップで防ぎ、機密は `.gitignore` が防ぐ。仕組み（hook/スクリプト）は一切足さない。

## 作業ステップ

### 1. `/workspace/CLAUDE.md` の2箇所を書き換え

- **L287（handover 実行時のルール 3項）**
  - 現: `ファイルを**個別指定**で \`git add <files>\` してコミット（\`git add .\` / \`git add -A\` は使わない）`
  - 新: `git status` 全体を目視し意図したファイルが過不足なく含まれることを確認のうえ `git add -A` でまとめて追加してよい旨へ（機密は `.gitignore` 除外済み／野良ファイルが混ざる場合のみ個別指定）。
- **L304（ドキュメント更新時のルール 2項）**
  - 現: `**個別ファイル指定**で \`git add <files>\`（\`git add .\` / \`-A\` は禁止）`
  - 新: 直前の `git status` で過不足を確認できたら `git add -A`（意図しない野良ファイルが見えた場合のみ個別指定）へ。
- L285 の「機密・gitignore 対象が含まれていないことを確認」は **残す**（目視習慣として有効）。新文と矛盾しないよう「`.gitignore` 済みだが念のため未追跡ファイルに想定外がないか確認」のニュアンスに整える。

### 2. `mgmt/lessons.md` に教訓を追記

- 事象：個別 add 運用で plan.md の列挙漏れ（Opus でも発生＝賢さでなく手順の問題）。
- 原因：`git add .` 禁止が手動列挙を強制し、列挙漏れという別事故を生んだ。守るべき機密は `.gitignore` が完全カバー済みでルールは副作用だけ残っていた。
- 対策：`git status` 全体目視 → `git add -A`。原因を消す。

### 3. `mgmt/plan.md` を本計画内容で更新

- plans/ の本ファイルを `mgmt/plan.md` へ反映（グローバル運用ルール）。
- 注：現 `mgmt/plan.md` は前回 FR-009 計画の控え（コミット漏れ分）だが、FR-009 作業自体はコミット済み（119865e）で実害なし。本計画で上書きしてコミットすることで、当初の「plan.md コミット漏れ」も同時に解消する。

### 4. コミット（新運用の初適用）

```
git status                       # 全体目視（CLAUDE.md / lessons.md / plan.md の3点 + 想定外がないこと）
git add -A                       # 過不足なしを確認のうえ
git commit
```
コミットメッセージ案（Conventional Commits・本文日本語）:
```
docs: コミット運用をgit status目視+add -Aに見直し（列挙漏れ対策）

- git add . 禁止ルールを撤廃。機密は .gitignore が完全カバー済みのため
  個別列挙は不要で、むしろ列挙漏れ事故を生んでいた
- handover / ドキュメント更新ルールの該当2箇所を更新
- lessons.md に教訓を記録
```

## 検証

```bash
grep -n "git add \." /workspace/CLAUDE.md            # 旧禁止記述が消えていること（0件）
grep -n "git add -A" /workspace/CLAUDE.md            # 新運用が2箇所に入っていること
git status                                           # コミット後クリーンであること
```
- 上記に加え、CLAUDE.md の2セクション（handover／ドキュメント更新）が新運用で整合していること、L285 の目視確認文言と矛盾がないことを目視確認する。
