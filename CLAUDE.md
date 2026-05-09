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
./build/test_mesh_analyze 4929                  # テスト（イメージ出力なし）
./build/test_mesh_analyze 4929 --save-image     # テスト（標高地形図 PNG も出力）
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
  ├─ キャッシュ済みタイルを読み込み [elevation.c]
  │   （未キャッシュ時はエラー終了）
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

### 主要モジュール

| モジュール | 役割 |
|---|---|
| `mesh.c/h` | 1次メッシュコード ↔ タイル座標変換（ズーム15 Web Mercator）、隣接メッシュ計算・MeshSet |
| `elevation.c/h` | PNG タイルデコード（RGB→標高）、8方向オーバーラップ対応、キャッシュ参照のみ |
| `unionfind.c/h` | Union-Find（経路圧縮・rank による union）でピークグループ管理 |
| `analyze.c/h` | タイル単体のピーク候補検出・比高計算、`col_margin_px` 算出 |
| `mesh_analyze.c/h` | メッシュ全体のオーケストレーション・標高地形図 PNG 出力・CSV 出力 |
| `scripts/prefetch_tiles.py` | タイル事前取得（If-Modified-Since 条件付き GET・並列4・429/503 backoff） |
| `scripts/preprocess_pref_boundaries.py` | N03 行政区域 GeoJSON を都道府県/振興局レベルに dissolve して軽量化（初回のみ実行） |

### 重要な実装詳細

- **標高デコード**: `elev = (R*65536 + G*256 + B) / 100.0` 、無効値 `(R=128,G=0,B=0)` → `-9999.0f`
- **境界処理**: メッシュ外周に 0m（海面）の 1px ボーダーを追加して、メッシュ端を海岸線とみなす
- **タイルサイズ**: 実画像は 256×256 px だが、隣接タイルとの境界を正確に処理するため 257×257 px（1px オーバーラップ）で管理
- **DEM フォールバック**: DEM5 が存在しないタイルは DEM10 で代替
- **無効標高のセンチネル**: `-9999.0f`
- **標高地形図 PNG**: `findsummits` 実行時に解析範囲を人間が視認しやすい配色で標高を色分けした PNG として出力。長辺 6000px に縮小して保存

### アーキテクチャ方針（ハイブリッド構成）

- **C エンジン** (`src/`): 標高デコード・Union-Find によるピーク/コル検出・標高地形図 PNG 出力・per-mesh CSV 出力（タイル取得は行わない）
- **Python スクリプト** (`scripts/`): タイル事前取得（prefetch_tiles.py）、N03 行政区域前処理（preprocess_pref_boundaries.py・初回のみ）、複数 CSV の統合、SOTA リスト突合、XLSX/GeoJSON 生成

C に XLSX/GeoJSON ライブラリを持ち込むコストが高く、`findsummits4sotaja`（Python）に出力生成コードが既存するため、この分担を採用。性能が必要な計算は C、申請用出力は Python。

### ディレクトリ構成

```
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
  01_URD.md                                # ユーザー要件定義書
  02_SRS.md                                # ソフトウェア要件仕様書
  environment.md                           # 環境定義
  decisions/                               # アーキテクチャ決定記録（ADR）
    research/                              # ADR 決定前の設計調査資料
ref/          # 参照データ（git管理）
  summitslist.csv                          # SOTAの山岳リスト（全サミット）
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
$DATA_DIR/tiles/        # ダウンロード済みタイルのキャッシュ
  └─ {z}/     # タイルのURLの命名規則と同様
     └─ {x}   # タイルのURLの命名規則と同様
         └─ {y}
$DATA_DIR/logs/         # findsummits・prefetch_tiles のログ
```

## 開発ドキュメント管理

### 各ステージの成果物

ウォーターフォール方式（URD→SRS→HLD→LLD→COD→UT→IT→ST→OPS）で開発を進める。各ステージの成果物を `docs/` に作成する。

| ステージ | 成果物ファイル |
|---------|--------------|
| URD | `docs/01_URD.md` |
| SRS | `docs/02_SRS.md` |
| HLD | `docs/03_HLD.md` |
| LLD | `docs/04_LLD.md` |
| COD | `src/*.c`, `scripts/*.py` |
| UT  | `docs/05_UT.md` |
| IT  | `docs/06_IT.md` |
| ST  | `docs/07_ST.md` |
| OPS | `docs/08_OPS.md` |

常駐ドキュメント（ステージ不問）:
- `docs/00_GLOSSARY.md` — 用語集（全文書から参照）
- `docs/environment.md` — 環境定義
- `ref/SOURCES.md` — 参照資料の出典一覧
- `docs/decisions/ADR-*.md` — アーキテクチャ決定記録
- `docs/decisions/research/` — ADR 決定前の設計調査資料

`docs/` は devel・main 両ブランチに含める。`mgmt/` は devel ブランチのみ（リリース時に `git rm -r mgmt/` で除外）。

### GLOSSARY と SOURCES の役割分担

