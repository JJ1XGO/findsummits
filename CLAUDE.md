# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## プロジェクト概要

国土地理院の標高タイル（レベル15）を使ってSOTA日本支部の対象サミットを最新化する為の支援ツール。
最新化するに当たって必要なO/Pは、下記3点。
・SOTA日本支部に更新申請をするに当たって申請用ファイル（excel。新規・変更・削除）があるので、それに値を入れたexcelファイル。最重要。
・対象サミットの一覧（座標と標高、そのサミットのKeyコル座標と標高。対象サミットとそのKeyコルの標高がわかればプロミネンスがわかるが一応データとしてはプロミネンスも欲しい。csvで可）。申請書の内容を証明するためのエビデンスとして。
・国土地理院地図上で目視確認するためのGEOJSONファイル。申請書の内容を証明するためのエビデンスとして。
当然、現行の対象サミットに変更・削除があるケースが考えられるので、それらを考慮して申請先の人が見てわかる形のO/Pにする必要がある。

解析に使う基本データはdem5aをベースとし、dem5aがない/もしくは有効値がない座標はdem5b(の該当座標)から、dem5bがない/もしくは有効値がない座標はdem5c(の該当座標)から、dem5cがない/もしくは有効値がない座標はdem10b(の該当座標)から標高値を取得する。

## ビルド・テスト

```bash
make                    # build/findsummits をビルド
make test_mesh_analyze  # build/test_mesh_analyze をビルド
make test_analyze       # build/test_analyze をビルド
make clean              # build/ ディレクトリごと削除

./build/findsummits 4929                        # 1次メッシュコード指定で実行
./build/findsummits params/mesh_list_japan.txt  # メッシュリストファイル指定で実行
./build/test_mesh_analyze                       # テスト実行（引数なし）
```

依存: `libpng`, `libm`, `pthread`（GCC / C99）

## アーキテクチャ

### データフロー

```
（事前準備）
prefetch_tiles.py でタイルを $DATA_DIR/tiles/ へ取得済み
  ↓
入力: 1次メッシュコード（例: 4929）またはメッシュリストファイル
  ↓
mesh_analyze() [mesh_analyze.c]
  ├─ 中心メッシュ + 隣接メッシュ（最大3×3）の結合範囲を計算 [mesh.c]
  ├─ elev_load_with_overlap_8dir_with_dem10() でキャッシュ済みタイルを読み込み [elevation.c]
  │   （未キャッシュ時はエラー終了）
  └─ 全タイルを1枚の大画像に結合
  ↓
Union-Find アルゴリズムで山頂・コルを検出 [unionfind.c]
  ↓
比高 >= 130m でフィルタ（最終 150m 判定は merge.py で実施）
  ↓
出力: $DATA_DIR/results/csv/<meshcode>.csv
```

### 主要モジュール

| モジュール | 役割 |
|---|---|
| `mesh.c/h` | 1次メッシュコード ↔ タイル座標変換（ズーム15 Web Mercator）、隣接メッシュ計算・MeshSet |
| `elevation.c/h` | PNG タイルデコード（RGB→標高）、8方向オーバーラップ対応、キャッシュ参照のみ |
| `unionfind.c/h` | Union-Find（経路圧縮・rank による union）で山頂グループ管理 |
| `analyze.c/h` | タイル単体の局所最大点検出・比高計算、`col_margin_px` 算出 |
| `mesh_analyze.c/h` | メッシュ全体のオーケストレーション・CSV 出力 |
| `scripts/prefetch_tiles.py` | タイル事前取得（If-Modified-Since 条件付き GET・並列4・429/503 backoff） |

### 重要な実装詳細

- **標高デコード**: `elev = (R*65536 + G*256 + B) / 100.0` 、無効値 `(R=128,G=0,B=0)` → `-9999.0f`
- **境界処理**: メッシュ外周に 0m（海面）の 1px ボーダーを追加して、メッシュ端を海岸線とみなす
- **タイルサイズ**: 実画像は 256×256 px だが、隣接タイルとの境界を正確に処理するため 257×257 px（1px オーバーラップ）で管理
- **DEM フォールバック**: DEM5 が存在しないタイルは DEM10 で代替
- **無効標高のセンチネル**: `-9999.0f`

### アーキテクチャ方針（ハイブリッド構成）

- **C エンジン** (`src/`): 標高デコード・Union-Find による山頂/コル検出・per-mesh CSV 出力（タイル取得は行わない）
- **Python スクリプト** (`scripts/`): タイル事前取得（prefetch_tiles.py）、複数 CSV の統合、SOTA リスト突合、XLSX/GeoJSON 生成

C に XLSX/GeoJSON ライブラリを持ち込むコストが高く、`findsummits4sotaja`（Python）に出力生成コードが既存するため、この分担を採用。性能が必要な計算は C、申請用出力は Python。

### ディレクトリ構成

```
src/          # ソースファイル（main.c, *.c, *.h）
scripts/      # Python スクリプト（タイル事前取得・CSV統合・SOTA突合・XLSX/GeoJSON出力）
  prefetch_tiles.py             # タイル事前取得（params/fetch_config.ini を参照）
  merge.py                      # 複数CSV統合・SOTA突合
  analyze_keycol_distance.py    # Keyコル距離分析
params/       # パラメータファイル
  mesh_list_japan.txt           # 解析対象メッシュコードリスト
  fetch_config.ini.example      # UA・並列数設定のテンプレート（コミット済み）
  fetch_config.ini              # 実設定（gitignore・メールアドレス記入）
ref/          # 参照データ（git管理）
  summitslist.csv                          # SOTAの山岳リスト（全サミット）
  SOTA-Summit-list-revision-request.xlsx   # SOTA日本支部への申請書テンプレート
tests/        # テスト用プログラム（test_*.c）
.claude/mngt/  # 管理ドキュメント（todo.md, lessons.md, plan.md）
.env.example  # DATA_DIR 設定例（コミット済み）
.env          # 実設定（gitignore）

# 以下のパスは .env の DATA_DIR で設定する（デフォルト: /mnt/findsummits）
$DATA_DIR/images/       # 処理範囲を目視確認するためのイメージファイル置き場
$DATA_DIR/results/      # 最終O/Pのxlsx,geojson,csv
$DATA_DIR/results/csv/  # 一時csv（findsummits が出力するper-mesh CSV）
$DATA_DIR/tiles/        # ダウンロード済みタイルのキャッシュ
  └─ {z}/     # タイルのURLの命名規則と同様
     └─ {x}   # タイルのURLの命名規則と同様
         └─ {y}
$DATA_DIR/logs/         # findsummits・prefetch_tiles のログ
```

## その他
他のプロジェクトの参考コードは以下の場所にあります：
@../findsummits4sotaja/ # 以前、pythonで開発した時のプロジェクト。九州・四国を解析してSOTA日本支部に申請した時のもの。

### SOTA関連資料
- SOTAの山岳リスト(JAで始まるものが日本支部のサミット)
  https://www.sotadata.org.uk/summitslist.csv
- SOTA日本支部への山岳リスト更新申請書
  https://www.kawauchi.homeip.mydns.jp/sotajp/wp-content/uploads/2024/03/SOTA-Summit-list-revision-request.xlsx
