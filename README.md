# findsummits

国土地理院の標高タイルを解析してSOTA日本支部の対象サミットを最新化する支援ツール

## 概要

- 国土地理院DEM5標高タイル(ズームレベル15)を使用
- Union-Findアルゴリズムでプロミネンス150m以上の山頂を検出
- SOTA日本支部サミットリストと照合して未登録候補を抽出

## 使用する標高データについて

国土地理院の標高タイルは DEM1a（1m メッシュ）も公開されているが、本ツールでは **DEM5a/b/c（5m メッシュ）→ DEM10b（10m メッシュ）のフォールバック構成** を採用している。

理由：SOTA のプロミネンス判定基準は 150m であり、5m 解像度で十分な精度が得られる。DEM1a を使用した場合、データ量・処理時間が約 25 倍になるがプロミネンス計算の精度向上はほぼない。

## 必要環境

- GCC（C99）
- libpng, libm, pthread
- Python 3.13 + python3-venv パッケージ

## セットアップ

```bash
# Python 仮想環境の作成（初回・requirements.txt 変更時）
make venv

# C エンジンのビルド
make
```

## 使い方

### 1. タイルの事前取得

```bash
# params/fetch_config.ini を用意してから実行
venv/bin/python3 scripts/prefetch_tiles.py \
  --mesh-list params/mesh_list_japan.txt \
  --config params/fetch_config.ini \
  --tile-dir /path/to/data/tiles
```

### 2. サミット候補の検出

```bash
./build/findsummits 4929                        # 1次メッシュコード指定
./build/findsummits params/mesh_list_japan.txt  # メッシュリスト指定
```

### 3. 結果の統合・出力

```bash
venv/bin/python3 scripts/merge.py ...
```

## データディレクトリ設定

`params/config.ini`（`params/config.ini.example` をコピーして作成）に `DATA_DIR` を記入する。

## データソース・出典

本ツールは [国土地理院](https://www.gsi.go.jp/) が提供する [地理院タイル](https://maps.gsi.go.jp/development/ichiran.html)（標高タイル DEM5a / DEM5b / DEM5c / DEM10b）を加工して作成しています。

- 出典: 国土地理院ウェブサイト (https://maps.gsi.go.jp/development/ichiran.html)
- 利用規約: [国土地理院コンテンツ利用規約](https://www.gsi.go.jp/kikakuchousei/kikakuchousei40182.html)

本ツールが生成する GeoJSON / CSV / XLSX には地理院タイルの標高値から解析した派生データが含まれます。再配布時も上記出典の明示をお願いします。

## ライセンス

GPL-3.0