| ファイル | 役割 | 書くこと | 書かないこと |
|---|---|---|---|
| `docs/00_GLOSSARY.md` | 用語の概念定義（What） | 「〇〇とは何か」の説明。必要に応じて仕様リンクや SOURCES へのリンク | 利用規約 URL・帰属表示義務の詳細 |
| `ref/SOURCES.md` | 出典・利用規約の詳細（Where/How） | 取得元 URL・利用規約 URL・帰属表示義務・利用形態 | 概念の説明 |

**更新ルール**:
- 新しいデータソース（地図タイル・参照ファイル等）を追加するときは **両方** を更新する
- 利用規約 URL や帰属表示義務の変更は SOURCES のみ更新すれば良い
- GLOSSARY の説明文に規約詳細を書かず、SOURCES へのリンクで委ねること
- 各セクション冒頭の注釈（`利用規約・出典の詳細は ref/SOURCES.md を参照`）を維持すること

### ドキュメントフォーマット標準

各文書のヘッダーテーブル:

```
| 項目 | 内容 |
|---|---|
| 作成日 | YYYY-MM-DD |
| 最終更新日 | YYYY-MM-DD |
| ステータス | ドラフト / 確定 |
```

- バージョン番号は記載しない（git で管理）
- 本文冒頭に目次（Markdown アンカーリンク）を設ける
- 用語定義は冒頭 blockquote ではなく番号付きセクション「N. 用語定義」として記載する
- 外部資料への参照は本文中にインライン注記のみ（例: `（参照: [SOURCES.md](../ref/SOURCES.md)）`）。別途「参照資料」セクションは作らない

### ADR 管理ルール

**いつ作るか**: アーキテクチャ上の重要な判断（実装方針・技術選択・スコープ決定）をしたとき。

**命名規則**: `docs/decisions/ADR-NNN-kebab-case-description.md`（NNN は3桁連番）

**フォーマット**:

```markdown
| 状態 | 採用・実装済み / 採用・未実装 / 検討中 / 却下 |
| 決定日 | YYYY-MM-DD |

## Context
## Decision
## Alternatives
## Consequences
```

**参照ルール**:
- ADR は自己完結した文書とする
- 参照可能なファイル: `docs/` 配下・`ref/` 配下・`CLAUDE.md`
- **`mgmt/` への参照は禁止**（devel ブランチ専用のため main で参照できない）
- 詳細な調査資料が必要な場合は `docs/decisions/research/` に置き ADR から参照する

**調査資料（research/）**:
- ADR の意思決定に至る検討過程・比較分析・Opus 相談内容等を記録する
- 内容は生の検討資料として整形不要
- 命名: `docs/decisions/research/<説明的な名前>.md`

**URD/SRS との相互参照**:
- URD のスコープ外・制約の根拠が ADR にある場合は URD からリンクを付ける

## 欠陥管理ルール（必須）

**バグ・欠陥を発見しても、すぐに修正を始めてはならない。必ず以下のフローを守ること。**

1. テスト結果を確認し、発見した欠陥を**すべて洗い出してから**まとめて一覧提示する（1件見つけるたびに都度登録しない）
2. ユーザーに確認を取ってから `python3 mgmt/tracker/track.py bug add` で登録する
3. 原因がわかっている場合は `--resolution` に対応方針まで記入してから登録する
4. 登録完了後、修正作業の承認を得てから着手する
5. 修正完了後は `bug close` コマンドでステータスを「対応完了」にする（解決済にしない）
6. ユーザーが確認完了後、`bug verify` コマンドでステータスを「解決済」にする

詳細な運用手順・コマンド一覧は `mgmt/tracker/CLAUDE.md` を参照。

## 課題管理ルール

**開発タスク（機能追加・改善・調査・設計）は `python3 mgmt/tracker/track.py issue` で管理する。**  
バグ（欠陥）は `track.py bug`、開発課題は `track.py issue` と使い分けること。

1. 新規の開発タスクが発生したら `python3 mgmt/tracker/track.py issue add` で登録する
2. 作業開始時は `issue update --status 対応中` でステータスを更新する
3. 実装完了後は `issue close` コマンドでステータスを「対応完了」にする
4. ユーザーが確認完了後、`issue verify` コマンドでステータスを「解決済」にする
5. `stage` フィールドは次ステージ移行の判断材料として活用する

詳細な運用手順・コマンド一覧は `mgmt/tracker/CLAUDE.md` を参照。

## ブランチ運用ルール

- `devel → main` の直接マージ禁止
- リリース時は必ず `release/*` ブランチを介す:
  1. `git checkout -b release/vX.X`
  2. `git rm -r mgmt/`
  3. `git commit -m "chore: リリース用にmgmt/除外"`
  4. `git checkout main && git merge release/vX.X`

## その他
他のプロジェクトの参考コードは以下の場所にあります：
@../findsummits4sotaja/ # 以前、pythonで開発した時のプロジェクト。九州・四国を解析してSOTA日本支部に申請した時のもの。

### SOTA関連資料
- SOTAの山岳リスト(JAで始まるものが日本支部のサミット)
  https://www.sotadata.org.uk/summitslist.csv
- SOTA日本支部への山岳リスト更新申請書
  https://www.kawauchi.homeip.mydns.jp/sotajp/wp-content/uploads/2024/03/SOTA-Summit-list-revision-request.xlsx
