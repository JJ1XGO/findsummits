# findsummits - ソフトウェア要件仕様書 (SRS)

| 項目 | 内容 |
|---|---|
| 作成日 | 2026-04-30 |
| 最終更新日 | 2026-05-08 |
| ステータス | ドラフト（TBD残4件） |
| 参照 URD | [`01_URD.md`](01_URD.md) |

---

## 目次

1. [目的・範囲](#1-目的範囲)
2. [用語定義](#2-用語定義)
3. [システムアーキテクチャ概要](#3-システムアーキテクチャ概要)
4. [機能要件](#4-機能要件)
   - [フェーズ1: タイル取得・標高デコード](#フェーズ1-タイル取得標高デコード)
     - [FR-001: 標高タイル事前取得](#fr-001-標高タイル事前取得)
     - [FR-002: DEM 階層フォールバック](#fr-002-dem-階層フォールバック)
     - [FR-003: 標高デコード・NODATA 処理](#fr-003-標高デコードnodata-処理)
   - [フェーズ2: 山頂・コル検出](#フェーズ2-山頂コル検出)
     - [FR-004: 3×3 メッシュ結合解析](#fr-004-33-メッシュ結合解析)
     - [FR-005: 局所最大点検出](#fr-005-局所最大点検出)
     - [FR-006: Keyコル検出・プロミネンス計算](#fr-006-keyコル検出プロミネンス計算)
     - [FR-007: プロミネンスフィルタ・per-mesh CSV 出力](#fr-007-プロミネンスフィルタper-mesh-csv-出力)
     - [FR-015: 標高地形図出力](#fr-015-標高地形図出力)
   - [フェーズ3: SOTA 突合・差分分類](#フェーズ3-sota-突合差分分類)
     - [FR-008: per-mesh CSV 統合](#fr-008-per-mesh-csv-統合)
     - [FR-009: SOTAリスト突合・match_status 判定](#fr-009-sotaリスト突合match_status-判定)
     - [FR-010: 削除候補のスコープ](#fr-010-削除候補のスコープ)
   - [フェーズ4: 申請用出力生成](#フェーズ4-申請用出力生成)
     - [FR-011: 申請書 XLSX 生成](#fr-011-申請書-xlsx-生成)
     - [FR-012: エビデンス CSV 生成](#fr-012-エビデンス-csv-生成)
     - [FR-013: GeoJSON・HTML ビューア生成](#fr-013-geojsonhtml-ビューア生成)
     - [FR-014: 独立峰対応（レベル14 広域再解析）](#fr-014-独立峰対応レベル14-広域再解析)
5. [非機能要件](#5-非機能要件)
   - [NFR-001: 精度（プロミネンス判定）](#nfr-001-精度プロミネンス判定)
   - [NFR-002: メモリ使用量](#nfr-002-メモリ使用量)
   - [NFR-003: 再現性（決定論的出力）](#nfr-003-再現性決定論的出力)
   - [NFR-004: アクセスマナー](#nfr-004-アクセスマナー)
   - [NFR-005: 処理時間目標](#nfr-005-処理時間目標)
   - [NFR-006: 可搬性・環境](#nfr-006-可搬性環境)
   - [NFR-007: ログ出力](#nfr-007-ログ出力)
6. [外部インターフェース仕様](#6-外部インターフェース仕様)
   - [6.1 入力: 地理院標高タイル](#61-入力-地理院標高タイル)
   - [6.2 入力: SOTA サミットリスト CSV](#62-入力-sota-サミットリスト-csv)
   - [6.3 出力: 申請書 XLSX](#63-出力-申請書-xlsx)
   - [6.4 出力: エビデンス CSV](#64-出力-エビデンス-csv)
   - [6.5 出力: GeoJSON・HTML ビューア](#65-出力-geojsonhtml-ビューア)
   - [6.6 内部インターフェース: per-mesh CSV（C → Python 境界）](#66-内部インターフェース-per-mesh-csvc--python-境界)
   - [6.7 出力: 標高地形図（Terrain-RGB PNG）](#67-出力-標高地形図terrain-rgb-png)
7. [依存関係・環境](#7-依存関係環境)
8. [制約・前提条件](#8-制約前提条件)
9. [スコープ外](#9-スコープ外)
10. [TBD 一覧](#10-tbd-一覧)

---

## 1. 目的・範囲

本文書は [`01_URD.md`](01_URD.md) に定めるユーザー要件（UR-001〜UR-010）を実現するための
ソフトウェア要件を規定する。

**対象システム**: findsummits（C エンジン + Python スクリプト群）  
**対象バージョン**: 1.0（予定）  
**対象外**: 実装詳細（HLD/LLD）、テスト仕様（UAT）

---

## 2. 用語定義

[`00_GLOSSARY.md`](00_GLOSSARY.md) を参照。

---

## 3. システムアーキテクチャ概要

C + Python ハイブリッド構成（ADR-001）。

```
prefetch_tiles.py    タイル事前取得（Python）
       ↓
findsummits (C)      標高デコード・Union-Find 山頂/コル検出
       ↓  per-mesh CSV  ($DATA_DIR/results/csv/<meshcode>.csv)
merge.py (Python)    CSV 統合・SOTA 突合
       ↓  merged.csv   ($DATA_DIR/results/merged.csv)
output_xlsx.py       申請書 XLSX 生成（Python）
output_geojson.py    GeoJSON + 静的 HTML ビューア生成（Python）
```

**C / Python 境界**: per-mesh CSV ファイル。  
詳細は [`decisions/ADR-001-hybrid-c-python-architecture.md`](decisions/ADR-001-hybrid-c-python-architecture.md) を参照。

---

## 4. 機能要件

### フェーズ1: タイル取得・標高デコード

#### FR-001: 標高タイル事前取得

- **対応 UR**: UR-001, UR-010
- 国土地理院の標高タイル（ズームレベル15、256×256px PNG）を `$DATA_DIR/tiles/` に取得・キャッシュする
- DEM 種別ごとに次のディレクトリ階層で保存する: `$DATA_DIR/tiles/{z}/{x}/{y}_{dem}.png`
- `If-Modified-Since` ヘッダーによる条件付き GET を使用し、差分取得に対応する
- HTTP 429/503 受信時はエクスポネンシャルバックオフでリトライする
- User-Agent は `findsummits/1.0 (mailto:<メールアドレス>)` 形式とし、`params/fetch_config.ini` で設定する
- リクエスト間隔（ms）は `params/fetch_config.ini` の `interval_ms` で設定する
- 並列ワーカー数は `params/fetch_config.ini` の `max_parallel` で設定する（デフォルト: 4）

#### FR-002: DEM 階層フォールバック

- **対応 UR**: UR-001
- 標高値の取得優先順位: DEM5a → DEM5b → DEM5c → DEM10b
- DEM5a/5b/5c はズームレベル 15（5m 解像度相当）
- DEM10b はズームレベル 14（10m 解像度相当）。ズームレベル 14 タイルを 2×2 ピクセルに展開してレベル 15 グリッドに合わせる
- タイルが存在しない（HTTP 404）または有効値が存在しない場合は次の DEM 種別を試みる
- 詳細は [`decisions/ADR-002-dem-hierarchy-fallback.md`](decisions/ADR-002-dem-hierarchy-fallback.md) を参照

#### FR-003: 標高デコード・NODATA 処理

- **対応 UR**: UR-001
- RGB → 標高変換: `elev = (R×65536 + G×256 + B) / 100.0`（単位: m）
- NODATA センチネル: `(R=128, G=0, B=0)` → `-9999.0f`
- 解析中は NODATA を山頂検出・プロミネンス計算から除外する

---

### フェーズ2: 山頂・コル検出

#### FR-004: 3×3 メッシュ結合解析

- **対応 UR**: UR-001, UR-007
- 対象メッシュを中心に最大 3×3（最大 9 メッシュ）を結合して解析する
- 結合範囲の外周に 1px 幅の海面ボーダー（0m）を付加し、メッシュ端を海岸線とみなす
- 出力は中心メッシュ内のピークのみ（周辺 8 メッシュは Keyコル検出専用）
- 詳細は [`decisions/ADR-003-3x3-mesh-analysis.md`](decisions/ADR-003-3x3-mesh-analysis.md) を参照

#### FR-005: 局所最大点検出

- **対応 UR**: UR-001, UR-002
- 8 近傍比較による局所最大点をピーク候補とする
- Union-Find（経路圧縮・rank による union）でピークをグループ管理する

#### FR-006: Keyコル検出・プロミネンス計算

- **対応 UR**: UR-002, UR-005
- 各ピークに対してプロミネンスを規定するコル（Keyコル）を検出する
- プロミネンス = ピーク標高 − Keyコル標高
- Keyコルが解析範囲外の場合は `is_tile_top=1` を付与し、プロミネンスを暫定値とする

#### FR-007: プロミネンスフィルタ・per-mesh CSV 出力

- **対応 UR**: UR-001, UR-005
- C エンジン内での一次フィルタ: プロミネンス ≥ 130m（最終判定は Python 側で 150m）
- 出力先: `$DATA_DIR/results/csv/<meshcode>.csv`
- **出力カラム**（C エンジン出力、ヘッダー行あり）:

| カラム | 型 | 精度 | 説明 |
|---|---|---|---|
| peak_lat | float | 小数点8桁 | ピーク緯度 |
| peak_lon | float | 小数点8桁 | ピーク経度 |
| peak_elev | float | 小数点2桁 | ピーク標高（m） |
| col_lat | float | 小数点8桁 | Keyコル緯度（is_tile_top=1 時は 0.0） |
| col_lon | float | 小数点8桁 | Keyコル経度（is_tile_top=1 時は 0.0） |
| col_elev | float | 小数点2桁 | Keyコル標高（m） |
| prominence | float | 小数点2桁 | プロミネンス（m） |
| is_tile_top | int | 0/1 | 解析範囲内で Keyコル未発見の場合 1 |
| col_margin_px | int | — | Keyコルがメッシュ端から何 px 離れているか |
| center_mesh | int | — | 解析中心メッシュコード（4桁） |

#### FR-015: 標高地形図出力

- **対応 UR**: UR-001（解析結果の目視確認補助）
- C エンジンは per-mesh CSV と同時に標高地形図（`$DATA_DIR/images/<meshcode>_terrain.png`）を生成する
- 出力形式: PNG（RGB 8bit）
- 解像度: 長辺 6000px に縮小（アスペクト比保持）
- 色分け: 標高を 15 段階のグラデーションで表現（NODATA は濃い青で表示）
- `$DATA_DIR/images/` ディレクトリが存在しない場合は自動生成する
- 詳細は 6.7 を参照

---

### フェーズ3: SOTA 突合・差分分類

#### FR-008: per-mesh CSV 統合

- **対応 UR**: UR-003
- `$DATA_DIR/results/csv/` 配下の全 per-mesh CSV を読み込み、重複排除して統合する
- プロミネンス最終フィルタ: ≥ 150m

#### FR-009: SOTAリスト突合・match_status 判定

- **対応 UR**: UR-003
- `ref/summitslist.csv` の JA プレフィックスサミットと突合する
- 突合は Chebyshev 距離（ズームレベル 15 ピクセル単位）で行い、許容距離は `params/fetch_config.ini` の `merge.tolerance_px` で設定する
- **match_status 値**:
  - `matched`: 現行 SOTA サミットと位置が一致したピーク
  - `new`: 解析結果にあるが SOTA リストに未登録（新規候補）
  - `deleted`: SOTA リストにあるが解析結果で未検出（削除候補）
- **stability 値**:
  - `confirmed`: 解析回数=期待値かつ is_tile_top=0 のみ
  - `unstable`: is_tile_top=1 が含まれる、または解析回数不一致
  - `-`: 削除候補（解析結果なし）
- **match_status 判定の詳細ロジック**:
  - 各ピークに対し、tolerance_px 以下の Chebyshev 距離（ズームレベル 15 ピクセル単位、1px ≒ 4.8m）で最近傍のサミットを matched と判定する
  - 同距離で複数候補がある場合は SOTA リストの行順（先着順）で最初に見つかったものを採用する
  - Prominence < 150m のピークは matched 判定の対象外とする
  - 1 つのサミットに複数のピークが tolerance 範囲内に入る場合は距離が最小のピークとマッチする（そのサミットは以降の判定から除外）

#### FR-010: 削除候補のスコープ

- **対応 UR**: UR-003
- `--mesh-list` で指定されたメッシュセットの地理的 bbox 内に座標がある SOTA サミットのみを削除候補の対象とする（解析対象外メッシュのサミットを誤って削除候補にしない）

---

### フェーズ4: 申請用出力生成

#### FR-011: 申請書 XLSX 生成

- **対応 UR**: UR-004
- SOTA日本支部指定のフォーマット（`ref/SOTA-Summit-list-revision-request.xlsx` テンプレート）に準拠した XLSX ファイルを生成する
- 出力先: `$DATA_DIR/results/submission.xlsx`
- テンプレート構造:
  - 1シート構成
  - カラム: 既存山岳ID または県名 / アクション / 変更前（山岳名JP・EN・標高m） / 変更後（山岳名JP・EN・標高m） / 変更の根拠 / MT使用欄
- 出力シート構成: 1シート（テンプレート仕様に準拠）
- アクション列の値（テンプレートの選択肢）:
  - `追加`: match_status=new のサミット
  - `変更`: match_status=matched かつ座標・標高に差異があるサミット
  - `削除`: match_status=deleted のサミット
  - `その他`: **[TBD-02: 使用条件・該当ケースを別途確認]**
- **各アクションで使用するカラムのマッピング**（テンプレート列 A〜J）:

| アクション | A: 山岳ID/県名 | B: アクション | C: 変更前 名JP | D: 変更前 名EN | E: 変更前 標高 | F: 変更後 名JP | G: 変更後 名EN | H: 変更後 標高 | I: 根拠 |
|---|---|---|---|---|---|---|---|---|---|
| 追加 | 都道府県名 | 追加 | 空白 | 空白 | 空白 | 山岳名JP ※1 | 山岳名EN ※1 | peak_elev | 解析根拠 ※2 |
| 変更 | SummitCode | 変更 | SOTA 登録名JP | SOTA 登録名EN | sota_alt_m | 山岳名JP ※1 | 山岳名EN ※1 | peak_elev | 変更根拠 ※2 |
| 削除 | SummitCode | 削除 | SOTA 登録名JP | SOTA 登録名EN | sota_alt_m | 空白 | 空白 | 空白 | 削除根拠 ※2 |

※1 山岳名は自動取得不可。OSM・国土地理院地図を参照しながら人間系で記入すること（URD スコープ外）  
※2 根拠の記載例: 「findsummits 解析: prominence=XXX.Xm」「解析範囲内で prominence 150m 以上のピーク未検出」

#### FR-012: エビデンス CSV 生成

- **対応 UR**: UR-005
- merge.py が生成する統合 CSV（`$DATA_DIR/results/merged.csv`）が本要件を満たす
- 出力先: `$DATA_DIR/results/merged.csv`
- **出力カラム**（merge.py 出力）:

| カラム | 説明 |
|---|---|
| match_status | SOTAリスト突合結果（matched/new/deleted） |
| stability | 解析品質（confirmed/unstable/-） |
| summit_code | SOTAサミットコード（例: JA/TK-001）、新規は ZZ/ZZ-XXX ダミー |
| summit_name | サミット名（SOTA リストから） |
| sota_alt_m | SOTA リスト登録標高（m） |
| peak_lat | 検出ピーク緯度 |
| peak_lon | 検出ピーク経度 |
| peak_elev | 検出ピーク標高（m） |
| col_lat | Keyコル緯度 |
| col_lon | Keyコル経度 |
| col_elev | Keyコル標高（m） |
| prominence | プロミネンス（m） |
| is_tile_top | 独立峰フラグ（1=Keyコル未確定） |
| col_margin_px | Keyコルのメッシュ端マージン（px） |
| analysis_count | このピークが含まれた解析回数 |
| expected_count | このピークが含まれるべき期待解析回数 |
| orig_lat | SOTA リスト登録緯度 |
| orig_lon | SOTA リスト登録経度 |

#### FR-013: GeoJSON・HTML ビューア生成

- **対応 UR**: UR-006
- 入力: `$DATA_DIR/results/merged.csv`
- **出力先**:
  - `$DATA_DIR/results/merged.geojson`（GeoJSON）
  - `$DATA_DIR/results/merged_viewer.html`（静的 HTML ビューア）
- **フィーチャ構成**:
  - Point: 各ピーク。match_status で色分け（matched=緑 #00AA00 / new=マゼンタ #FF00FF / deleted=灰 #888888）
  - LineString: matched 行のみ、検出ピーク → SOTA 元座標を結ぶ（座標ずれ確認用）
- **Point プロパティ**:
  - `match_status`, `summit_code`, `summit_name`, `peak_elev`, `prominence`, `stability`, `icon` (地理院地図アイコン URL)
- **GeoJSON 属性の詳細定義**: **[TBD-03: ISSUE-008 設計確認後に確定]**
- **HTML ビューア仕様**:
  - Leaflet.js（CDN）+ 背景タイル切り替え機能（国土地理院標準地図・OSM・OpenTopoMap）を持つ
  - GeoJSON は外部参照（`merged.geojson` を相対パスで `fetch()`）
  - ローカルでの閲覧には HTTP サーバ（`python3 -m http.server`）が必要
  - GitHub Pages では静的ホスティングのみで動作
  - 地図帰属表示: Leaflet の attribution に `© 国土地理院`・`© OpenStreetMap contributors`・`© OpenTopoMap contributors` を必ず含める

#### FR-014: 独立峰対応（レベル14 広域再解析）

- **対応 UR**: UR-007
- `is_tile_top=1` のピークに対して、ズームレベル 14 で広域再解析を行いプロミネンスを確定させる
- 実装方針: レベル 15 タイルを読み込んで結合時に max pooling でレベル 14 化する（別途タイル取得不要）
- **詳細仕様**: **[TBD-01: ADR-004 の実装設計完了後に確定]**
  - 座標変換ロジック（ズーム15→14 変換）
  - col_margin_px への影響
  - 1px ボーダーの地理的幅変化の許容判断
  - 257×257 オーバーラップの max pooling 時の処理
- 詳細は [`decisions/ADR-004-level14-max-pooling-isolated-peaks.md`](decisions/ADR-004-level14-max-pooling-isolated-peaks.md) を参照

---

## 5. 非機能要件

#### NFR-001: 精度（プロミネンス判定）

- **対応 UR**: UR-002
- プロミネンス ≥ 150m をサミット候補として出力すること
- C エンジンの一次フィルタは 130m（境界付近の精度マージン確保のため）
- 最終 150m 判定は merge.py で実施

#### NFR-002: メモリ使用量

- **対応 UR**: UR-001
- 実測値（9メッシュ最大解析時）: Union-Find 解析ピーク 90.8%（≒56.9GB）、物理メモリ 62.72GB + スワップ 39.7GB で動作確認済み
- 全国 176 メッシュ逐次実行時は中心付近（5339 等）でスワップ使用が発生しうる。許容範囲内とする。

#### NFR-003: 再現性（決定論的出力）

- **対応 UR**: UR-009
- 同一タイルキャッシュ・同一パラメータで実行した場合、出力 CSV の内容が一致すること
- ピークのソート順を決定論化するために `cmp_elev_desc` に 2次キー（x→y）を設ける

#### NFR-004: アクセスマナー

- **対応 UR**: UR-010
- User-Agent: `findsummits/1.0 (mailto:<メールアドレス>)` 形式（必須）
- リクエスト間隔: `params/fetch_config.ini` の `interval_ms`（デフォルト: 100ms）
- HTTP 429/503 受信時はバックオフを行いリトライする

#### NFR-005: 処理時間目標

- **対応 UR**: UR-001
- **[TBD-04: 全国 176 メッシュ処理の実績データ取得後に設定]**
- 参考: 1 メッシュ当たり数分〜十数分（メッシュの地形・解析範囲による）

#### NFR-006: 可搬性・環境

- **対応 UR**: UR-008
- 動作環境は [`environment.md`](environment.md) に定める特定マシン上のみを前提とする
- サーバー構成・マルチユーザー運用は対象外

#### NFR-007: ログ出力

- **対応 UR**: UR-009
- C エンジン（`findsummits`）はメッシュごとに `$DATA_DIR/logs/<meshcode>.log` を出力する
- stdout とログファイルに同時出力する（二重出力）
- 記録内容: 解析開始・解析範囲・タイル読み込み進捗・標高地形図 PNG 出力・Union-Find 実行時間・CSV 保存件数・解析完了・エラー発生時の失敗理由
- `$DATA_DIR/logs/` ディレクトリが存在しない場合は自動生成する

---

## 6. 外部インターフェース仕様

### 6.1 入力: 地理院標高タイル

| 項目 | 仕様 |
|---|---|
| 形式 | PNG（RGB エンコード） |
| ズームレベル | DEM5a/5b/5c: 15 / DEM10b: 14 |
| タイルサイズ | 256×256 px |
| RGB→標高変換 | `elev = (R×65536 + G×256 + B) / 100.0` |
| NODATA | R=128, G=0, B=0 → -9999.0m |
| タイル URL パターン | `https://cyberjapandata.gsi.go.jp/xyz/{dem}/{z}/{x}/{y}.png` |
| キャッシュ保存先 | `$DATA_DIR/tiles/{z}/{x}/{y}_{dem}.png` |

### 6.2 入力: SOTA サミットリスト CSV

| 項目 | 仕様 |
|---|---|
| ファイル | `ref/summitslist.csv` |
| 取得元 | https://www.sotadata.org.uk/summitslist.csv |
| 対象レコード | SummitCode が `JA` で始まるもの |
| 使用カラム | SummitCode, SummitName, AltM, Latitude, Longitude（その他は無視） |

### 6.3 出力: 申請書 XLSX

| 項目 | 仕様 |
|---|---|
| ファイル | `$DATA_DIR/results/submission.xlsx` |
| テンプレート | `ref/SOTA-Summit-list-revision-request.xlsx` |
| カラム構成 | FR-011 参照 |
| シート構成 | 1シート（アクション列で追加/変更/削除/その他を識別） |

### 6.4 出力: エビデンス CSV

| 項目 | 仕様 |
|---|---|
| ファイル | `$DATA_DIR/results/merged.csv` |
| エンコーディング | UTF-8 |
| 区切り文字 | カンマ |
| カラム | FR-012 参照 |

### 6.5 出力: GeoJSON・HTML ビューア

| 項目 | 仕様 |
|---|---|
| GeoJSON ファイル | `$DATA_DIR/results/merged.geojson` |
| 座標参照系 | WGS84（EPSG:4326） |
| フィーチャ構成 | FR-013 参照 |
| HTML ビューアファイル | `$DATA_DIR/results/merged_viewer.html` |
| 地図ライブラリ | Leaflet.js（CDN 参照） |
| 背景タイル | 国土地理院標準地図・OSM・OpenTopoMap（切り替え可能） |
| GeoJSON 参照方式 | 外部参照（同ディレクトリの merged.geojson を fetch） |
| ローカル閲覧 | HTTP サーバ（`python3 -m http.server`）が必要 |
| GitHub Pages | 静的ホスティングのみで動作 |

### 6.6 内部インターフェース: per-mesh CSV（C → Python 境界）

| 項目 | 仕様 |
|---|---|
| ファイル | `$DATA_DIR/results/csv/<meshcode>.csv` |
| エンコーディング | UTF-8 |
| カラム | FR-007 参照 |

### 6.7 出力: 標高地形図（Terrain-RGB PNG）

| 項目 | 仕様 |
|---|---|
| ファイル | `$DATA_DIR/images/<meshcode>_terrain.png` |
| 形式 | PNG（RGB 8bit） |
| 解像度 | 長辺 6000px に縮小（アスペクト比保持） |
| 色分け | 標高 15 段階グラデーション（NODATA: 濃い青 / 海面: 薄い青 / 低地: 緑 / 中地: 黄茶 / 高山: 白） |
| 生成タイミング | `findsummits` 実行時（per-mesh CSV と同時） |

---

## 7. 依存関係・環境

環境詳細は [`environment.md`](environment.md) を参照。

### C エンジン (`src/`)

| 依存 | バージョン |
|---|---|
| GCC | C99 準拠 |
| libpng | システム提供 |
| libm | システム提供 |
| pthread | システム提供 |

### Python スクリプト (`scripts/`)

| 依存 | 用途 |
|---|---|
| Python 3 | スクリプト実行 |
| openpyxl | XLSX 生成 |
| requests | タイル取得（prefetch_tiles.py） |

---

## 8. 制約・前提条件

URD セクション 4 より:

- 標高データは国土地理院タイルのみ使用（DEM5a/5b/5c/DEM10b の優先順）
- サミット判定基準は SOTA ルール（プロミネンス ≥ 150m）に従う
- 申請書フォーマットは SOTA 日本支部指定の XLSX テンプレートに従う
- 解析対象は日本国内の 1 次メッシュ全 176 メッシュ
- タイル取得時はインターネット接続が必要（解析・出力生成はオフライン可）
- 地理院サーバへのアクセスは[国土地理院コンテンツ利用規約](https://www.gsi.go.jp/kikakuchousei/kikakuchousei40182.html)に従い、サーバへの過度な負荷を避けること
- 本ツールは開発者本人の特定マシン上での動作を前提とする（[`environment.md`](environment.md) 参照）

---

## 9. スコープ外

URD セクション 5 より:

- 日本以外の SOTA 申請
- 山岳名の取得・変更（解析結果から山岳名を自動取得する手段がなく技術的に困難なため対象外。新規サミット・変更サミットともに、申請時は HTML ビューア等で OSM・国土地理院地図を参照しながら人間系で確認・記入すること）
- DEM1a（データ量が DEM5 の 25 倍、精度向上が僅少なため採用しない）
- SOTA 申請書の提出・承認プロセス（ツールは申請書生成まで。提出は手動）
- リアルタイム処理（バッチ処理のみ）
- 地形の現地確認（目視確認は GeoJSON または HTML ビューアを使って地図上で行う）

---

## 10. TBD 一覧

今後埋める必要がある未確定項目。

| ID | 箇所 | 内容 | 埋める条件 |
|---|---|---|---|
| TBD-01 | FR-014 | 独立峰レベル 14 再解析の詳細仕様（座標変換・col_margin_px・ボーダー・オーバーラップ） | ADR-004 実装設計着手前に確定 |
| TBD-02 | FR-011 | 「その他」アクションの使用条件 | 申請書テンプレートの運用確認後に確定 |
| TBD-03 | FR-013, 6.5 | GeoJSON フィーチャプロパティの完全定義 | ISSUE-008 設計確認後 |
| TBD-04 | NFR-005 | 全国 176 メッシュ処理時間目標 | フルパイプライン実行後に実績から設定 |
