# 計画: エントリポイントスクリプトによるプラグイン設定パス自動修正

## Context

ホスト側のユーザー名（例: `tsu`）とコンテナ内ユーザー名（`node`）が異なるため、
`~/.claude/plugins/installed_plugins.json` 等に記録されたパスが `/home/tsu/...` のまま
コンテナ内で読まれ、`/home/node/...` と不一致になる。

Claude Code を毎回 `--rm` で起動するため、「コンテナ内で再インストール」しても
根本解決にならない。代わりに起動時にパスを自動修正するエントリポイントスクリプトを追加する。

## 変更ファイル

### 1. `entrypoint.sh`（新規作成）

```bash
#!/bin/bash
# ~/.claude/plugins/ 内の設定ファイルに残存するホスト側パスをコンテナ内パスに修正
for f in \
  /home/node/.claude/plugins/installed_plugins.json \
  /home/node/.claude/plugins/known_marketplaces.json; do
  [ -f "$f" ] && sed -i "s|/home/[^/]*/\.claude|/home/node/.claude|g" "$f"
done
exec claude --dangerously-skip-permissions
```

- `/home/<任意>/\.claude` → `/home/node/.claude` に置換（ユーザー名が何であっても対応）
- `exec` で claude プロセスを置換（シグナル処理が正しく行われる）
- ファイルが存在しない場合はスキップ（`[ -f "$f" ]`）

### 2. `Dockerfile.claude`（修正）

CACHEBUST ARG の直前に以下を追加：

```dockerfile
COPY --chown=node:node entrypoint.sh /home/node/entrypoint.sh
RUN chmod +x /home/node/entrypoint.sh
```

最終行を変更：

```dockerfile
# 変更前
CMD ["claude", "--dangerously-skip-permissions"]
# 変更後
CMD ["/home/node/entrypoint.sh"]
```

### 3. `CLAUDE.md`（修正）

Architecture セクションの「Three files work together:」を「Four files work together:」に変更し、
`entrypoint.sh` の説明を追加：

```
- **`entrypoint.sh`** — コンテナ起動時に実行されるシェルスクリプト。
  `~/.claude/plugins/` 内の設定ファイルに残存するホスト側ユーザーのパス
  （例: `/home/tsu/.claude`）をコンテナ内パス（`/home/node/.claude`）に自動修正してから
  `claude --dangerously-skip-permissions` を起動する。
  ホスト側とコンテナ内でユーザー名が異なる環境でのプラグインパス不整合を吸収するための仕組み。
```

## 検証手順

```bash
# 1. シンタックスチェック
bash -n entrypoint.sh

# 2. Compose ファイル検証
podman compose -f compose.yml config

# 3. -b でリビルド後に起動し、プラグイン設定のパスを確認
./claude-container -b /path/to/project
# コンテナ内で確認:
# cat ~/.claude/plugins/installed_plugins.json | grep home
```
