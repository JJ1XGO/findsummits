# 環境定義

| 項目 | 内容 |
|---|---|
| 作成日 | 2026-04-30 |
| 最終更新日 | 2026-06-25 |
| ステータス | ドラフト |

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
コンテナ実装: <https://github.com/JJ1XGO/claude-container>

| 項目 | 値 |
|---|---|
| OS | Debian GNU/Linux trixie (x86_64、コンテナ・リビルドで最新化) |
| CPU | AMD Ryzen 7 2700（16スレッド）@ 3.20 GHz |
| メモリ | 62.72 GiB |
| データディスク (`/data`) | ホストの `/mnt/findsummits` をマウント（457.38 GiB ext4） |
| Shell | bash 5.3.9 |

## C 解析エンジン ビルド依存（`src/` のビルドに必要）

| 依存 | 用途 |
|---|---|
| GCC（C99 準拠） | コンパイラ |
| libpng | PNG タイルデコード |
| libm | 数学関数 |
| pthread | マルチスレッド処理 |

## Python 環境セットアップ

Python スクリプトの実行には venv を使用する。venv は `/workspace/venv/` に作成されるため、
ホストの `/mnt/findsummits` にマウントされたまま永続化され、コンテナをリビルドしても消えない。

```bash
make venv          # venv 作成 + 依存パッケージインストール（初回 or requirements.txt 変更時）
```

以降は venv を activate せず、`venv/bin/python3` で直接呼び出す:

```bash
venv/bin/python3 scripts/prefetch_tiles.py ...
venv/bin/python3 scripts/merge.py ...
venv/bin/python3 scripts/preprocess_pref_boundaries.py ...
venv/bin/python3 mgmt/tracker/track.py ...
```

## Python パッケージ（`requirements.txt` で管理）

**ランタイム依存**（本番パイプラインで import するもの）:

| パッケージ | 用途 |
|---|---|
| requests | タイル取得（prefetch_tiles.py） |
| openpyxl | XLSX 出力 |
| shapely | 都道府県/振興局 point-in-polygon 判定（merge.py, preprocess_pref_boundaries.py） |
| numpy | Keyコル距離分析（analysis/analyze_keycol_distance.py） |
| pillow | PNG タイルデコード（analysis/ スクリプト群） |

**開発ツール**（ランタイムで import しない、バージョン固定）:

| パッケージ | 用途 |
|---|---|
| pymarkdownlnt | Markdown lint（`make lint` → `lint-md`） |
| ruff | Python 静的解析（`make lint` → `lint-py`） |
| geojson-validator | GeoJSON 構造・ジオメトリ検証（`make lint` → `lint-geojson`） |
| djlint | HTML 構文チェック（`make lint` → `lint-html`） |
