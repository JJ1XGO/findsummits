# 幽霊 clangd-lsp プラグイン登録の削除

## Context（背景）

前回セッションで「環境がおかしい／挙動が不安定」の原因として、
(1) `~/.claude/commands/` に混入した外部オーケストレーター7コマンド、
(2) plugin の install パスが旧ホスト `/home/tsu` を指したまま、の2点が挙がっていた。

今回の調査で **(1)(2) はいずれも解消済み**であることを確認した：
- 外部7コマンドは `commands_disabled/` へ退避されたまま、`commands/` は正規5個のみ
- `installed_plugins.json` の installPath は現行 `/home/node` を指しており、`/home/tsu` 残存は過去ログ・バックアップのみ（実害なし）

残る唯一の不具合は **clangd-lsp プラグインの「幽霊状態」**：
- `installed_plugins.json` に `clangd-lsp@1.0.0` がインストール済みと記録
- 実体 `cache/claude-plugins-official/clangd-lsp/1.0.0/` は**空**
- `clangd` バイナリもシステムに存在しない

この記録だけ残った状態が起動時のプラグイン不具合の原因。`clangd-lsp` は C 補完用 LSP で、
本プロジェクトのビルド・解析（`make` / `findsummits`）には**不要**。ユーザー判断により削除して無効化する。

## 変更対象

- `/home/node/.claude/plugins/installed_plugins.json`

## 手順

1. 念のためバックアップ：`cp installed_plugins.json installed_plugins.json.bak`
2. `installed_plugins.json` を編集し、`plugins` から `clangd-lsp@claude-plugins-official` エントリを削除：
   ```json
   {
     "version": 2,
     "plugins": {}
   }
   ```
3. （任意）空になった cache ディレクトリ `cache/claude-plugins-official/clangd-lsp/` を削除
4. メモリ `env_foreign_orchestrator_commands.md` の「未対応の別件 (1)」を解決済みに更新

## 検証

- `installed_plugins.json` を再読込し、`plugins` が空であること（`clangd-lsp` 記録なし）を確認
- 反映には**セッション再起動が必要**。再起動後、起動時に clangd-lsp 関連のプラグインエラーが
  出ないことをユーザーに確認してもらう

## 注意

- marketplace 登録（`known_marketplaces.json`）はそのまま残す（無害・将来の再導入用）
- `commands_disabled/` の7コマンドはそのまま退避維持（今回は触らない）
