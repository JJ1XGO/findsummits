# 課題管理を GitHub Issues へ移行する（findsummits issue #1 対応）

## Context

claude-container 側で確立した GitHub Issues 運用（グローバル CLAUDE.md の opt-in 節「GitHub Issues による課題管理」）を findsummits に横展開する。対象は GitHub issue `jj1xgo/findsummits#1`（Fable 5 起票、移行手順6ステップを明記）。前提条件（コンテナ内 PAT の Issues Read/Write スコープ）は整備済みで、issue #1 自体が疎通テストを兼ねる。

**ユーザー合意済みの決定事項**（AskUserQuestion で確定）:

1. **バグも移行する**: bugs.json も `bug` ラベル付き issue として移行し、`mgmt/tracker/` を完全廃止する
2. **todo.md は維持**: issue/todo の二層構造（判定基準「残作業に文書・仕様の議論が必要か」）はそのまま
3. **ラベル体系は type+priority**: `type:改善/調査/設計` + `priority:高/中/低` + `on-hold` を新設、バグは既存 `bug` を流用

**実行モデル**: 全タスク Sonnet 直接対応（ユーザー指示: 計画=Fable、実装=Sonnet）。上位モデル委譲が必要なタスクなし。

## 現状（調査済み）

- 未解決バックログ: issue 14件（全て未対応）+ bug 6件（未対応5 + BUG-017 対応完了・verify待ち）
- 解決済・却下: issue 148件 / bug 11件（移行しない。履歴は git で参照可能なまま残る）
- `mgmt/tracker` への参照（履歴系=handovers/archive/incidents/lessons を除く）:
  - `/workspace/CLAUDE.md`（欠陥管理ルール・課題管理ルール・環境課題の連携 各節）
  - `.claude/todo.md` 冒頭ルール行（5行目）
  - `.claude/README.md` 82行目（permissions 表）
  - `.claude/settings.local.json` 6行目（track.py の permission、git 管理外）
  - `scripts/lint_docs.py` 40-42行目（コメントと ADR ファイル名のみ → 変更不要）
- session-start hook: `.claude/hooks/session-start.sh` に claude-container issue 注入ブロック（fail-soft）が既存。同型ブロックを findsummits 自身用に追加する

## タスク

### A. ラベル作成

`gh label create` で7個新設（`--repo jj1xgo/findsummits`）:

| ラベル | 色 | description |
|---|---|---|
| `type:改善` | `a2eeef` | 既存仕様の変更・拡張・整理 |
| `type:調査` | `d4c5f9` | 何かを決めるための情報収集・分析 |
| `type:設計` | `0e8a16` | 実装方針・アーキテクチャの判断 |
| `priority:高` | `d73a4a` | 優先度: 高 |
| `priority:中` | `fbca04` | 優先度: 中 |
| `priority:低` | `c2e0c6` | 優先度: 低 |
| `on-hold` | `fbca04` | 保留（本文に再検討トリガーを明記） |

バグ移行分は既存 `bug` ラベル（+priority）。

### B. バックログ棚卸し → 合意 → 一括起票

1. `track.py issue show` / `bug show` で未解決20件の全文（description/resolution/notes/history）を読み、**keep/drop 推奨リスト**を作成してユーザーに提示（AskUserQuestion または対話）。「死んだ課題はこの機会に落とす」（issue #1 の指示）
2. 合意後、keep 分を `gh issue create` で一括起票。本文フォーマット:
   - 冒頭に旧ID明記（例: `旧トラッカー: ISSUE-043`。詳細履歴は `git log -- mgmt/tracker/data/` で追跡可能な旨も一行）
   - description → 背景、resolution → 対応方針、notes → 補足、の順で転記
   - 末尾にモデル名署名（`— Sonnet 5`）
   - **公開リポジトリのため、ホスト固有パス・環境内部詳細を含めない**（転記前に各本文をチェック）
3. BUG-017（対応完了・verify待ち）は open で起票し「修正済み・ユーザー動作確認待ち」を明記
4. drop 分は理由一覧を記録し、後で issue #1 の対応完了コメントに含める（tracker 廃止後も判断根拠が残るように）

