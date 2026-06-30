---
paths:
  - "src/**"
  - "tests/**"
  - "scripts/**"
  - "analysis/**"
  - "**/*.c"
  - "**/*.h"
  - "**/*.py"
---

# アーキテクチャ

## データフロー

```text
（事前準備）
prefetch_tiles.py でタイルを $DATA_DIR/tiles/ へ取得済み
  ↓
入力: 1次メッシュコード（例: 4929）またはメッシュリストファイル
  ↓
mesh_analyze() [mesh_analyze.c]
  ├─ 中心メッシュ + 隣接メッシュ（最大3×3）の結合範囲を計算 [mesh.c]
  ├─ ローカルキャッシュ済みタイルを読み込み [elevation.c]
  │   （ローカルキャッシュ未取得時はエラー終了）
  └─ 全タイルを1枚の大画像に結合
  ↓
標高地形図 PNG 出力 [mesh_analyze.c]
  └─ $DATA_DIR/images/<meshcode>_terrain.png（長辺6000px縮小）
  ↓
Union-Find アルゴリズムでピーク・コルを検出 [unionfind.c]
  ↓
比高 >= 130m でフィルタ（最終 150m 判定は merge.py で実施）
  ↓
出力: $DATA_DIR/results/csv/<meshcode>.csv
```

## 主要モジュール

| モジュール | 役割 |
|---|---|
| `mesh.c/h` | 1次メッシュコード ↔ タイル座標変換（ズーム15 Web Mercator）、隣接メッシュ計算・MeshSet |
| `elevation.c/h` | PNG タイルデコード（RGB→標高）、8方向オーバーラップ対応、ローカルキャッシュ参照のみ |
| `unionfind.c/h` | Union-Find（経路圧縮・rank による union）でピークグループ管理 |
| `analyze.c/h` | タイル単体のピーク候補検出・比高計算、`col_margin_px` 算出 |
| `mesh_analyze.c/h` | メッシュ全体のオーケストレーション・標高地形図 PNG 出力・CSV 出力 |
| `scripts/prefetch_tiles.py` | タイル事前取得（If-Modified-Since 条件付き GET・並列4・429/503 backoff） |
| `scripts/preprocess_pref_boundaries.py` | N03 行政区域 GeoJSON を都道府県/振興局レベルに dissolve して軽量化（初回のみ実行） |

## 重要な実装詳細

- **標高デコード**: `elev = (R*65536 + G*256 + B) / 100.0` 、無効値 `(R=128,G=0,B=0)` → `-9999.0f`
- **境界処理**: メッシュ外周に 0m（海面）の 1px ボーダーを追加して、メッシュ端を海岸線とみなす
- **タイルサイズ**: 実画像は 256×256 px だが、隣接タイルとの境界を正確に処理するため 257×257 px（1px オーバーラップ）で管理
- **DEM フォールバック**: DEM5 が存在しないタイルは DEM10 で代替
- **無効標高のセンチネル**: `-9999.0f`
- **標高地形図 PNG**: `findsummits` 実行時に解析範囲を人間が視認しやすい配色で標高を色分けした PNG として出力。長辺 6000px に縮小して保存

## アーキテクチャ方針（ハイブリッド構成）

- **C エンジン** (`src/`): 標高デコード・Union-Find によるピーク/コル検出・標高地形図 PNG 出力・per-mesh CSV 出力（タイル取得は行わない）
- **Python スクリプト** (`scripts/`): タイル事前取得（prefetch_tiles.py）、N03 行政区域前処理（preprocess_pref_boundaries.py・初回のみ）、複数 CSV の統合、SOTA リスト突合、XLSX/GeoJSON 生成

C に XLSX/GeoJSON ライブラリを持ち込むコストが高く、`findsummits4sotaja`（Python）に出力生成コードが既存するため、この分担を採用。性能が必要な計算は C、申請用出力は Python。

## ディレクトリ構成

