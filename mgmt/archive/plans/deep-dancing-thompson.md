# トラッカー＆handover にセッションID トレーサビリティを追加

## Context（背景・目的）

課題管理トラッカー（`mgmt/tracker/track.py`）の各レコードは現在、ステータス遷移を
`history[]`（`{date, actor, from, to, comment}`）として記録しているが、**どの Claude Code
セッションでその操作が行われたか**を残していない。後から「この課題はどの会話で議論・対応
されたか」を辿る手段が、日時の突合しかなく不確実。

`$CLAUDE_CODE_SESSION_ID` 環境変数で各セッションの一意 id（jsonl ファイル名と一致）が確実に
取得できることを確認したため、これをトラッカーと handover に記録し、トレーサビリティを確立する。

### 設計方針（確定済み）

「どのセッションで何をいじったか」は、以下の **3 層**で追える設計とする。重い「内容編集ログ」を
history に焼く方式は採らない（肥大化回避）。

1. **session_id**（トラッカー history + handover 冒頭）… どのセッションで操作したか
2. **handover 本文**（「今回やったこと」要約）… そのセッションの作業内容（人間可読）
3. **git diff**（`issues.json`/`bugs.json` は git 管理）… フィールド単位の厳密な変更内容

→ history の session_id から同 id の handover を引き、必要なら git diff で厳密差分を確認できる。

**運用前提**: トラッカーを変更したセッションは handover を残す（handover 無しセッションの編集は
git diff でのみ追える）。

## 実装スコープ

### A. `mgmt/tracker/track.py`（中核）

import は `os`・`datetime` とも追加済み（28-32 行）。新規引数は追加せず、環境変数から自動取得する
（手動指定不要・形骸化防止）。

1. **session_id 取得ヘルパー追加**（`append_history`（94 行）付近）
   ```python
   def current_session_id():
       return os.environ.get("CLAUDE_CODE_SESSION_ID", "")
   ```
2. **`append_history` に session_id を追加**（94-101 行）
   - dict に `"session_id": current_session_id()` を追加
   - bug/issue 両方の `update --status`・`verify` がこの関数を経由するため、**1 箇所の修正で遷移時の
     session_id 記録が完結**（呼び出し: 376・423・684・729 行）
3. **`created_session_id` をトップレベルに追加**
   - `new_bug` の dict（240-259 行）に `"created_session_id": current_session_id()`
   - `new_issue` の dict（555-571 行）に同上
   - これらは add 時に生成されるため、登録セッションが自動記録される
4. **詳細表示（show）に反映**
   - `bug_show`（312-318 行）・`issue_show`（625 行付近）の history 表示行に session_id（短縮 8 桁）を併記
   - 登録セッション（`created_session_id`）も詳細に 1 行表示
5. **後方互換**: 既存レコードは新フィールドを持たないため、表示は必ず `.get(..., "")` 経由で
   フォールバック（`-` 表示）。`history[]` の旧エントリも `h.get("session_id")` で安全に扱う。

※ xlsx エクスポート（`reports/*.xlsx`）への列追加は今回**対象外**。session_id は長く列向きでないため、
  `show` コマンドと JSON 記録で十分。必要になれば別途。

### B. handover テンプレ（`/home/node/.claude/commands/handover.md`）

- 「引き継ぎノートの構成」冒頭（タイトル直下）に **セッションID・日時**の記録を必須化する項目を追加
  - 日時: `date '+%Y-%m-%d_%H%M'` で取得（既存ルールに準拠）
  - session_id: `$CLAUDE_CODE_SESSION_ID`
  - 記載例:
    ```
    - セッションID: 5222a314-a00e-4d70-979b-1cbcccf3f81d
    - 日時: 2026-06-15_HHmm
    ```
- ※ このファイルはグローバル（`~/.claude` 配下）でプロジェクト git 管理外 → コミット対象外。

### C. ドキュメント追従（`mgmt/tracker/CLAUDE.md`）

- フィールド説明に `created_session_id` / `history[].session_id` を追記
- 「session_id は `$CLAUDE_CODE_SESSION_ID` から自動記録される（引数指定不要）」旨を注記

## 検証

1. ダミー課題で一連の流れを確認:
   ```bash
   venv/bin/python3 mgmt/tracker/track.py issue add --title "session_id 検証用" --priority 低 --type 調査
   # → issues.json に created_session_id が入ることを確認
   venv/bin/python3 mgmt/tracker/track.py issue update <ID> --status 対応中 --actor Sonnet
   # → history エントリに session_id が入ることを確認
   venv/bin/python3 mgmt/tracker/track.py issue show <ID>
   # → 登録セッション・遷移セッションが表示されることを確認
   ```
2. **既存レコードの回帰確認**: `issue show ISSUE-001`（session_id 無しの旧レコード）が
   エラーなく `-` フォールバック表示されること
3. bug 側も同様に `bug add` → `bug update` → `bug show` で確認
4. 検証後、ダミーレコードは JSON から削除（または検証専用に残さない）
5. `git diff mgmt/tracker/track.py` で意図した差分のみか確認

## コミット方針

- `track.py` 改修 + `mgmt/tracker/CLAUDE.md` 追従を 1 コミット（Conventional Commits・本文日本語）
- handover.md はグローバル管理外のためコミット不要
- ダミー検証で変化した `issues.json`/`bugs.json` を元に戻してからコミット
