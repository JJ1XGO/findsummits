# 課題管理と ToDo リストの分離整備

## Context

現状、開発タスクの管理が課題管理トラッカー（`mgmt/tracker/`）に過度に集中しており、「文書・仕様に紐づかないオペレーション作業」も Issue として登録されている。`mgmt/todo.md` は実在するが、ルールが todo.md 自身にしか書かれておらず、グローバル/プロジェクト CLAUDE.md からリンクされていないため、運用に組み込まれず死蔵状態。

調査結果：
- 全 Issue 81 件中、約 16%（13 件）が作業 ToDo 的（「ファイル名追従」「ログ追記」「実装修正」レベル）
- `mgmt/tracker/CLAUDE.md` には Issue/Bug の有効値（type など）の定義はあるが、「何を Issue として登録すべきか」の概念的線引きがない
- 結果、handover の「次にやること」を選ぶ際に Claude が「これは Issue か todo か」で判断を迷う

### 目指す状態

| 管理場所 | 対象 |
|---|---|
| `mgmt/tracker/` issue | プロジェクトの**仕様・設計・調査・新機能**（文書・仕様の議論を伴うもの） |
| `mgmt/tracker/` bug | 動作不良・欠陥 |
| `mgmt/todo.md` | 上記以外の作業リスト（実装タスク・ファイル名追従・修正作業・運用作業 等） |
| `mgmt/plan.md` | 現在進行中の実装計画（既存運用維持） |
| `mgmt/lessons.md` | 学び・避けるべきパターン（既存運用維持） |

判定基準: **「文書・仕様の議論を伴うか？」が機械的判定軸**。Yes → issue、No → todo.md。

## 修正対象ファイル

### 1. `/workspace/CLAUDE.md`

**「課題管理ルール」セクション（L195前後）の改訂**:

- 現状: 「開発タスク（機能追加・改善・調査・設計）は track.py issue で管理」
- 改訂後: 「**プロジェクトの仕様・設計・調査・新機能（文書に紐づくもの）は track.py issue で管理**」「**それ以外の作業リスト（実装タスク・運用作業等）は `mgmt/todo.md` で管理**」と境界を明示

**ToDo リスト運用ルール節を新規追加**（課題管理ルール節の直後）:

- `mgmt/todo.md` の用途・書き方・粒度
- handover の「次にやること」との関係（todo.md と一部重複する場合の方針）
- Issue から todo.md に降格させる判定基準

### 2. `/workspace/mgmt/tracker/CLAUDE.md`

**「Issue と Bug の概念定義」セクションを新規追加**:

- Issue = プロジェクトの仕様・設計・調査・新機能（文書議論を伴う）
- Bug = 動作不良・欠陥
- **Issue に登録すべきでない例**: 単独のファイル修正・ログ追記・ファイル名追従（→ todo.md へ）
- type 有効値（機能追加/改善/調査/設計）の各定義を補足

### 3. `/workspace/mgmt/todo.md`

- 現状の旧メモ（アーキテクチャ刷新時のチェックリスト）はクリア
- 新運用テンプレートに置き換え:
  - 冒頭にルール（用途・判定基準・Issue との使い分け）を明記
  - セクション例: 「進行中」「次回着手」「保留」
  - 完了したものは消す or 取り消し線（運用上の好みをユーザーに確認しつつ決定）

### 4. 既存 Issue の棚卸し

**未対応 Issue から ToDo 寄りを抽出** → ユーザーに一覧提示 → 承認後に処理:

- 「Issue に残す」: 仕様・設計議論を含む
- 「todo.md に転記して Issue クローズ」: 単独オペレーション
- 「却下/不要」: 既に意義を失っている

候補（要再確認）: ISSUE-013（prefetch 取得範囲修正）、ISSUE-056（prefetch FR-001 仕様追従）、ISSUE-059（実装側ファイル名追従）、ISSUE-064（gsi_tile_latest_date を merged に）、ISSUE-079（preprocess_pref_boundaries.py 仕様追従）など。

**判定は実装フェーズで全未対応 Issue を 1 件ずつ確認してから提示**（独断で動かさない）。

## 実装手順

1. CLAUDE.md の課題管理ルール改訂・ToDo 節新規追加
2. mgmt/tracker/CLAUDE.md に Issue/Bug 概念定義追記
3. 全未対応 Issue を読み、棚卸し候補一覧をユーザーに提示
4. ユーザー承認後、対象 Issue を mgmt/todo.md に転記 → Issue クローズ（理由欄に「todo.md に移行」と記載）
5. mgmt/todo.md の中身をクリアし、棚卸しで移ってきた項目で再構成
6. ドキュメント変更分をまとめて 1 コミット（Conventional Commits 形式・本文日本語）

## 検証方法

- `/workspace/CLAUDE.md` の改訂内容を実際に読み、Claude が次セッションで判定軸として使えるか確認
- `mgmt/todo.md` 冒頭ルールを Claude が読んだ際に迷わない記述か確認
- 棚卸し後の Issue 一覧（`venv/bin/python3 mgmt/tracker/track.py issue list --open`）が「仕様・設計・調査・新機能」に絞られているか目視
- 棚卸し後の todo.md が「作業リスト」として機能する粒度になっているか目視

## 影響範囲

- ドキュメントのみ（コード変更なし）
- 既存 Issue データの ID は維持（移動した Issue は理由欄に「todo.md に移行」と記載してクローズ）
- handover・plan.md・lessons.md の運用は現状維持
