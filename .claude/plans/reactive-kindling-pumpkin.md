# issue #28 対応: クロスリポジトリ issue 操作を GitHub 公式 MCP サーバー経由へ切替

## Context

dotclaude-ops からのお知らせ issue #28（詳細: dotclaude-ops#27）を受け、findsummits の
クロスリポジトリ issue 操作（claude-container・dotclaude-ops 宛の起票・コメント・クローズ）を
gh CLI＋セカンダリトークン方式から GitHub 公式 MCP サーバー経由へ切り替える。

動機: 現行方式は全コマンドで `GH_TOKEN=$(cat <トークンファイル>)` の明示指定が必要で、
指定忘れによる誤判定事故が実際に発生している（2026-07-12 handover: 切り替え忘れにより
「セカンダリトークンが機能していない」と誤判定し、ユーザーに不要な手動起票をさせた）。
MCP 経由なら認証がサーバー定義に固定され、この失敗クラスが構造的に消える。

設計は dotclaude-ops#27 のレシピと claude-container README「利用側プロジェクトの設定」節
（SECRETS_DIR・GitHub 公式 MCP レシピ）に従う。新規の設計判断はない。

## 決定事項（ユーザー確認済み）

- **トークン配置はコピー方式**: 既存の `~/.config/claude-container/issues-all-gh-token`（ホスト側）は
  そのまま残し、`~/.config/claude-container/secrets.d/findsummits/GITHUB_MCP_PAT` へ値をコピーする
  （他プロジェクトへの影響ゼロ。ローテーション時は2ファイル更新が必要な点は許容）。
  `GH_TOKEN_SECONDARY_FILE` の配線は現状維持（hooks 用・フォールバック用に残す）
- **MCP の URL はツールセット限定版** `https://api.githubcopilot.com/mcp/x/issues`
  （fine-grained PAT はスコープによる自動ツールフィルタ対象外のため、限定しないと権限外ツールが露出する。
  dotclaude-ops#27 と同一）
- **自リポジトリ（findsummits）の issue 操作は従来どおり gh CLI（プライマリトークン）**。
  MCP はクロスリポジトリ（PAT スコープ内: claude-container・dotclaude-ops）専用。
  findsummits が PAT スコープに含まれるかは public リポジトリのため API では判定不能（README 既知制約）

## タスク（すべて Sonnet・機械的作業）

### 1. リポジトリ内ファイルの編集（コミット対象）

- `.mcp.json`: `github` サーバー定義を追加

  ```json
  {
    "mcpServers": {
      "github": {
        "type": "http",
        "url": "https://api.githubcopilot.com/mcp/x/issues",
        "headers": {
          "Authorization": "Bearer ${GITHUB_MCP_PAT}"
        }
      }
    }
  }
  ```

- `.claude/settings.json`: `"enabledMcpjsonServers": ["github"]` を追加（承認プロンプトの機構的事前承認）
- `CLAUDE.md`「環境課題の連携」節: クロスリポジトリ issue 操作は github MCP サーバー経由を第一とし、
  gh CLI＋セカンダリトークンはフォールバック（hooks はシェルのため従来どおり gh CLI）である旨を追記
- `.claude/commands/claude-container-issue.md` 注記節: 同様に MCP 優先・gh CLI フォールバックへ更新
- `make lint` 警告ゼロ確認 → Conventional Commits でコミット

### 2. ローカル設定の編集（gitignore 対象・コミットなし）

- `.claude-container.d/allowed-domains.txt`: `api.githubcopilot.com` を追加（コメント付き）
- `.claude-container.d/env`: `SECRETS_DIR=~/.config/claude-container/secrets.d/findsummits` を追加
  （`GH_TOKEN_SECONDARY_FILE` 行は現状維持）

### 3. ホスト側作業（ユーザーに依頼・手順を提示する）

```bash
mkdir -p ~/.config/claude-container/secrets.d/findsummits
chmod 700 ~/.config/claude-container/secrets.d/findsummits
cp ~/.config/claude-container/issues-all-gh-token \
   ~/.config/claude-container/secrets.d/findsummits/GITHUB_MCP_PAT
chmod 600 ~/.config/claude-container/secrets.d/findsummits/GITHUB_MCP_PAT
```

その後 `-b` 付きでコンテナを再ビルド（`allowed-domains.txt` はビルド時焼き込みのため必須。
`env` の変更はランタイム設定のため再ビルド不要だが、再ビルドに再起動が含まれるため同時に反映される）。

### 4. 検証（再ビルド後の新セッションで実施）

MCP サーバー定義は既起動セッションに反映されないため（設定反映は再起動単位）、新セッションで:

1. `GITHUB_MCP_PAT` の export 確認 — **値は出力しない**。存在のみ確認
   （例: `sh -c 'test -n "$GITHUB_MCP_PAT" && echo OK'`）
2. MCP サーバー接続確認: ToolSearch で `mcp__github` 系ツールが見えること
3. 読み取りテスト: MCP 経由で dotclaude-ops#27 または claude-container の issue を読む
4. 書き込みテスト: MCP 経由で dotclaude-ops#27 へ findsummits 側の切替完了報告コメントを投稿
   （対外投稿のため**投稿直前にユーザーへ文面を提示して承認を得る**。署名はグローバルルールどおり
   `— <モデル名> (findsummits)`）
5. hooks 経路の非退行確認: session-start.sh の issue 一覧自動注入が従来どおり動くこと
   （新セッション冒頭の注入で確認できる）
6. issue #28 へ対応完了コメント（自リポジトリのため gh CLI プライマリトークンで投稿）。
   クローズは課題管理ルールどおりユーザーの動作確認後

### 5. 後始末

- 検証完了後、本計画ファイルを `git rm` で削除しコミット

## フェイルセーフ

- MCP サーバー接続失敗は fail-soft（セッション自体は動作継続、gh CLI 経路は無傷で残る）
- `SECRETS_DIR` 未配線の環境では `${GITHUB_MCP_PAT}` が未定義になり github サーバーの認証が
  失敗するだけで、他機能に影響しない

## スコープ外

- グローバル `~/.claude/CLAUDE.md` の記述更新（dotclaude-ops 管轄。dotclaude-ops#27 側で追従される）
- hooks の gh CLI → MCP 化（シェルから MCP は呼べないため対象外。README にも明記あり）
- `/spec-panel` セルフレビュー: 本計画は `docs/` 配下を含まないため対象外（ルールどおりスキップ）
