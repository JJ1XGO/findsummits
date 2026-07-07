---
description: claude-container への GitHub issue 起票・対応完了確認・クローズの詳細フロー
---

# claude-container への issue 連携フロー

`findsummits` 自体ではなくコンテナ環境（claude-container）に起因する問題・要望を GitHub issue
で起票・追跡する際の詳細手順。**起票先の判定基準（findsummits か claude-container か）は
`CLAUDE.md`「環境課題の連携」節を参照**（本コマンドは判定後のフロー詳細）。

## フロー

起票 → （claude-container 側が調査・実装・対応完了コメント）→ 対応待ち →
**リビルド後**に動作確認 → 確認内容をコメントに付記してクローズ。

## ルール

1. クローズの原則・例外は `CLAUDE.md`「環境課題の連携」節を参照（本コマンドでは繰り返さない）
2. 動作確認は稼働中コンテナでは不十分になりうるため、リビルド（`-b`）後に行う
3. AI が起票・コメント・クローズする場合は、本文の**末尾に署名**として記入する。claude-container は
   findsummits 作業中のセッションから見て異なるリポジトリ（クロスリポジトリ連携）にあたるため、
   グローバル CLAUDE.md の署名ルールに従いモデル名にリポジトリ名を括弧書きで付記する
   （例: `— Sonnet 5 (findsummits)`）。経緯の説明文（「findsummits の作業中に起票」等）は書かない
4. 1 issue 1 論点（`CLAUDE.md`「課題管理ルール」の「1 項目 1 課題」と同じ）

## 注記

`gh` はコンテナ内セッションのみ利用可能（ホストセッションには無い）。挙動は `CLAUDE.md`
「課題管理ルール」節の hook 説明と同じ（対象リポジトリが `jj1xgo/claude-container` である点のみ異なる）。
