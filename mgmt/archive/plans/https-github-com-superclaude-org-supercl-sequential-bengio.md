# Context7（リモートHTTP版）MCP の導入

## Context（なぜやるか）

ユーザーから2点の確認依頼:
1. グローバル CLAUDE.md にサブエージェント積極活用が書かれているか → **既に原則2「サブエージェントを活用」に明記済み・採用済み。対応不要**（本プランの作業対象外）。
2. 本PJで採用すべき MCP があるか → 調査の結果、Context7（リモート版）の1点のみ採用と確定。

### MCP 調査結果（根拠）
- 本環境は `node/npx`・`uvx/pipx/uv`・`docker` がいずれも**未インストール**。ローカル起動型 MCP は実行基盤の追加導入が必要で、コンテナ再ビルドで消える恐れ・過去の不安定化リスクがあるため不採用。
- filesystem/git/fetch 系 MCP は Claude Code 内蔵ツールと重複 → 不要。
- 唯一「追加導入なし＋実利あり」なのが **Context7 リモート版**（Streamable HTTP・node等不要・無料枠は APIキー不要）。先日追加した「根拠主義（Evidence-based）」原則を直接補強し、最重要 O/P の openpyxl（申請用Excel生成）や geojson の最新ドキュメント取得に有効。
- 増分的価値（内蔵 WebFetch/WebSearch でも公式docは引けるが、ライブラリ解決の確実さ・速さで上回る）。リスク低（読み取り専用・リモート・送信内容はライブラリ名/クエリのみで機密データ無し）。

ユーザー選択により**ユーザースコープ（全プロジェクト共通・git非管理）**で追加と確定。

## 実装

ユーザースコープに Context7 リモート MCP を1つ追加する（無料枠のため APIキー無し）:

```bash
claude mcp add --scope user --transport http context7 https://mcp.context7.com/mcp
```

- スコープ `user` → `~/.claude.json`（git非管理）に記録され、全プロジェクトで利用可能
- transport `http` + エンドポイント `https://mcp.context7.com/mcp`
- APIキーは無料枠で不要（レート上限を上げたい場合のみ後から `--header "CONTEXT7_API_KEY: ..."` で再追加可能）

## 検証

1. 登録確認:
   ```bash
   claude mcp list
   ```
   → `context7` が表示され、接続状態が `✓ Connected`（または同等）であること
2. 反映: MCP ツール（`resolve-library-id` / `get-library-docs`）は**次セッション以降**にツール一覧へ追加される（現セッションには反映されない）
3. 動作確認（次セッション以降）: 「openpyxl の最新ドキュメントを Context7 で引いて」等で get-library-docs が呼べること

## やらないこと（スコープ外）

- グローバル CLAUDE.md のサブエージェント記述の変更（既に記載・採用済み）
- node/uvx/docker のインストールおよびローカル起動型 MCP（Playwright・Serena 等）の導入
- Context7 APIキーの設定（無料枠で運用。必要時のみ後日追加）
- プロジェクトスコープ `.mcp.json` への追加（ユーザースコープに統一）