### C. CLAUDE.md ほかドキュメント追従

`/workspace/CLAUDE.md`:

- **欠陥管理ルール**節: フロー（全件洗い出し→ユーザー確認→登録→承認→着手）は維持し、登録先を `gh issue create`（`bug` ラベル）に置換。`mgmt/tracker/CLAUDE.md` 参照を削除
- **課題管理ルール**節: GitHub Issues 前提に書き換え。グローバル CLAUDE.md「GitHub Issues による課題管理（opt-in）」を参照する形で以下を定義:
  - 対象リポジトリ: `jj1xgo/findsummits`
  - ラベル体系（上表）
  - ステータス運用: 未対応=open無コメント / 対応中=着手コメント / 対応完了=完了コメント / 解決済=クローズ。**クローズは動作確認後にユーザーが行う（または指示を受けて AI が実行）**。現行の verify ゲートを維持
  - issue/todo 判定基準・「1 項目 1 課題」・impersonation 禁止（署名 `— <モデル名>`）は維持
  - 公開リポジトリ注意（内部詳細を書かない）
  - クローズは `gh issue close` を正とする（`fixes #N` は push まで閉じないため使わない。issue #1 の注意事項）
- **環境課題の連携**節: 「`mgmt/tracker` ではなく」の対比文言を新運用に合わせて微調整
- 追従: `.claude/todo.md` 5行目のルール文言、`.claude/README.md` 82行目の permissions 表、`.claude/settings.local.json` の track.py permission 行削除（git 管理外・コミット不要）

### D. session-start.sh 改修

`.claude/hooks/session-start.sh` の claude-container ブロックの直前または直後に、同型の findsummits 自身の open issue 注入ブロックを追加:

- `gh issue list --repo jj1xgo/findsummits --state open --json number,title,labels,updatedAt`（ラベルも表示に含める）
- fail-soft 設計を踏襲（`command -v gh` 判定 / `timeout 10` / 失敗時は一行メッセージで継続）
- AUTO-INJECTED REFERENCE ブロック形式・見出しで claude-container 分と区別

### E. 旧トラッカー廃止

- `git rm -r mgmt/tracker/`（track.py・data/・reports/・CLAUDE.md すべて。履歴は git で追える）
- `venv/` は lint 等で使用するため残す
- `grep -rn "mgmt/tracker"` で残存確認（履歴系ファイル=handovers/archive/incidents/lessons と `scripts/lint_docs.py` の ADR ファイル名参照は残って正常、と明記した上でゼロ確認）

### F. 検証・コミット・クローズ

1. `make lint` 警告ゼロ
2. `bash .claude/hooks/session-start.sh` 手動実行で findsummits issue ブロックが終端仕様どおり出力されること
3. `gh issue list --repo jj1xgo/findsummits` で移行 issue とラベルを確認
4. コミット（Conventional Commits・論理単位で分割。例: ①docs+hook の運用切り替え ②tracker 削除）。push は指示があるまでしない — ただし issue 起票は gh API 経由なので push 不要で反映される
5. issue #1 へ対応完了コメント（棚卸し結果の keep/drop 一覧を含む・署名 `— Sonnet 5`）。**クローズはユーザーが動作確認後に実施**（次回セッション開始時の hook 注入が実機確認になる）

## 検証方法（まとめ)

- hook 手動実行の出力目視 + 次回セッション起動時の自動注入（最終確認はユーザー側）
- `make lint` ゼロ
- `gh issue list` / `gh label list` で件数・ラベル照合
- `git status` クリーン・`git diff` で意図した変更のみか照合

## スコープ外

- `docs/` 配下（URD/SRS/HLD/LLD/ADR）は変更しない → spec-panel セルフレビューは規定によりスキップ
- コンテナイメージ・ビルドには触れない（リビルド不要。issue #1 の注意事項）
- handover・lessons・incidents・plan ファイルの運用は従来どおり（issue #1 手順6）
- 過去の解決済・却下レコードは issue 化しない（git 履歴で参照）
