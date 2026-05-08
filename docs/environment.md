# 環境定義

> 最終更新: 2026-04-30

## ホスト環境

| 項目 | 値 |
|---|---|
| OS | Debian GNU/Linux testing (x86_64、最新化運用) |
| CPU | AMD Ryzen 7 2700（16スレッド）@ 3.20 GHz |
| メモリ | 62.72 GiB |
| データディスク (`/mnt/findsummits`) | 457.38 GiB ext4 |
| Shell | bash 5.3.9 |

## コンテナ環境

ホスト環境マシン上のコンテナ（リソース制限なし）。コンテナリビルドによりパッケージ類は常に最新化される。  
コンテナ実装: https://github.com/JJ1XGO/claude-container

| 項目 | 値 |
|---|---|
| OS | Debian GNU/Linux trixie (x86_64、コンテナ・リビルドで最新化) |
| CPU | AMD Ryzen 7 2700（16スレッド）@ 3.20 GHz |
| メモリ | 62.72 GiB |
| データディスク (`/data`) | ホストの `/mnt/findsummits` をマウント（457.38 GiB ext4） |
| Shell | bash 5.3.9 |
