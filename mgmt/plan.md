# コミット運用の見直し：`git add .` 禁止の撤廃（列挙漏れ対策）

## Context

前回セッションで `mgmt/plan.md` のコミット漏れが発生。調査の結果、`git add .` 禁止ルールは
機密保護目的だったが `.gitignore` が完全カバー済みのため副作用（手動列挙漏れ）だけ残っていた。

## 変更内容

- CLAUDE.md の2箇所（handover ルール・ドキュメント更新ルール）を新運用に書き換え
- lessons.md に教訓を追記
- 守りは `git status` 目視 + `.gitignore` に一本化（hook/スクリプト追加なし）

## 対応完了

2026-06-18 — CLAUDE.md 修正・lessons.md 追記・コミット完了