```text
src/          # ソースファイル（main.c, *.c, *.h）
scripts/      # 本番パイプライン用スクリプト
  prefetch_tiles.py             # タイル事前取得（params/fetch_config.ini を参照）
  preprocess_pref_boundaries.py # N03行政区域前処理（初回のみ: $DATA_DIR/ref/N03-2026_regions.geojson 生成）
  merge.py                      # 複数CSV統合・SOTA突合・都道府県ベース仮コード割り振り
  run_all.sh                    # 全メッシュ一括解析ラッパー
analysis/     # 検証・解析用スクリプト（本番パイプライン外）
  analyze_keycol_distance.py    # Keyコル距離分析
params/       # パラメータファイル
  mesh_list_japan.txt           # 解析対象メッシュコードリスト
  config.ini.example            # DATA_DIR 設定テンプレート（コミット済み）
  config.ini                    # 実設定（gitignore）: DATA_DIR を記入
  fetch_config.ini.example      # UA・並列数設定のテンプレート（コミット済み）
  fetch_config.ini              # 実設定（gitignore・メールアドレス記入）
docs/         # 設計ドキュメント（git管理・devel/main 両ブランチ）
  00_GLOSSARY.md                           # 用語集
  01_environment.md                        # 環境定義
  10_URD.md                                # ユーザー要件定義書
  20_SRS.md                                # ソフトウェア要件仕様書
  decisions/                               # アーキテクチャ決定記録（ADR）
    research/                              # ADR 決定前の設計調査資料
ref/          # 参照データ（git管理）
  SOTA-Summit-list-revision-request.xlsx   # SOTA日本支部への申請書テンプレート
  SOURCES.md                               # 参照資料の出典一覧
tests/        # テスト用プログラム（test_*.c）
mgmt/          # 管理ドキュメント（lessons.md, plan.md, tracker/）※ devel ブランチのみ・main には含めない
.claude-container  # 実設定（gitignore）: EXTRA_MOUNT でホストの /mnt/findsummits をコンテナ内にマウント

# 以下のパスは params/config.ini の DATA_DIR で設定する
# - claude-container 使用時: DATA_DIR = /data（コンテナ内パス）
# - 非コンテナ時: DATA_DIR = /path/to/your/data（ホストのデータパス）
$DATA_DIR/images/       # 標高地形図 PNG（findsummits が自動出力: <meshcode>_terrain.png）
$DATA_DIR/results/      # 最終O/Pのxlsx,geojson,csv
$DATA_DIR/results/csv/  # 一時csv（findsummits が出力するper-mesh CSV）
$DATA_DIR/tiles/        # ダウンロード済みタイルのローカルキャッシュ
  └─ {サービス名}/  # dem5a_png / dem5b_png / dem5c_png / dem_png（タイル URL の命名規則と同様）
     └─ {z}/
        └─ {x}
           └─ {y}.png
$DATA_DIR/logs/         # findsummits・prefetch_tiles のログ
$DATA_DIR/ref/          # ユーザー手動配置ファイルおよびシステム生成参照データ（git 管理外）
  summitslist.csv           # SOTA サミットリスト（ユーザー手動ダウンロード・配置）
  geojson_v{N}/             # SOTA 既存サミット GeoJSON（ユーザー手動ダウンロード・配置）
  N03-2026_regions.geojson  # N03 行政区域前処理済み（preprocess_pref_boundaries.py が生成）
```

## Python スクリプトの実行

**Python スクリプトは必ず `venv/bin/python3` で呼び出すこと（`python3` は不可）。**
`python3` はコンテナのシステム Python であり、パッケージが入っていない。

venv が存在しない場合は先に `make venv` を実行する:

```bash
make venv                                                          # 初回セットアップ・requirements.txt 変更時
make venv-rebuild                                                  # venv をクリーン再構築（孤立パッケージ除去・定期/整合確認用）
venv/bin/python3 scripts/prefetch_tiles.py ...                    # タイル取得
venv/bin/python3 scripts/merge.py ...                             # CSV 統合・出力
venv/bin/python3 scripts/preprocess_pref_boundaries.py ...        # 都道府県境界前処理
venv/bin/python3 mgmt/tracker/track.py issue list                 # 課題管理
```

venv は `/workspace/venv/`（ホストマウント下）に作られるためコンテナリビルド後も消えない。

## パッケージ依存の整合ルール（ズレ防止）

`requirements.txt` を唯一の正とし、venv と常に一致させる:

- **新しいサードパーティ製パッケージを import したら、同じコミット内で `requirements.txt` に追記する。** `pip install` だけで済ませない（venv にあるが requirements.txt に無い、というズレを防ぐ）
- `requirements.txt` からパッケージを外したら `make venv-rebuild` で venv をクリーン再構築し、孤立パッケージを除去する
- 定期的に `make venv-rebuild` を実行して venv == requirements.txt を担保する
