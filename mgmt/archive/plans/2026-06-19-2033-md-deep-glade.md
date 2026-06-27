# 計画: 課題 type「機能追加」廃止 + 対話入力UI改善

## Context

`mgmt/tracker/track.py` の課題 type「機能追加」が「実装タスク（SRS確定済みFRの実装）」と
「本物の設計課題」の両方を飲み込み、issue が todo 的に使われる温床になっている。
2026-06-19 にユーザー承認済みの方針（type 廃止 + 再分類 + 対話UI改善）を実装する。
あわせて、ユーザー自身が課題管理 CLI を使えていない（選択肢を覚えられない・各フィールドに
何を書くか分からない）問題を、対話入力の番号選択メニュー化＋フィールドガイドで解消する。

検証で判明した前提補正:
- handover 記載の `normalize_type()`/`TYPE_ALIASES` は**存在しない** → エイリアス廃止作業は不要
- 未解決の `type=機能追加` は **9件**（元計画の6件＋032/040/044）。全件再分類する（ユーザー承認済み）

## 作業 A: type「機能追加」廃止（コード）

対象: `mgmt/tracker/track.py`

1. `VALID_TYPES`（43行）: `["機能追加", "改善", "調査", "設計"]` → `["改善", "調査", "設計"]`
   - 廃止理由＋再追加禁止をコメントで明記
2. ヘルプ文（24行付近）の `--type 機能追加` 例を `--type 改善` 等に差し替え
3. 既存データ表示互換: summary（763, 857行）・export（832行）は `VALID_TYPES` をループするだけなので、
   廃止後は「機能追加」を集計表に出さない。再分類で全件 valid な type に移すため宙に浮くデータは残らない
   （万一の歴史データは show コマンドで生の値をそのまま表示＝fallback で互換維持。要コード確認）
- argparse の `--type` choices（911, 912行）は `VALID_TYPES` 参照のため自動で「機能追加」が外れ、
  `--type 機能追加` は argparse エラーになる（追加実装不要）

対象: `mgmt/tracker/CLAUDE.md`
- type 表から「機能追加」行を削除、廃止理由＋再追加禁止を明記
- 「有効な値」表の `type (Issue)` を `改善, 調査, 設計` に更新
- コマンド例の `--type 機能追加` を更新

対象: `/workspace/CLAUDE.md`
- 「課題管理ルール」に1行追記: 「FR確定済みの実装は todo、issue type は 改善/調査/設計 の3種」

## 作業 B: 対話入力モードの改善（番号選択＋ガイド）

対象: `mgmt/tracker/track.py` の `ask_interactive`（79-92行）

1. シグネチャに `guide=None` を追加
2. `guide` があればプロンプト前にグレー表示で「書く内容の説明＋記入例」を出す
3. `choices` がある場合は番号メニュー表示（例: `1) 改善  2) 調査  3) 設計`）。
   入力は**番号・文字列の両方を受理**し、`default` も従来通り反映。標準ライブラリのみ（依存追加なし）
   - questionary 等の矢印キーTUIは環境依存のため不採用（コンテナで確実に動く方式）

対象: `issue_add`（671-689行）/ `bug_add`（同等の対話ブロック）の各 `ask_interactive` 呼び出し
- 主要フィールドに `guide` を付与。文面の方針:
  - title: 「主題を一行で。例: SRS: RTM の追加」
  - description: 「課題（問い）の背景・現状・困りごと」
  - type: 「改善=既存仕様の変更/整理、調査=決めるための情報収集、設計=実装方針の判断」
  - resolution: 「仕様検討の決着・対応方針」
  - priority/stage/category 等にも一行ガイド

## 作業 C: 既存 issue 再分類（9件・ユーザー承認済み）

| ISSUE | 操作 |
|---|---|
| 062 / 063 / 066 | `issue close` 理由「todo.md に移行（type機能追加廃止に伴う）」→ `mgmt/todo.md` 高/中へ転記 |
| 070 / 071 / 040 / 044 | `issue update --type 設計 --actor Opus --comment "type機能追加廃止に伴う再分類"` |
| 032 | `issue update --type 改善 --actor Opus --comment "同上"` |
| 009 | `issue update --type 調査 --actor Opus --comment "同上"` |

- 解決済/却下の歴史データ（002/003/004/014/015/048/049/050/055/091/102 等）は**据え置き**（生値で表示互換）

## 作業 D: 運用変更を issue 1件で記録

```
issue add --type 設計 --priority 中 --actor Opus \
  --title "課題 type『機能追加』廃止 — 実装タスクの todo 化を防ぐ" \
  --description "..." --resolution "VALID_TYPES から廃止＋対話UI改善＋既存9件再分類"
```
- 採番された ISSUE 番号を、A/B/C の文書注記（mgmt/tracker/CLAUDE.md 等）の根拠として記載

## 検証

1. `venv/bin/python3 mgmt/tracker/track.py issue add --type 機能追加 --title x` → argparse エラーで弾かれる
2. `issue summary` → 種別別に「機能追加」が出ない／3種のみ
3. `issue show ISSUE-070` → type=設計、`ISSUE-009` → type=調査
4. 引数なし `issue add`（対話）→ 番号メニュー＋ガイドが表示され、番号で type 選択できる（Ctrl-C で中断）
5. `python3 -c "import json; json.load(open('mgmt/tracker/data/issues.json'))"` で JSON 妥当性
6. `mgmt/todo.md` に 062/063/066 が転記済み・文書3ファイル（track.py コメント / tracker CLAUDE.md / project CLAUDE.md）の整合を目視

## コミット

作業完了後にまとめて1コミット（Conventional Commits・本文日本語）:
`chore(tracker): 課題 type「機能追加」を廃止し対話入力を番号選択+ガイド化`
- ドキュメント（CLAUDE.md 2ファイル）も同コミットに含める

## モデル推奨

設計判断は対話で確定済み、残りはファイル編集中心（guide 文面・todo 転記文程度）のため **Sonnet 推奨**。

## 注記

- ToolSearch は本環境で壊れているため**使用禁止**。本作業は Read/Edit/Bash/Write のみで完結し deferred tool 不要
- 再分類の `--actor` はモデル名（Opus）を記入（impersonation 禁止ルール）
