# findsummits - ソフトウェア要件仕様書 (SRS)

| 項目 | 内容 |
|---|---|
| 作成日 | 2026-04-30 |
| 最終更新日 | 2026-05-31 |
| ステータス | ドラフト（FR-009/FR-012 出力 xlsx 化・merged_summit.xlsx / merged_summit_revised.xlsx 確定・FR-012 呼称変更・UR-011 対応として FR-013/FR-012/FR-021 に出典表示要件追加） |
| 参照 URD | [`01_URD.md`](01_URD.md) |

---

## 目次

1. [目的・範囲](#1-目的範囲)
2. [用語定義](#2-用語定義)
3. [システムアーキテクチャ概要](#3-システムアーキテクチャ概要)
   - [3.1 システムコンテキスト](#31-システムコンテキスト)
   - [3.2 主要コンポーネント構成](#32-主要コンポーネント構成)
   - [3.3 フェーズ分割の俯瞰](#33-フェーズ分割の俯瞰)
   - [3.4 データ辞書](#34-データ辞書)
4. [機能要件](#4-機能要件)
   - [フェーズ1: タイル取得・前処理](#フェーズ1-タイル取得前処理)
     - [FR-017: N03 行政区域前処理（データ準備）](#fr-017-n03-行政区域前処理データ準備)
     - [FR-001: 標高タイル事前取得](#fr-001-標高タイル事前取得)
   - [フェーズ2: ピーク解析（per-mesh）](#フェーズ2-ピーク解析per-mesh)
     - [FR-002: DEM 階層フォールバック](#fr-002-dem-階層フォールバック)
     - [FR-003: 標高デコード・NODATA 処理](#fr-003-標高デコードnodata-処理)
     - [FR-004: 3×3 メッシュ結合解析](#fr-004-33-メッシュ結合解析)
     - [FR-005: ピーク候補検出](#fr-005-ピーク候補検出)
     - [FR-006: コル検出・プロミネンス計算](#fr-006-コル検出プロミネンス計算)
     - [FR-016: ピーク域ポリゴン生成](#fr-016-ピーク域ポリゴン生成)
     - [FR-007: プロミネンスフィルタ・per-mesh CSV 出力](#fr-007-プロミネンスフィルタper-mesh-csv-出力)
     - [FR-015: 標高地形図出力](#fr-015-標高地形図出力)
   - [フェーズ3: ピーク統合（全国）](#フェーズ3-ピーク統合全国)
     - [FR-008: per-mesh CSV 統合](#fr-008-per-mesh-csv-統合)
     - [FR-018: per-mesh activation.geojson 統合](#fr-018-per-mesh-activationgeojson-統合)
     - [FR-014: 独立峰のコル探索](#fr-014-独立峰のコル探索)
   - [フェーズ4: SOTA突合・中心成果物生成・HTML ビューア生成](#フェーズ4-sota突合中心成果物生成html-ビューア生成)
     - [FR-009: SOTAリスト突合・match_status 判定](#fr-009-sotaリスト突合match_status-判定)
     - [FR-010: 削除候補のスコープ](#fr-010-削除候補のスコープ)
     - [FR-013: HTML ビューア生成](#fr-013-html-ビューア生成)
   - [フェーズ5: 申請書生成](#フェーズ5-申請書生成)
     - [FR-019: HTML ビューア機能仕様](#fr-019-html-ビューア機能仕様)
     - [FR-011: 申請書 XLSX 生成](#fr-011-申請書-xlsx-生成)
     - [FR-012: サミット一覧（申請内容反映版）生成](#fr-012-サミット一覧申請内容反映版生成)
     - [FR-020: 公開用 HTML ビューア生成](#fr-020-公開用-html-ビューア生成)
     - [FR-021: 申請エビデンス ZIP 生成](#fr-021-申請エビデンス-zip-生成)
5. [非機能要件](#5-非機能要件)
   - [NFR-001: 精度（プロミネンス判定）](#nfr-001-精度プロミネンス判定)
   - [NFR-002: メモリ使用量](#nfr-002-メモリ使用量)
   - [NFR-003: 再現性（決定論的出力）](#nfr-003-再現性決定論的出力)
   - [NFR-004: アクセスマナー](#nfr-004-アクセスマナー)
   - [NFR-005: 処理時間目標](#nfr-005-処理時間目標)
   - [NFR-006: 可搬性・環境](#nfr-006-可搬性環境)
   - [NFR-007: ログ出力](#nfr-007-ログ出力)
   - [NFR-008: UI レスポンス](#nfr-008-ui-レスポンス)
6. [外部インターフェース仕様](#6-外部インターフェース仕様)
   - [6.1 入力: 地理院標高タイル](#61-入力-地理院標高タイル)
   - [6.2 入力: SOTA サミットリスト CSV](#62-入力-sota-サミットリスト-csv)
   - [6.3 出力: 申請書 XLSX](#63-出力-申請書-xlsx)
   - [6.4 出力: サミット一覧（申請内容反映版）](#64-出力-サミット一覧申請内容反映版)
   - [6.5 出力: サミット一覧（突合後）](#65-出力-サミット一覧突合後)
   - [6.6 出力: GeoJSON・作業用 HTML ビューア](#66-出力-geojsonhtml-作業用-html-ビューア)
   - [6.7 中間ファイル: メッシュ別ピーク候補 CSV](#67-中間ファイル-メッシュ別ピーク候補-csv)
   - [6.8 出力: 標高地形図（Terrain-RGB PNG）](#68-出力-標高地形図terrain-rgb-png)
   - [6.9 中間ファイル: メッシュ別ピーク域 GeoJSON](#69-中間ファイル-メッシュ別ピーク域-geojson)
   - [6.10 入力: N03 前処理済みファイル（FR-017 生成）](#610-入力-n03-前処理済みファイルfr-017-生成)
   - [6.11 入力: SOTA 既存サミット GeoJSON（geojson_v{N}）](#611-入力-sota-既存サミット-geojsongeojson_vn)
   - [6.12 中間ファイル: 全国統合済みピーク域 GeoJSON](#612-中間ファイル-全国統合済みピーク域-geojson)
   - [6.13 出力: 公開用 HTML（閲覧専用・ブラウザダウンロード）](#613-出力-公開用-html閲覧専用ブラウザダウンロード)
7. [外部システム依存関係・環境](#7-外部システム依存関係環境)
8. [制約・前提条件](#8-制約前提条件)
9. [スコープ外](#9-スコープ外)

---

## 1. 目的・範囲

本文書は [`01_URD.md`](01_URD.md) に定めるユーザー要件（UR-001〜UR-010）を実現するための
ソフトウェア要件を規定する。

**対象システム**: findsummits（解析エンジン + Python スクリプト群）  
**対象バージョン**: 1.0（予定）  
**対象外**: 実装詳細（HLD/LLD）、テスト仕様（UT/IT/ST）

---

## 2. 用語定義

[`00_GLOSSARY.md`](00_GLOSSARY.md) を参照。

---

## 3. システムアーキテクチャ概要

### 3.1 システムコンテキスト

![システムコンテキスト図](figures/context.drawio.svg)

本システムは以下の外部要素と接続する。

- **外部データソース**
  - 国土地理院 標高タイル（DEM5a/5b/5c/DEM10b、ズームレベル15 PNG）
  - 国土交通省 国土数値情報 N03 行政区域 GeoJSON
  - SOTA Reflector summitslist.csv
- **外部アクター**
  - 申請担当者（ユーザー）— データ取得・解析実行・申請書確認
  - SOTA 日本支部マネージャ — 申請書受領・審査
- **外部成果物（本システムの最終出力）**
  - 申請書 XLSX（SOTA 日本支部提出用）
  - サミット一覧（申請内容反映版）
  - 証跡 GeoJSON

### 3.2 主要コンポーネント構成

各コンポーネントの責務と主要 I/O を論理的に定義する。実装言語・実装ファイル名の決定は ADR に委ねる（[ADR-SRS-001](decisions/ADR-SRS-001-hybrid-c-python-architecture.md)、[ADR-SRS-010](decisions/ADR-SRS-010-cpp-opencv-migration.md) 参照）。

| コンポーネント | 責務 | 主要入力 | 主要出力 |
|---|---|---|---|
| タイル取得コンポーネント | DEM タイルを国土地理院から取得・キャッシュ | メッシュコード、取得設定 | キャッシュ済み PNG タイル |
| 行政区域前処理コンポーネント | 都道府県・振興局境界 GeoJSON を解析用形式に変換（初回のみ） | N03 行政区域 GeoJSON | 軽量化された境界 GeoJSON |
| 地形解析エンジン | DEM からピーク／コル／プロミネンス／AZ・delete判定ゾーンを検出 | キャッシュ済み PNG タイル | 処理モードにより異なる（通常モード: per-mesh CSV / per-mesh activation GeoJSON / 標高地形図 PNG、広域モード: per-mesh CSV のみ） |
| 統合・突合コンポーネント | per-mesh 成果物を統合し SOTA リストと突合、rationale 生成・不備フラグ判定・exit code 制御 | per-mesh CSV／GeoJSON、SOTA リスト CSV、境界 GeoJSON | **merged.geojson**（中心成果物）、merged_summit.xlsx（サミット一覧（突合後））、merged_peak.csv（内部 work CSV）、merged_activation.geojson（内部中間） |
| 可視化生成コンポーネント | merged.geojson から HTML ビューアを生成 | merged.geojson | merged_viewer.html |
| 申請書生成 UI | HTML ビューア内でユーザー操作に応じて申請書 XLSX を生成 | ユーザー操作（HTML ビューア上） | 申請用 XLSX |

### 3.3 フェーズ分割の俯瞰

![フェーズ分割の俯瞰図](figures/phases_overview.drawio.svg)

フェーズ1〜4 はバッチ処理、フェーズ5 はローカル HTML ビューア上のユーザー操作で構成される。

```
【フェーズ1: タイル取得・前処理】
タイル取得コンポーネント              タイル事前取得
行政区域前処理コンポーネント           N03 行政区域前処理（初回のみ）
       └─ $DATA_DIR/ref/N03-{n03_year}_regions.geojson  ← 統合・突合コンポーネントが参照
       ↓ ユーザーが解析コマンドを実行
【フェーズ2: ピーク解析（per-mesh）】
地形解析エンジン         ピーク・コル検出・ピーク域ポリゴン生成
       ├─ per-mesh CSV              ($DATA_DIR/results/csv/<meshcode>.csv)
       ├─ per-peak polygons         ($DATA_DIR/results/csv/<meshcode>_activation.geojson)
       └─ 標高地形図                ($DATA_DIR/images/<meshcode>_terrain.png)
       ↓
【フェーズ3: ピーク統合（全国・独立峰広域再解析を内包）】
統合コンポーネント        ピーク統合・独立峰コル探索
       ├─ merged_peak.csv           （内部 work CSV）
       └─ merged_activation.geojson ($DATA_DIR/results/merged_activation.geojson) ← 内部中間ファイル
       ↓
【フェーズ4: SOTA突合・中心成果物生成・HTML ビューア生成】
統合・突合コンポーネント   SOTA突合・中心成果物生成
       ├─ merged.geojson            ($DATA_DIR/results/merged.geojson) ← 中心成果物（全フィーチャ + rationale）
       └─ merged_summit.xlsx        ($DATA_DIR/results/merged_summit.xlsx) ← サミット一覧（突合後）
可視化生成コンポーネント   HTML ビューア生成
       └─ merged_viewer.html        ← 編集可能なローカル HTML ビューア（GeoJSON 埋め込み・rationale 編集機能付き）
       ↓ ユーザーが HTML ビューアで確認・rationale 編集後にエクスポートを実行
【フェーズ5: 申請書生成（ローカル HTML ビューア上のユーザー操作）】
       ├─ 申請書 XLSX               （ブラウザダウンロード）
       ├─ サミット一覧（申請内容反映版）  merged_summit_revised.xlsx
       └─ 公開用 HTML               （ブラウザダウンロード）
```

### 3.4 データ辞書

この SRS の機能要件（FR-XXX）でパラメータ・入出力データを参照するときは、本セクションで定義した名称を使用すること。定義の重複を防ぎ、変更時の更新箇所を一元化するためのセクションである（ADR-SRS-014 参照）。

#### 3.4.1 設定可能項目

ユーザーまたは運用者が調整できる閾値・パラメータ。「どのファイル・引数で渡すか」の受け渡し形式は HLD で定義する。

| 項目名 | 意味 | デフォルト値 | 許容範囲 |
|---|---|---|---|
| プロミネンス一次フィルタ閾値 | フェーズ2 で per-mesh CSV に出力するピーク候補の最低プロミネンス | 130m | 0〜1000m（最終フィルタ閾値 + 20m 以下を推奨） |
| プロミネンス最終フィルタ閾値 | フェーズ3 で統合後に採用するピーク候補の最低プロミネンス | 150m | 0〜1000m |
| delete_zone 比高上限 | delete判定ゾーン Flood Fill の比高上限。Key コルとの標高差と本値の小さい方が Flood Fill の下限閾値となる | 250m | 100〜500m（実測値。変更は ADR 判断を要する） |
| 解析範囲サイズ N | 通常解析の中心メッシュを含む N×N メッシュ数 | 3 | {3, 4, 5, 6} |
| タイル取得の最大並列数 | 標高タイルの HTTP 取得を並行して行う最大リクエスト数 | 4 | 1〜16（地理院サーバー負荷に配慮した上限） |
| タイル取得のリクエスト間隔 | タイル取得リクエスト間の最短待機時間 | HLD で定義 | 0〜5000ms |
| User-Agent 識別子 | タイル取得 HTTP リクエストの送信元識別子。ツール名と連絡先メールアドレスを含む（地理院側での問い合わせ対応のため必須） | — | `<ツール名>/<バージョン> (mailto:<メールアドレス>)` 形式 |
| N03 行政区域データ年版 | 国土数値情報 N03 データの年版。対象ファイルのファイル名に含まれる年 | — | 4桁の年（例: 2024） |

#### 3.4.2 入出力データ

| データ名 | 意味 | 制約・形式 |
|---|---|---|
| 1次メッシュコード | 日本測地系の1次標準地域メッシュコード。解析対象区画の識別に使用 | 4桁数字（例: 4929）。範囲 3622〜6848 |
| 標高タイル | 地理院 DEM5a/5b/5c/DEM10b の RGB エンコード標高データ（PNG）。解析前にローカルキャッシュ済みであることを前提とする | 256×256px PNG、RGB 各8bit。デコード式: `(R×65536 + G×256 + B) / 100.0`（m）。無効値: R=128・G=0・B=0 |
| per-mesh ピーク候補 CSV | フェーズ2（FR-007）が出力する 1次メッシュ単位のピーク候補リスト | CSV 形式。フォーマットは FR-007 出力仕様参照 |
| per-mesh アクティベーションゾーン GeoJSON | フェーズ2（FR-016）が出力する 1次メッシュ単位のピーク域ポリゴン | GeoJSON（Polygon フィーチャ集合） |
| 統合ピーク候補（merged.geojson） | フェーズ3〜4 を通じて構築される全国統合の中心成果物。全 Point・Polygon・LineString・metadata を含む（ADR-SRS-013） | GeoJSON FeatureCollection |
| SOTA サミットリスト CSV | SOTA Reflector が公開する世界全サミットリスト。本システムでは JA（日本支部）エントリを対象とする | CSV 形式。フォーマットは 6.2 参照 |
| 申請書 XLSX | SOTA 日本支部への山岳リスト更新申請書。フェーズ5（FR-011）が生成する | XLSX 形式。フォーマットは 6.3 参照 |
| 標高地形図 PNG | フェーズ2（FR-015）が生成する解析範囲の標高色分け PNG。目視確認用 | PNG。長辺 6000px に縮小 |
| N03 前処理済み地域 GeoJSON | FR-017 が生成する都道府県/振興局・市区町村単位の行政区域 GeoJSON | GeoJSON。フォーマットは 6.10 参照 |

---

## 4. 機能要件

### フェーズ1: タイル取得・前処理

#### FR-017: N03 行政区域前処理（データ準備）

- **対応 UR**: [UR-001](01_URD.md#ur-001), [UR-003](01_URD.md#ur-003), [UR-004](01_URD.md#ur-004)
- **概要**: 国土数値情報の市区町村単位の行政区域データを前処理し、FR-009 用の都道府県/振興局・市区町村 GeoJSON と FR-003 用の北方領土除外タイルリストを生成する、初回のみ実行するデータ準備スクリプト（`preprocess_pref_boundaries.py`）。
- **入力**: `$DATA_DIR/ref/N03-{n03_year}.geojson`（国土数値情報 N03-{n03_year} 全国行政区域データ。ユーザーが事前にダウンロードして配置する。`{n03_year}` は `params/config.ini` の `n03_year` パラメータで指定）
- **出力**（3種）:
  1. `$DATA_DIR/ref/N03-{n03_year}_regions.geojson` — 都道府県/振興局単位（47都道府県 + 北海道14振興局 = 計61地域）。FR-009 での SOTA エリアコード自動付与に使用
  2. `$DATA_DIR/ref/N03-{n03_year}_municipalities.geojson` — 市区町村単位（約2,000地域）。FR-009 での所在地（市区町村名）取得に使用
  3. `$DATA_DIR/ref/N03-{n03_year}_excluded_tiles.txt` — 北方領土6村（市区町村コード 01696〜01701）に該当するポリゴン内のズームレベル15タイル座標リスト（x y 形式、1行1タイル）。判定基準: タイルの中心点が対象ポリゴン内に含まれるタイルを列挙する（詳細: [`decisions/ADR-URD-005-northern-territories-exclusion.md`](decisions/ADR-URD-005-northern-territories-exclusion.md)）。FR-003 での NODATA マスクに使用
- **詳細**:
  - **実行タイミング**: 初回のみ（突合処理の前に一度だけ実行する）
  - **北方領土の識別**: N03 の行政区域コード属性（N03_007）が 01696〜01701 に一致するポリゴンを除外対象として識別する（詳細: [`decisions/ADR-URD-005-northern-territories-exclusion.md`](decisions/ADR-URD-005-northern-territories-exclusion.md)）
  - **フォールバック**: 前処理済みファイルが存在しない場合、北方領土タイル除外（FR-003）をスキップして警告ログを出力し、地域不明を示す仮サミットコード（`ZZ/ZZ-A01` 形式）を付与して処理を続行する（処理は停止しない）。仮サミットコードの採番ロジックは [FR-009 参照](#fr-009-sotaリスト突合match_status-判定)
  - 出典: [`ref/SOURCES.md`](../ref/SOURCES.md)（国土数値情報 N03 行政区域）

#### FR-001: 標高タイル事前取得

- **対応 UR**: [UR-001](01_URD.md#ur-001), [UR-010](01_URD.md#ur-010)
- **概要**: 解析対象メッシュをカバーする国土地理院標高タイル（ズームレベル15、256×256px PNG）を取得し `$DATA_DIR/tiles/` にキャッシュする。
- **入力**: メッシュコードリストファイル（解析対象の1次メッシュコードを1行1件で列挙したファイル。`params/mesh_list_japan.txt` が標準）
- **出力**: `$DATA_DIR/tiles/{z}/{x}/{y}_{dem}.png`（DEM 種別ごと。`{dem}` は DEM 種別の最後1桁: `a`=DEM5a、`b`=DEM5b/DEM10b、`c`=DEM5c。DEM5b と DEM10b はズームレベル（z=15/14）で区別）
- **詳細**:
  - 各メッシュコードについて、[メッシュコード → 緯度経度変換](00_GLOSSARY.md#mesh-to-latlon)の計算式でそのメッシュの四隅の緯度経度を求め、[緯度経度 → XYZ タイル番号変換](00_GLOSSARY.md#latlon-to-tile)の計算式から取得対象のタイル番号範囲を特定する
  - **取得範囲**: メッシュコードリストの各メッシュコードについて、そのメッシュをカバーする最小範囲の標高タイルを取得する（隣接メッシュへの拡張は行わない。全176メッシュを一括取得する運用では、メッシュ境界付近のタイルが複数メッシュから重複して取得されるが問題ない）
  - キャッシュ済みタイルの再取得時は `If-Modified-Since` ヘッダーを付与してリクエストし、サーバー側に更新がない場合（HTTP 304）はダウンロードをスキップする
  - サーバーから「リクエスト過多（HTTP 429）」または「一時利用不可（HTTP 503）」が返された場合、待機時間を倍々に増やしながら（エクスポネンシャルバックオフ）リトライする
  - タイル取得の HTTP リクエストに、ツール名と連絡先メールアドレスを含む識別子（User-Agent: `findsummits/1.0 (mailto:<メールアドレス>)`）を付与する（地理院サーバー側でアクセス元を特定・問い合わせできるようにするため）。メールアドレスはパラメータファイルに記入する（詳細は HLD 参照）
  - リクエスト間隔はパラメータファイルで設定する（詳細は HLD 参照）
  - タイルを並列で取得する（並列数はパラメータファイルで設定する、既定値: 4）
  - DEM5a / DEM5b / DEM5c / DEM10b の各種別について、対象タイル範囲の全タイルを種別間の条件分岐なしに独立して取得する（FR-002 のピクセル単位フォールバックが機能するには全種別のタイルがローカルに揃っている必要があるため）
  - 地理院側未整備のタイル（HTTP 404）については、同パスのローカルキャッシュが存在すれば削除し、地理院側と同じ状態に保つ

---

### フェーズ2: ピーク解析（per-mesh）

#### FR-002: DEM 階層フォールバック

- **対応 UR**: [UR-001](01_URD.md#ur-001)
- **概要**: タイル読み込み時にピクセル単位で DEM5a→DEM5b→DEM5c→DEM10b の順でフォールバックし、有効な標高値を得る。
- **入力**: タイル読み込み時のピクセル（FR-004 のタイル読み込み処理から適用される）
- **出力**: フォールバック適用後の標高値（有効値または -9999m）
- **詳細**:
  - **適用タイミング**: タイル読み込み時にピクセル単位で実行する
  - 標高データの利用優先順位は DEM5a → DEM5b → DEM5c → DEM10b の順とする
  - DEM5a/5b/5c は 5m 解像度、DEM10b は 10m 解像度。DEM10b のデータは 2×2 ピクセルに拡大して DEM5 と同じグリッドに合わせて使用する
  - 上位 DEM のピクセルが無効値（-9999m）または該当タイルが存在しない場合、同座標の次の DEM 種別のピクセル値を使用する
  - 詳細は [`decisions/ADR-SRS-002-dem-hierarchy-fallback.md`](decisions/ADR-SRS-002-dem-hierarchy-fallback.md) を参照

#### FR-003: 標高デコード・NODATA 処理

- **対応 UR**: [UR-001](01_URD.md#ur-001)
- **概要**: PNG タイルの RGB ピクセルから標高値をデコードし、NODATA・海面・北方領土・竹島・マイナス標高を統一的に処理する。
- **入力**: PNG タイルピクセル（FR-002 のフォールバック後）
- **出力**: デコード済みピクセル標高値（NODATA は -9999m、海面下は ELEV_SEA=0.0m に統一）
- **詳細**:
  - 標高タイルは PNG 画像形式で配信されており、各ピクセルの RGB 値から標高（m）を計算する: `標高 = (R×65536 + G×256 + B) / 100.0`
  - 標高値なし（海・データ未整備等）を示すピクセル（R=128, G=0, B=0）は無効値（-9999m）として扱う
  - 無効値のピクセルはピーク検出・プロミネンス計算の対象から除外する
  - **北方領土タイル除外**: タイル読み込み時に `$DATA_DIR/ref/N03-{n03_year}_excluded_tiles.txt`（FR-017 生成）を参照し、リスト内のタイル（ズームレベル15の x/y 座標）は全ピクセルを無効値（-9999m）として扱う。ファイル未存在時の動作は [FR-017 参照](#fr-017-n03-行政区域前処理データ準備)（詳細: [`decisions/ADR-URD-005-northern-territories-exclusion.md`](decisions/ADR-URD-005-northern-territories-exclusion.md)）
  - **竹島の除外**: 竹島が含まれるメッシュ 5531 を `params/mesh_list_japan.txt` から除外することで対応する。N03 ポリゴンベースのタイルマスク（北方領土と同方式）は使用しない。FR-003 での追加処理はない（詳細: [`decisions/ADR-URD-009-takeshima-exclusion.md`](decisions/ADR-URD-009-takeshima-exclusion.md)）
  - **マイナス標高の扱い**: デコード式の中間値 x（符号なし 24bit）が 0x800000（= 2^23）より大きい場合は `x - 2^24` を適用して符号付き整数に変換し、100 で除算する。これにより干拓地など海面下ピクセルは負の標高値としてデコードされる。ただしデコード完了後のタイル処理段階で、負値は無効値（-9999m）と同様に海面高度（`ELEV_SEA = 0.0m`）に統一される。この時点で「本来の海面（0m）」「NODATA」「負の標高」は区別されなくなり、いずれもピーク検出・プロミネンス計算の対象から除外される

#### FR-004: 3×3 メッシュ結合解析

- **対応 UR**: [UR-001](01_URD.md#ur-001), [UR-007](01_URD.md#ur-007)
- **概要**: メッシュコードリストの各メッシュを 3×3 単位（中心+隣接最大8）で解析対象として結合画像を生成し、per-mesh パイプライン（FR-005〜FR-007・FR-016）に引き渡すオーケストレーション。
- **入力**: メッシュコードリストファイル（[FR-001](#fr-001-標高タイル取得) と同じファイル）
- **出力**: 結合画像 + 解析対象メッシュ範囲（メモリ上。後段の FR-005/006/016/007 に引き渡し、最終的に per-mesh CSV/GeoJSON として出力）
- **詳細**:
  - メッシュコードリストをコード昇順にソートしてから、1 メッシュずつ順番に解析する
  - 解析対象メッシュの決定: 対象メッシュを中心とした 3×3 のメッシュグリッド（最大 9 メッシュ）のうち、メッシュコードリストに存在するものを解析対象とする。

  <table border="1">
  <tr>
    <th colspan="3" align="center">抽象</th>
    <th width="30"></th>
    <th colspan="3" align="center">具体例（5239 を中心とした場合）</th>
  </tr>
  <tr>
    <td align="center">隣接</td><td align="center">隣接</td><td align="center">隣接</td>
    <td></td>
    <td align="center">5338</td><td align="center">5339</td><td align="center">5340</td>
  </tr>
  <tr>
    <td align="center">隣接</td><td align="center"><b>対象</b></td><td align="center">隣接</td>
    <td></td>
    <td align="center">5238</td><td align="center"><b>5239</b></td><td align="center">5240</td>
  </tr>
  <tr>
    <td align="center">隣接</td><td align="center">隣接</td><td align="center">隣接</td>
    <td></td>
    <td align="center">5138</td><td align="center">5139</td><td align="center">NODATA</td>
  </tr>
  </table>

  上が北・右が東。5140 はリスト外（海域）→ タイル未取得のため NODATA として解析
  - 解析対象メッシュ全体の四隅の緯度経度を求め（[メッシュコード → 緯度経度変換](00_GLOSSARY.md#mesh-to-latlon) 参照）、その緯度経度をもとに標高タイル（ズームレベル 15）のタイル番号を特定する（[緯度経度 → XYZ タイル番号変換](00_GLOSSARY.md#latlon-to-tile) 参照）
  - 特定したタイルを 1 枚の画像に結合する。タイルとメッシュの境界は一致しないため、結合画像はメッシュ境界より数十 m はみ出す
  - 結合範囲の外周に 1px 幅の海面ボーダー（0m）を付加し、メッシュ端を海岸線とみなす
  - 解析対象メッシュ全体の地理的範囲内のピークを per-mesh CSV に出力する
  - 全176メッシュを逐次実行すると、同一ピークが複数の解析（最大9回）に含まれる。フェーズ3 の重複排除後に `analysis_count`（実際の解析回数）と `expected_count`（期待解析回数）で安定性を評価する（[FR-009 参照](#fr-009-sotaリスト突合match_status-判定)）
  - **処理モードパラメータ**（通常モード／広域モード）:
    - **通常モード**: メッシュ数 N=3、ズームレベル L=15。本 FR の標準動作。出力 per-mesh CSV/GeoJSON のファイル名は `<meshcode>.csv` / `<meshcode>_activation.geojson`
    - **広域モード**: メッシュ数 N=4/5/6、ズームレベル L=14。[FR-014](#fr-014-独立峰のコル探索) から「対象ピーク + N×N メッシュコードリスト」を指定して呼び出される。タイル結合時に 2×2 ピクセル max pooling でレベル 14 化する（別途タイル取得不要）。広域モードの出力 per-mesh CSV/GeoJSON は通常モードと区別できるファイル名で出力する（詳細は [FR-014](#fr-014-独立峰のコル探索) 参照）
    - 解析・ピーク検出・コル検出のロジック自体は通常モードと共通（[FR-005](#fr-005-ピーク候補検出)・[FR-006](#fr-006-コル検出プロミネンス計算)・[FR-007](#fr-007-プロミネンスフィルタper-mesh-csv-出力)）。広域モードは入力パラメータのみが異なる。広域モードは [FR-016](#fr-016-ピーク域ポリゴン生成) のポリゴン生成を呼び出さない（詳細は [FR-014](#fr-014-独立峰のコル探索) 参照）
  - 詳細は [`decisions/ADR-SRS-003-3x3-mesh-analysis.md`](decisions/ADR-SRS-003-3x3-mesh-analysis.md) を参照

#### FR-005: ピーク候補検出

- **対応 UR**: [UR-001](01_URD.md#ur-001), [UR-002](01_URD.md#ur-002)
- **概要**: 結合画像の全ピクセルを標高降順に走査し、8 近傍に処理済みピクセルが無いピクセルを新規ピーク候補とする。
- **入力**: 結合画像のピクセル群（FR-004 から）
- **出力**: ピーク候補リスト + 山塊グループ（Union-Find 構造。FR-006/007 に引き継ぎ）
- **詳細**:
  - 全ピクセルを標高降順にソートし、高い順に 1 ピクセルずつ処理する
  - 処理中のピクセルの 8 近傍に処理済みピクセルが 1 つもない場合、そのピクセルが新しい山塊グループの頂点（ピーク候補）となる
  - 処理済み隣接が 1 グループの場合はそのグループに合流し、2 グループ以上の場合はコル（[FR-006 参照](#fr-006-コル検出プロミネンス計算)）として各グループを統合する
  - Union-Find（連結成分管理アルゴリズム）でグループを管理する

#### FR-006: コル検出・プロミネンス計算

- **対応 UR**: [UR-002](01_URD.md#ur-002), [UR-005](01_URD.md#ur-005)
- **概要**: FR-005 の走査中に 2 つ以上の山塊グループが初接触するピクセルをコルとし、ピーク標高 − コル標高でプロミネンスを算出する。
- **入力**: ピーク候補と山塊グループ（FR-005 から）
- **出力**: 各ピークのコル座標・コル標高・プロミネンス・key_col_resolved フラグ
- **詳細**:
  - 各ピークに対してプロミネンスを規定するコルを検出する
  - [FR-005](#fr-005-ピーク候補検出) の処理中に 2 つ以上の山塊グループが初めて接触した時点のピクセルがコルであり、その標高がコル標高となる
  - プロミネンス = ピーク標高 − コル標高
  - コルが解析範囲外の場合、プロミネンスは暫定値とし、per-mesh CSV に `key_col_resolved=false` を付与する（[FR-014 参照](#fr-014-独立峰のコル探索)）

#### FR-016: ピーク域ポリゴン生成

- **対応 UR**: [UR-003](01_URD.md#ur-003), [UR-006](01_URD.md#ur-006)
- **概要**: 各ピークについてアクティベーションゾーン（標高差25m以内）と delete判定ゾーン（プロミネンスと 250m の小さい方）のポリゴンを Flood Fill で生成し、per-mesh GeoJSON として出力する。
- **入力**: ピーク座標・プロミネンス・コル標高（FR-005/006 から） + 結合画像
- **出力**: `$DATA_DIR/results/csv/<meshcode>_activation.geojson`（2種類のポリゴンを同一ファイルに収録）
- **詳細**:
  - ピーク候補検出・プロミネンス計算（[FR-005](#fr-005-ピーク候補検出)・[FR-006](#fr-006-コル検出プロミネンス計算)）完了後に、各ピークについて以下の2種類のポリゴンを GeoJSON として生成する
  - **共通仕様**:
    - **計算方法**: ピーク位置を起点として Flood Fill（隣接ピクセルを再帰的に広げる領域塗りつぶし）を実行し、条件を満たす連続ピクセルを抽出する。ピクセル群の外周輪郭を GeoJSON Polygon として出力する
    - **出力は lossless とする**: [FR-009](#fr-009-sota-リスト突合) の point-in-polygon 突合精度を確保するため、形状を変える簡略化（Douglas-Peucker 等）や等間隔での頂点間引きは行わない。直線上にある冗長な中間頂点の削除（ピクセル境界トレース結果で連続する collinear 点の除去）は形状を変えないため可とする
    - Flood Fill が解析対象メッシュ全体の地理的範囲内で完結している場合 `area_complete=true`、解析範囲外で途切れた場合 `area_complete=false` を付与する（false の場合、ポリゴンが実際より小さく計算されている可能性を示す）。同一ピークは複数の 3×3 メッシュ解析（中心メッシュ・隣接メッシュ）にまたがって検出されるため、[FR-018](#fr-018-per-mesh-activationgeojson-統合) で複数の per-mesh GeoJSON を統合する段階で `area_complete=true` のレコードが必ず見つかる想定（AZ は 25m 標高差以内、delete判定ゾーンは `delete_zone_max_drop` 上限キャップにより、いずれかの 3×3 解析で完結する。[ADR-SRS-011](decisions/ADR-SRS-011-delete-zone-polygon.md)）。FR-018 統合後も `area_complete=true` が見つからなかった場合は [FR-009](#fr-009-sotaリスト突合match_status-判定) の `is_area_incomplete` 不備フラグが true となり、後続処理を停止する
  - **アクティベーションゾーンポリゴン**（`feature_type="activation_zone"`）:
    - **定義**: SOTA ルールに従い、ピークから標高差 25m 以内の連続エリア
    - **Flood Fill 閾値**: `peak_elev - 25.0m` 以上
    - **area_complete の扱い**: 共通仕様の通り。標高差 25m 以内のため、いずれかの 3×3 解析で必ず完結する想定
  - **delete判定ゾーンポリゴン**（`feature_type="delete_zone"`）:
    - **定義**: 既存 SOTA サミットの削除判定に使用する。ピーク頂上から、プロミネンスと上限値（`delete_zone_max_drop` = 250m）のどちらか小さい方の標高差以内の連続エリア（[ADR-SRS-011](decisions/ADR-SRS-011-delete-zone-polygon.md)）
    - **Flood Fill 閾値**: Key コルの標高と「ピーク標高 − 250m」のどちらか高い方以上を対象として Flood Fill する。`key_col_resolved=false`（Key コルの標高が未確定）のピークでは「ピーク標高 − 250m」を下限として使用する（250m の上限キャップにより、プロミネンス未確定でもポリゴン生成が可能）
    - **パラメータ**: `delete_zone_max_drop` は `params/config.ini` で管理（値 250m、実測による確定値。詳細は [ADR-SRS-011](decisions/ADR-SRS-011-delete-zone-polygon.md)）
    - **area_complete の扱い**: 共通仕様の通り。`delete_zone_max_drop` 上限キャップにより、いずれかの 3×3 解析で必ず完結する想定
  - 詳細は [6.9 中間ファイル: メッシュ別ピーク域 GeoJSON](#69-中間ファイル-メッシュ別ピーク域-geojson) を参照

#### FR-007: プロミネンスフィルタ・per-mesh CSV 出力

- **対応 UR**: [UR-001](01_URD.md#ur-001), [UR-005](01_URD.md#ur-005)
- **概要**: 一次フィルタ（プロミネンス ≥ 130m）を適用してピーク・コル情報を per-mesh CSV に出力する。
- **入力**: ピーク・コル情報（FR-005/006 から）
- **出力**: `$DATA_DIR/results/csv/<meshcode>.csv`
- **詳細**:
  - フェーズ2 で検出したピーク候補とコルの情報を per-mesh CSV として出力する。フェーズ3（[FR-008](#fr-008-per-mesh-csv-統合)）での最終判定（プロミネンス ≥ 150m）に備えて、一次フィルタとしてプロミネンス ≥ 130m を超えたピーク候補のみを出力する
  - 一次フィルタ: プロミネンス ≥ 130m（最終判定はフェーズ3 で 150m）
  - **出力カラム**（ヘッダー行あり）:

| カラム | 型 | 精度 | 説明 |
|---|---|---|---|
| peak_lat | float | 小数点8桁 | ピーク緯度 |
| peak_lon | float | 小数点8桁 | ピーク経度 |
| peak_elev | float | 小数点2桁 | ピーク標高（m） |
| points | int | — | 標高バンドに基づくポイント数（1/2/4/6/8/10）。`peak_elev` を切り捨て（floor）した整数 m でバンド判定（[GLOSSARY 参照](00_GLOSSARY.md#標高バンドpoints-算出表)） |
| col_lat | float | 小数点8桁 | コル緯度（`key_col_resolved=false` の場合は 0.0） |
| col_lon | float | 小数点8桁 | コル経度（`key_col_resolved=false` の場合は 0.0） |
| col_elev | float | 小数点2桁 | コル標高（m） |
| prominence | float | 小数点2桁 | プロミネンス（m） |
| key_col_resolved | bool | true/false | コルが解析範囲内で確定済みの場合 true、解析範囲外で未発見の場合 false |
| col_margin_px | int | — | コルから解析範囲（結合画像）の端（4辺）までの最短距離（ピクセル単位）。小さいほど信頼性が低い |
| center_mesh | int | — | 解析中心メッシュコード（4桁） |

#### FR-015: 標高地形図出力

- **対応 UR**: [UR-001](01_URD.md#ur-001)（解析結果の目視確認補助）
- **概要**: 解析開始前に結合画像から色分け PNG を生成し、解析範囲を目視確認できるようにする。
- **入力**: 結合画像（FR-004 のタイル読み込み完了直後、解析処理開始前）
- **出力**: `$DATA_DIR/images/<meshcode>_terrain.png`
- **詳細**:
  - 出力形式: PNG（人間が視認しやすい配色で標高を色分けしたイメージ）
  - 解像度: 長辺 6000px に縮小（アスペクト比保持、最近傍サンプリング）（参照: [ADR-SRS-012](decisions/ADR-SRS-012-terrain-image-downscaling-method.md)）
  - NODATA は識別可能な色で表示する
  - `$DATA_DIR/images/` ディレクトリが存在しない場合は自動生成する
  - 詳細は [6.8 出力: 標高地形図](#68-出力-標高地形図terrain-rgb-png) を参照

---

### フェーズ3: ピーク統合（全国）

#### FR-008: per-mesh CSV 統合

- **対応 UR**: [UR-003](01_URD.md#ur-003)
- **概要**: per-mesh CSV を同一ピーク座標で重複排除し、プロミネンス最終フィルタ（≥150m）を適用した内部 work CSV を生成する。
- **入力**: `$DATA_DIR/results/csv/` 配下の per-mesh CSV（通常モード + 広域モード混在可）+ メッシュコードリスト（オプション。指定時はそのメッシュの CSV のみ読み込む）
- **出力**: `$DATA_DIR/results/merged_peak.csv`（内部 work CSV。[FR-009](#fr-009-sotaリスト突合match_status-判定) の入力として使用される中間ファイル。最終的な中心成果物は FR-009 が出力する `merged.geojson`）
- **詳細**:
  - **通常モード（フェーズ2）の per-mesh CSV** と **広域モード（[FR-014](#fr-014-独立峰のコル探索) から生成）の per-mesh CSV** が同一ディレクトリ配下に混在することがある。両方を読み込み、同一ピーク座標で重複排除する
  - 同一ピーク座標（ズームレベル15 タイル座標が一致）のレコードを同一ピークとして重複排除し、統合する
    - 複数解析のうち `(key_col_resolved=true, コル標高が最も高い)` レコードを代表に採用する（保守的評価）
    - 通常 per-mesh では `key_col_resolved=false` だったピークも、広域 per-mesh で `key_col_resolved=true` の結果が得られていれば、本ロジックにより広域モード結果が自動的に代表として採用される
    - `analysis_count`: 重複排除前の出現回数（実際の解析回数）
    - `expected_count`: メッシュコードリスト（オプション）をもとに算出する期待解析回数。リストが省略された場合は空欄
    - `stability`: `key_col_resolved=false` が 1 件でも含まれるか、`analysis_count ≠ expected_count` の場合 `unstable`、それ以外は `confirmed`
  - プロミネンス最終フィルタ: ≥ 150m（FR-007 の 130m フィルタ通過済みのレコードに適用）
  - **再入可能性**: 本機能は FR-014 のループから複数回呼び出され、その都度 merged_peak.csv（内部 work CSV）が再生成される

#### FR-018: per-mesh activation.geojson 統合

- **対応 UR**: [UR-003](01_URD.md#ur-003), [UR-006](01_URD.md#ur-006)
- **概要**: per-mesh GeoJSON を同一ピーク座標で統合し、`area_complete=true` を採用して中間 GeoJSON を生成する。
- **入力**: `$DATA_DIR/results/csv/<メッシュコード>_activation.geojson`（FR-016 出力。通常 per-mesh のみ）+ メッシュコードリスト（オプション。指定時はそのメッシュのファイルのみ読み込む）
- **出力**: `$DATA_DIR/results/merged_activation.geojson`（**内部中間ファイル**。[FR-009](#fr-009-sotaリスト突合match_status-判定) の point-in-polygon 突合に使用される中間ファイル。デバッグ・差分検査用として物理出力は残す。詳細: [ADR-SRS-013](decisions/ADR-SRS-013-merged-geojson-as-central-data.md)）
- **詳細**:
  - 通常 per-mesh の `<meshcode>_activation.geojson`（[FR-016](#fr-016-ピーク域ポリゴン生成) 出力）のみを統合対象とする。広域モード（[FR-014](#fr-014-独立峰のコル探索)）は GeoJSON を生成しないため、広域 per-mesh ファイルは本機能の入力に含まれない
  - 同一ピーク座標（ズームレベル15 タイル座標が一致）の Polygon のうち、`area_complete=true`（完全なポリゴン）のものを採用する
  - `area_complete=true` がどこにも存在しない場合は [FR-009](#fr-009-sotaリスト突合match_status-判定) の `is_area_incomplete` 不備フラグが true となり、後続の [FR-013](#fr-013-html-ビューア生成)（HTML ビューア生成）は実施されない（[ADR-SRS-011](decisions/ADR-SRS-011-delete-zone-polygon.md)）
  - delete判定ゾーンポリゴン（`feature_type="delete_zone"`）も同様に統合する。同一ピーク座標で複数ある場合は activation zone と同じ方針（`area_complete=true` のものを採用）で処理する
  - **再入可能性**: FR-014 のループ中は再入しない。広域モードは GeoJSON を生成しないため、本機能は FR-014 以前の1回のみ実行される

---

フェーズ3 の統合結果（内部 work CSV `merged_peak.csv`）で `key_col_resolved=false` が残ったピークを対象に、per-mesh 解析エンジンを
広域モード（[FR-004](#fr-004-33-メッシュ結合解析) の N×N + L14 パラメータ）で再呼び出しし、
Key コルを特定する。生成された広域 per-mesh CSV は通常 per-mesh CSV とともに
[FR-008](#fr-008-per-mesh-csv-統合) に再投入され、内部 work CSV `merged_peak.csv` の `key_col_resolved` ・`col_elev` ・`prominence` が更新される。
N=4 で解消しなければ N=5、N=6 とエスカレーションする（縮小版パイプラインのループ）。
広域モードでは [FR-016](#fr-016-ピーク域ポリゴン生成) のポリゴン生成は実行しない（広域再解析の目的は Key コル特定のみであり、ポリゴンは通常 per-mesh の結果を使用する。[ADR-SRS-011](decisions/ADR-SRS-011-delete-zone-polygon.md)）。
アクティベーションゾーンポリゴン・delete判定ゾーンポリゴンは通常 per-mesh で常に 3×3 内で完結する想定のため、`area_complete=false` は広域再解析のトリガー対象外とする。

#### FR-014: 独立峰のコル探索

- **対応 UR**: [UR-007](01_URD.md#ur-007)
- **概要**: merged_peak.csv の `key_col_resolved=false` ピークに対して、N×N メッシュ + L14 max pooling の広域モードで FR-004 ピーク解析を再呼び出しし、Key コルを特定する。フェーズ3（統合の直後・フェーズ4 突合の直前）に位置する。
- **入力**: [FR-008](#fr-008-per-mesh-csv-統合) 出力の内部 work CSV `$DATA_DIR/results/merged_peak.csv`
- **出力**: 広域 per-mesh CSV（`widearea_<peak>_<n>x<n>_<col>_<row>.csv`） → FR-008 再実行による更新済み merged_peak.csv（コル探索完了後の最終状態は [FR-009](#fr-009-sotaリスト突合match_status-判定) が merged.geojson として出力する）
- **詳細**:
  - **再解析トリガー**: `merged_peak.csv` の各ピークのうち `key_col_resolved=false`（コルが通常 per-mesh 3×3 解析範囲外 → プロミネンス未確定）のピークを対象とする
    - アクティベーションゾーン・delete判定ゾーンの `area_complete=false` はトリガー対象外（通常 per-mesh で 3×3 内に完結する想定であり、想定外発生時は [FR-009](#fr-009-sotaリスト突合match_status-判定) の `is_area_incomplete` 不備フラグで処理停止。[ADR-SRS-011](decisions/ADR-SRS-011-delete-zone-polygon.md)）
  - **基本フロー（縮小版パイプラインのループ）**:
    1. **対象ピーク特定**: 上記トリガー条件で `merged_peak.csv` から対象ピークを抽出する
    2. **エスカレーション・ループ（N = 4, 5, 6 の順）**:
       - 各対象ピークの緯度経度から、ピークが属するメッシュコードを算出
       - そのメッシュコードを含む N×N メッシュコードリストを生成（対象メッシュの位置は (1,1)〜(N,N) の N² 通り。後述の全パターン探索で順に試す）
       - **[FR-004](#fr-004-33-メッシュ結合解析) を「処理モード = N×N + L14」パラメータ付きで呼び出す**。per-mesh パイプライン（FR-004→[FR-005](#fr-005-ピーク候補検出)→[FR-006](#fr-006-コル検出プロミネンス計算)→[FR-007](#fr-007-プロミネンスフィルタper-mesh-csv-出力)）が広域モードで実行され、広域 per-mesh CSV が出力される（[FR-016](#fr-016-ピーク域ポリゴン生成) のポリゴン生成は実行しない。広域再解析の目的は Key コル特定のみであり、ポリゴンは通常 per-mesh の結果を使用する）
       - **対象ピーク絞り込み**: 広域モードでは、解析範囲内に検出される他のピークは出力せず、対象ピークの行だけを per-mesh CSV に出力する（merged 統合時のノイズを防ぐため）
       - **[FR-008](#fr-008-per-mesh-csv-統合) を再実行**: 通常 per-mesh CSV と広域 per-mesh CSV を**まとめて**入力として再統合し、`merged_peak.csv` を更新する
       - 更新後の `merged_peak.csv` で対象ピークの `key_col_resolved=false` が解消されていなければ、N+1 にエスカレーションして 2 を繰り返す
    3. **最終残存**: N=6 でも `key_col_resolved=false` のピークが残った場合、当該フラグ状態を維持したまま処理を継続する（[FR-009](#fr-009-sotaリスト突合match_status-判定) の `is_key_col_unresolved` 不備フラグが true となり、[FR-009](#fr-009-sotaリスト突合match_status-判定) が非ゼロ exit で終了する。[ADR-SRS-011](decisions/ADR-SRS-011-delete-zone-polygon.md)）
  - **解析ウィンドウ全パターン探索**: 各 N の段階で、対象メッシュを N×N ウィンドウ内 (1,1)〜(N,N) の各位置に置いた N² 通りのパターンを順に試す。`key_col_resolved=true` を得たパターンが見つかった時点で早期終了する（次のパターン・次の N へは進まない）。存在しないメッシュ（海上・日本国外等）を含むパターンはスキップする
  - **広域 per-mesh ファイル命名**（区別のため通常 per-mesh と異なる名前にする）:
    - CSV: `$DATA_DIR/results/csv/widearea_<対象peak識別>_<n>x<n>_<col>_<row>.csv`
    - 対象 peak 識別子は merged_peak.csv の行を一意に特定できる値（例: peak_lat と peak_lon を結合した文字列）を用いる
  - **実装方針（[ADR-SRS-010](decisions/ADR-SRS-010-cpp-opencv-migration.md) 移行後の C++ エンジン前提）**:
    - C++ エンジンに「処理モード（N, L）」入力を追加するだけで、`mesh` / `elevation` / `unionfind` / `analyze` / `mesh_analyze` モジュールを通常モードと共有する（広域モード専用のロジック実装は行わない）
    - 広域モードでは `activation` モジュール（[FR-016](#fr-016-ピーク域ポリゴン生成) のポリゴン生成）は呼び出さない（広域再解析の目的は Key コル特定のみ）
    - Python オーケストレーションの責務は: 対象ピーク特定・N×N メッシュコードリスト生成・処理モードパラメータ指定で C++ エンジン呼び出し・出力ファイル確認・[FR-008](#fr-008-per-mesh-csv-統合) 再呼び出し・エスカレーション判定のみ
  - **実装設計**（詳細は [`decisions/ADR-SRS-004`](decisions/ADR-SRS-004-level14-max-pooling-isolated-peaks.md) Consequences 参照）:
    - 座標変換: 各 256×256 L15 タイルを 128×128 に 2×2 max pooling し L14 combined image に書き込む（combined image を L15 で先に作ってから pooling しない）
    - col_margin_px: px 単位のまま per-mesh CSV に出力、`zoom_level` 列を追加して FR-008 の統合時に L15 相当値に換算
    - 1px ボーダー: 現行実装を踏襲（L14 での 1px 幅増加は FR-014 対象ピークへの影響なし、許容）
    - 257×257 オーバーラップ: 広域モードでは使用しない（cross-tile pooling は不要）
  - 詳細は [`decisions/ADR-SRS-004-level14-max-pooling-isolated-peaks.md`](decisions/ADR-SRS-004-level14-max-pooling-isolated-peaks.md) を参照

---

### フェーズ4: SOTA突合・中心成果物生成・HTML ビューア生成

#### FR-009: SOTAリスト突合・match_status 判定

- **対応 UR**: [UR-003](01_URD.md#ur-003)
- **概要**: merged_peak.csv（ピーク中心の内部 work CSV）と SOTA サミットリストを point-in-polygon 突合し、全 Point/Polygon/LineString フィーチャ・rationale プロパティ・不備フラグ metadata を含む merged.geojson（中心成果物）と merged_summit.xlsx（サミット中心の確認用 XLSX）を出力する。本 FR はデータ概念が「ピーク中心 → サミット中心」へ切り替わる節目である。
- **入力**:
  - **内部 work CSV**: フェーズ3 ([FR-008](#fr-008-per-mesh-csv-統合)) で統合され、[FR-014](#fr-014-独立峰のコル探索) の広域再解析でコル特定を経た `$DATA_DIR/results/merged_peak.csv`。全ピークが `key_col_resolved=true` かつ全ポリゴンが `area_complete=true` であることを前提とする（不備があれば不備フラグとして後続に引き継ぎ、本 FR を非ゼロ exit で終了する。[ADR-SRS-011](decisions/ADR-SRS-011-delete-zone-polygon.md)）
  - **merged_activation.geojson**: フェーズ3 ([FR-018](#fr-018-per-mesh-activationgeojson-統合)) で統合された `$DATA_DIR/results/merged_activation.geojson`。通常 per-mesh の GeoJSON のみから統合される（広域モードは GeoJSON を生成しない）
  - `ref/summitslist.csv`（JA プレフィックスサミット一覧）
  - メッシュコードリスト（オプション）: FR-008・FR-018 と同じリストを受け取る。省略時は全範囲を対象とする。FR-010 の削除候補スコープ判定に使用する
  - `ref/geojson_v{N}/`（ja0〜ja9 ファイル群）: 既存 SOTA サミットの日本語山岳名（`summit_name_jp`）取得用。バージョン番号 `{N}` は `params/config.ini` の `geojson_version` パラメータで指定する
- **出力**:
  - `$DATA_DIR/results/merged.geojson`（フェーズ4 末尾の中心成果物）。全 Point フィーチャ・全 Polygon フィーチャ・`rationale` プロパティ・不備フラグを含む metadata を持つ。フィーチャ構成の詳細は [FR-013](#fr-013-html-ビューア生成) のフィーチャ構成テーブルに記載
  - `$DATA_DIR/results/merged_summit.xlsx`（サミット一覧（突合後）。バッチ生成時点の中身確認用。カラム構成は [FR-012](#fr-012-サミット一覧申請内容反映版生成) と同じ）。ユーザーが HTML ビューアで編集した山岳名・rationale は反映しない
- **詳細**:
  - `ref/summitslist.csv` の JA プレフィックスサミットと突合する
  - geojson_v{N} の各フィーチャの `name` プロパティは `"JA/XX-NNN(山岳名)"` 形式。SOTAコードで突合し、括弧内の文字列を `summit_name_jp` として matched・delete サミットに付与する。geojsonに存在しないサミットは `summit_name_jp` を空文字とする
  - 突合は各ピークのアクティベーションゾーンポリゴンおよび delete判定ゾーンポリゴン（[FR-016](#fr-016-ピーク域ポリゴン生成) → [FR-018](#fr-018-per-mesh-activationgeojson-統合) で確定済み）を用いた point-in-polygon（点が多角形の内側にあるかを判定）で行う。判定は**座標のみ**で行い、SOTA 登録標高と DEM 標高の前後関係には依存しない（[ADR-SRS-011](decisions/ADR-SRS-011-delete-zone-polygon.md)）
  - マッチング一意性: プロミネンス ≥ 150m の制約により、1 つのアクティベーションゾーンポリゴン内に複数 SOTA サミットは数学的に存在しない（delete判定ゾーン内には縦走路上などで複数 SOTA サミットが含まれうるが、主ピーク特定アルゴリズム（[ADR-SRS-008](decisions/ADR-SRS-008-dominant-peak-identification.md)）で各サミットの主ピークが一意に決まる）
  - **市区町村判定**: 各ピーク（matched/new/dominant）および delete サミットの座標を `N03-{n03_year}_municipalities.geojson`（FR-017 生成・市区町村単位）と照合し、`municipality` カラム（例: "根室市"・"標津町"）を merged_summit.xlsx に付与する。市区町村ファイルが存在しない場合は空文字を付与して続行する（警告ログ出力）
  - **`peak.match_status` 判定（以下の順に評価）**（用語整理の経緯は [ADR-URD-007](decisions/ADR-URD-007-peak-match-status-terminology.md)、ポリゴン種別変更の経緯は [ADR-SRS-011](decisions/ADR-SRS-011-delete-zone-polygon.md) 参照）:
    1. ピークのアクティベーションゾーン内に既存 SOTA サミット座標が存在する → `matched`
    2. ピークの delete判定ゾーン内に既存 SOTA サミット座標が存在するが、アクティベーションゾーン外 → `dominant`（そのサミットが削除候補となり、このピークが主ピークとなる）
    3. いずれにも該当しない（delete判定ゾーン内にも既存サミットが存在しない）→ `new`（プロミネンス ≥ 150m を満たす新規申請候補）
  - **`summit.match_status` 判定**（peak.match_status とは独立した値。ピーク中心 → サミット中心へ視点が切り替わる基点。以下の順に評価し、AZ 内が最優先）:
    - `matched`: 既存 SOTA サミット座標がいずれかのピークのアクティベーションゾーン内に存在する（正常存続）
    - `delete`: 既存 SOTA サミット座標がいずれかのピークの delete判定ゾーン内かつアクティベーションゾーン外に存在する（削除候補）
    - `unmatched`: 既存 SOTA サミット座標がいずれのピークの AZ・delete判定ゾーンにも含まれない（[ADR-SRS-011](decisions/ADR-SRS-011-delete-zone-polygon.md)）。本来発生しないべき状態で、発生した場合は `delete_zone_max_drop` 値の不備または解析欠落を示すため、本 FR はログ警告を出力して**非ゼロ exit で処理を中止**する。後続の [FR-013](#fr-013-html-ビューア生成)（HTML ビューア生成）はスキップされる
  - **仮サミットコード割り当て**（`new` および `dominant` ピーク）:
    - match_status=new・dominant 両方のピークに、FR-017 で前処理した地域データを用いて仮サミットコードを付与する
    - フォーマット: `JAx/XX-A01`
      - `JAx`: SOTA アソシエーションコード（JA / JA5 / JA6 / JA8 のいずれか）
      - `XX`: SOTA エリアコード（例: TK = 東京都・島部、KS = 鹿児島県）
      - `A01`: 仮番号（`A` + 2桁連番、エリアごとに 01 からリセット）
    - 海上・地域不明ピーク（FR-017 フォールバック）: `ZZ/ZZ-A01` 形式
    - **採番順序**: 同一エリア（XX）内で次の優先順位でソートし、`A01` から順に採番する。
      1. 標高（`peak_elev`）降順（1次キー）
      2. プロミネンス降順（2次キー）
      3. `peak_lat` 降順（北→南、3次キー）
      4. `peak_lon` 昇順（西→東、4次キー）

      標高を 1 次キーとすることで「より高い山ほど若い番号」という直感的な序列を実現する。プロミネンスは標高同値時のタイブレーク、座標は更にタイブレークとして用いる。`new`/`dominant` の両カテゴリを区別せずに、同一エリア内で混在させて 1 つの順序列としてソートする。
    - **採番タイミング**: 本 FR の実行ごとに、入力 per-mesh CSV から全件再採番する。メッシュ範囲指定（[FR-010](#fr-010-削除候補のスコープ)）の有無に関わらず同じ規則を適用する。同一の入力 per-mesh CSV 集合からは常に同一の仮サミットコードが得られる（決定論性は [NFR-003](#nfr-003-再現性決定論的出力) が保証する）。
    - **連番上限**: 1 エリアあたり `A99` まで。`A99` を超える地域が発生した場合は、エラーメッセージ（地域コード・超過件数を含む）を出力して処理を中止する（exit code 非 0）。想定外件数の発生は、解析対象範囲やプロミネンス閾値の異常を示唆するため、自動で桁数を拡張せず人手判断を仰ぐ。
    - **以降、仮サミットコードを `JAx/XX-A01` と表記する**（JAx・XX は実際の値の例示、01 は連番の例示）
  - **stability 値**:
    - `confirmed`: 解析回数=期待値かつ `key_col_resolved=true`
    - `unstable`: `key_col_resolved=false` あり、または解析回数不一致
    - `-`: delete サミット（解析対象外のため）
  - **主ピーク特定**（[ADR-SRS-008](decisions/ADR-SRS-008-dominant-peak-identification.md)）:
    - 各 delete 候補サミット座標に対して、delete判定ゾーンポリゴン（`feature_type="delete_zone"`）内に
      その座標が含まれるピークを候補とする（point-in-polygon 判定）
    - 候補が複数の場合は**プロミネンスが最小のピーク**を主ピークとする（プロミネンスが最小のピークは親ピークへ最も早く合流する局所的な隆起であり、delete 候補サミットと同一山塊と見なせる）
    - いずれの delete判定ゾーンにも含まれないサミットは `summit.match_status="unmatched"` として上記のエラー停止が発動する（自動フォールバックは行わない）
    - 付与するカラム: `dominant_peak_code`（主ピークのサミットコード）、`dominant_peak_dist_m`（主ピークから delete 候補サミット座標までの距離 m。Haversine 公式で計算。人手確認用）
  - **rationale プロパティ生成**（各フィーチャの `rationale` プロパティに格納する申請書根拠テキスト。HTML ビューアで編集可能・FR-011 の XLSX 列 I に転記）:
    - **対象フィーチャ**: match_status が `new` / `dominant` のピーク Point、`matched_band_change`（`is_band_change_candidate=true`）の matched ピーク Point、match_status が `delete` の既存 SOTA サミット Point
    - **※2 追加根拠フォーマット**（new / dominant ピーク Point に付与）:
      ```
      国土地理院標高タイルで解析
      {peak_lat},{peak_lon}
      所在地：{都道府県または振興局名} {市区町村名}
      コル標高：{col_elev}m
      プロミネンス：{prominence}m
      ```
    - **※4 削除根拠フォーマット**（match_status=delete の既存 SOTA サミット Point に付与）:
      ```
      国土地理院標高タイルを解析し、{dominant_peak_code}に従属している事を確認
      ```
    - **※5 変更根拠フォーマット**（is_band_change_candidate=true の matched ピーク Point に付与）:
      ```
      国土地理院 DEM 解析による標高再測定: {sota_alt_m}m ({sota_points}pt) → {floor(peak_elev)}m ({peak_points}pt)
      座標: {peak_lat},{peak_lon}（{都道府県または振興局名} {市区町村名}）
      ```
    - `key_col_resolved=false` のピークは `col_elev`・`prominence` が確定していないため、※2 の該当箇所を「未確定」と表示する
    - rationale はビューア上の textarea で**編集可能**。編集後の値が FR-011 の XLSX 列 I に反映される（編集前は上記フォーマットの自動生成値が初期値）。永続化方式・編集値マージロジックの詳細は HLD 範疇
  - **不備フラグ**（merged.geojson の `metadata` プロパティに格納。[ADR-SRS-011](decisions/ADR-SRS-011-delete-zone-polygon.md)・[ADR-SRS-013](decisions/ADR-SRS-013-merged-geojson-as-central-data.md) により不備を集中管理し、後続処理（FR-013）への波及を防ぐ）:
    - `is_unmatched_summit` (bool): 既存サミット行で `summit.match_status="unmatched"` となった場合 true
    - `is_area_incomplete` (bool): ピーク行で AZ または delete判定ゾーンポリゴンの `area_complete=false`（[FR-016](#fr-016-ピーク域ポリゴン生成) で 3×3 完結が想定されているが、想定外に発生した場合に true）
    - `is_key_col_unresolved` (bool): ピーク行で `key_col_resolved=false`（[FR-014](#fr-014-独立峰のコル探索) 広域再解析でも解消せず）
    - `is_out_of_range_summit` (bool): 既存サミット座標が解析対象メッシュ範囲外（FR-010 のスコープ外）
    - `is_dem_invalid_summit` (bool): 既存サミット座標の DEM が NODATA / 海面
    - **exit code 制御**: 本 FR は処理終了時に上記いずれかの不備フラグが true の場合、**非ゼロ exit で終了**する。merged.geojson は不備フィーチャも含めて出力する（調査用）が、[FR-013](#fr-013-html-ビューア生成)（HTML ビューア生成）は exit code を見てスキップする
    - フラグの追加は実装中に随時行ってよい（網羅性が必要）。新規不備種別を発見した場合は本リストに追記する
  - **変更申請判定（Points バンド遷移）**:
    - 判定対象: `peak.match_status="matched"` のピーク
    - `peak_points = band(floor(peak_elev))`: `peak_elev` を切り捨て（floor）した整数 m でバンド判定
    - `sota_points = band(sota_alt_m)`: `sota_alt_m` は整数 m なので丸め不要
    - `is_band_change_candidate = (peak_points ≠ sota_points)`: true の場合、申請書エクスポートで「変更」行として自動出力
    - バンド定義は [`00_GLOSSARY.md` 標高バンド（Points 算出表）](00_GLOSSARY.md#標高バンドpoints-算出表) を参照

#### FR-010: 削除候補のスコープ

- **対応 UR**: [UR-003](01_URD.md#ur-003)
- **概要**: メッシュコードリストの地理的範囲内の SOTA サミットのみを削除候補対象とし、範囲外サミットの誤削除を防ぐ。
- **入力**: FR-009 に渡されたメッシュコードリスト（オプション）
- **出力**: 削除候補対象サミットの絞り込み結果（FR-009 内のフィルタとして機能）
- **詳細**:
  - メッシュコードリストが指定されている場合: 各メッシュコードから地理的範囲（緯度・経度の矩形）を算出し、その範囲内に座標がある SOTA サミットのみを削除候補の対象とする
  - メッシュコードリストが省略された場合: 全 SOTA サミットを削除候補の対象とする（全国フル解析を前提）
  - （解析対象外メッシュのサミットを誤って削除候補にしない）

#### FR-013: HTML ビューア生成

- **対応 UR**: [UR-006](01_URD.md#ur-006)
- **概要**: フェーズ4 で `merged.geojson`（[FR-009](#fr-009-sotaリスト突合match_status-判定) が生成した中心成果物）を入力として HTML ビューアを生成する。
- **入力**: `$DATA_DIR/results/merged.geojson`（FR-009 出力の中心成果物）
- **出力**:
  - `$DATA_DIR/results/merged_viewer.html`（静的 HTML ビューア）
- **詳細**:
  - **前提条件**: [FR-009](#fr-009-sotaリスト突合match_status-判定) が正常終了（exit code 0）した場合のみ実行する。[FR-009](#fr-009-sotaリスト突合match_status-判定) が非ゼロ exit（不備フラグが true）で終了した場合、本 FR は実行をスキップし、HTML は生成しない（[ADR-SRS-011](decisions/ADR-SRS-011-delete-zone-polygon.md)）
  - **GeoJSON メタデータ**: `merged.geojson` のトップレベルの `metadata` オブジェクト（FR-009 が生成）:
    - `summitslist_date`: `ref/summitslist.csv` 1行目（`SOTA Summits List (Date=DD/MM/YYYY)` 形式）からパースした日付文字列
    - `generated_at`: FR-009 実行時の ISO 8601 形式の日時文字列（パイプライン最終実行日時）
    - `attribution`: `"地理院タイル（標高タイル）を加工して作成。出典: 国土地理院"` （固定文字列。[UR-011](01_URD.md#ur-011)・[ADR-URD-014](decisions/ADR-URD-014-gsi-tile-attribution-policy.md) 準拠）
    - `source_url`: `"https://maps.gsi.go.jp/development/ichiran.html"` （固定文字列）
    - `license_url`: `"https://www.gsi.go.jp/kikakuchousei/kikakuchousei40182.html"` （固定文字列）
    - 不備フラグ群（`is_unmatched_summit` 等。詳細は [FR-009 参照](#fr-009-sotaリスト突合match_status-判定)）
  - **フィーチャ構成**（match_status 別）:

| match_status | フィーチャ |
|---|---|
| matched | Point（ピーク）+ Point（コル）+ Point（既存 SOTA サミット）+ Polygon（アクティベーションゾーン）+ LineString（ピーク → コル）+ LineString（ピーク → SOTA サミット） |
| new | Point（ピーク）+ Point（コル）+ Polygon（アクティベーションゾーン）+ Polygon（delete判定ゾーン）+ LineString（ピーク → コル） |
| dominant | Point（ピーク）+ Point（コル）+ Point（既存 SOTA サミット）+ Polygon（アクティベーションゾーン）+ Polygon（delete判定ゾーン）+ LineString（ピーク → コル）+ LineString（ピーク → SOTA サミット） |

##### 各フィーチャのプロパティ

**Point: ピーク**

| プロパティ名 | 説明 |
|---|---|
| `feature_type` | "peak" |
| `match_status` | matched / new / dominant |
| `summit_code` | サミットコード（matched のみ）または仮サミットコード（new / dominant。`JAx/XX-A01` 形式） |
| `summit_name` | サミット名（matched / dominant のみ・英語/ローマ字） |
| `summit_name_jp` | 日本語山岳名（matched / dominant のみ・geojson_v{N} から取得。未取得時は空文字） |
| `peak_elev` | 検出標高（m） |
| `prominence` | プロミネンス（m） |
| `stability` | confirmed / unstable |
| `key_col_resolved` | コル確定フラグ（true=確定 / false=未確定） |
| `points` | 標高バンドに基づくポイント数（1/2/4/6/8/10）。`peak_elev` から算出 |
| `rationale` | 申請書根拠テキスト（new / dominant は ※2 フォーマット、`is_band_change_candidate=true` の matched は ※5 フォーマット。FR-009 が自動生成。matched（バンド変更なし）は空文字）。HTML ビューアで編集可能 |

**Point: コル**

`key_col_resolved=false` の場合は含めない。

| プロパティ名 | 説明 |
|---|---|
| `feature_type` | "col" |
| `summit_code` | 対応ピークのサミットコード（ピーク Point との対応付け用） |
| `col_elev` | コル標高（m） |
| `points` | 対応ピークの `points` と同値（コル自身の標高ではなくピークの標高から算出） |

**Point: 既存 SOTA サミット**

| プロパティ名 | 説明 |
|---|---|
| `feature_type` | "summit" |
| `match_status` | matched / delete |
| `summit_code` | SOTA サミットコード |
| `summit_name` | サミット名（summitslist.csv の SummitName、英語/ローマ字） |
| `summit_name_jp` | 日本語山岳名（geojson_v{N} から取得。未取得時は空文字） |
| `sota_alt_m` | SOTA 登録標高（m） |
| `sota_points` | 標高バンドに基づくポイント数（1/2/4/6/8/10）。`sota_alt_m` から算出 |
| `rationale` | 申請書根拠テキスト（match_status=delete は ※4 フォーマット。FR-009 が自動生成。matched は空文字）。HTML ビューアで編集可能 |

**Polygon: アクティベーションゾーン**

| プロパティ名 | 説明 |
|---|---|
| `feature_type` | "activation_zone" |
| `summit_code` | 対応ピークのサミットコード（ピーク Point との対応付け用） |
| `area_complete` | true / false（アクティベーションゾーンが解析範囲内で完結している場合 true、解析範囲外で途切れた場合 false） |
| `points` | 対応ピークの `points` と同値（ビューアでの色付け用） |

**Polygon: delete判定ゾーン**

new / dominant のみ。FR-016 出力から取得（[ADR-SRS-011](decisions/ADR-SRS-011-delete-zone-polygon.md)）。

| プロパティ名 | 説明 |
|---|---|
| `feature_type` | "delete_zone" |
| `summit_code` | 対応ピークのサミットコード（ピーク Point との対応付け用） |

**LineString: ピーク → コル**

`key_col_resolved=false` の場合は生成しない。

| プロパティ名 | 説明 |
|---|---|
| `feature_type` | "prominence_range" |
| `summit_code` | 対応ピークのサミットコード（ピーク Point との対応付け用） |

**LineString: ピーク → SOTA サミット**

matched / dominant のみ。

| プロパティ名 | 説明 |
|---|---|
| `feature_type` | "coord_diff" |
| `summit_code` | 対応ピークのサミットコード（ピーク Point との対応付け用） |
| `match_status` | matched / dominant |

---

### フェーズ5: 申請書生成

フェーズ5 はバッチ処理ではなく、フェーズ4 で生成された編集可能なローカル HTML ビューア（`merged_viewer.html`）上でユーザーが行う操作で構成される。

#### FR-019: HTML ビューア機能仕様

- **対応 UR**: [UR-006](01_URD.md#ur-006)
- **概要**: FR-013 で生成された `merged_viewer.html` がブラウザ上で提供する機能（マップ表示・編集 UI・localStorage・エクスポート機能）の仕様を定義する。
- **入力**: `merged_viewer.html`（FR-013 出力）に埋め込まれた GeoJSON データ
- **出力**: ビューア上のユーザー操作に応じた表示・編集状態・各エクスポート（[FR-011](#fr-011-申請書-xlsx-生成)・[FR-012](#fr-012-サミット一覧申請内容反映版生成)・[FR-020](#fr-020-公開用-html-ビューア生成) 経由）
- **詳細**:
  - HTML テンプレートファイル（詳細は HLD）をソースコードに同梱する
  - フェーズ4（FR-013）で `merged.geojson` の GeoJSON データが JavaScript 変数として埋め込まれた `$DATA_DIR/results/merged_viewer.html` を生成する。本 FR はその HTML をブラウザで開いた際に提供される機能を定義する
  - 埋め込み方式を採用する理由: `file://` プロトコルで直接開いても CORS エラーが発生しないため、ローカル HTTP サーバが不要
  - **使用ライブラリ（CDN 経由）**: Leaflet（地図）・SheetJS/xlsx.js（XLSX エクスポート）・JSZip（ZIP 生成）
  - 背景タイル切り替え機能（国土地理院標準地図・国土地理院淡色地図・OSM・OpenTopoMap）を持つ（選定経緯: [ADR-SRS-006](decisions/ADR-SRS-006-viewer-background-tile-selection.md)）
  - 各マーカーの色は `points` プロパティに基づく標高バンド色（1pt=濃緑〜10pt=赤）を使用する
  - 未確定フラグ付きのアクティベーションゾーンは警告色で表示する
  - delete判定ゾーンポリゴン（new / dominant）を独立したトグルレイヤーとして追加（デフォルト ON・半透明）。new は delete判定ゾーン内に既存サミットが存在しないことを、dominant は delete判定ゾーン内に削除候補サミットが存在することを可視化する
  - ローカル（`file://` 直接開く）・GitHub Pages（静的ホスティング）の両方で動作する
  - **サミット検索機能**:
    - ヘッダーに検索ボックスを設ける
    - 検索対象: サミットコード（仮コード含む）・山岳名（日本語）・山岳名（アルファベット）の部分一致（大文字小文字無視）
    - 検索対象: 緯度経度（例: `35.68, 139.76` または `35.68 139.76` 形式）による直接座標ジャンプ
    - 入力に応じてサジェスト候補（最大10件、先頭一致優先）をリアルタイム表示する
    - 候補クリックまたは Enter 確定で該当サミットへ地図ズーム移動しポップアップを開く
    - 確定時、当該サミットが現在フィルターで非表示の場合は対象カテゴリのフィルターを自動的に ON にする
  - 埋め込みデータの `metadata` から以下の情報を画面上に表示する:
    - SOTA サミットリスト基準日（`summitslist_date`）（UTC）
    - 地理院タイル更新日（提供元）（`gsi_tile_latest_date`）（UTC）: パイプラインがキャッシュタイルの mtime 最大値として `merged.geojson` の `metadata` に格納する（生成実装は別 ISSUE 管理）。表示時は `(UTC)` を付記する
    - 解析実行日時（`generated_at`）
  - 地図帰属表示: Leaflet の attribution に `© 国土地理院`・`© OpenStreetMap contributors`・`© OpenTopoMap contributors` を必ず含める
  - **山岳名入力 UI**:
    - new（新規）ピーク: クリックで開くポップアップまたはサイドパネルに「山岳名JP」「山岳名EN」入力フィールドを表示
    - matched（既存）ピーク: 入力フィールド不要（名称変更は申請対象外。`is_band_change_candidate=true` の場合は申請書エクスポート時に自動的に「変更」行を出力する）
    - dominant（削除候補ピーク）: 入力フィールド不要（GeoJSON データを使用）
  - **rationale 編集 UI**:
    - new / dominant ピーク・match_status=delete サミット・`is_band_change_candidate=true` の matched ピーク: ポップアップまたはサイドパネルに `rationale` プロパティを表示する textarea を設ける
    - 初期値: merged.geojson の `rationale` プロパティ（FR-009 が自動生成したテンプレート文字列）
    - ユーザーが textarea を編集した場合、その内容が申請書 XLSX（[FR-011](#fr-011-申請書-xlsx-生成)）の列 I に反映される
    - 未編集の場合は初期値（自動生成テンプレート）がそのまま使用される
    - matched（バンド変更なし）: rationale 表示不要（XLSX 列 I は空白）
  - **入力内容の保持（localStorage）**:
    - 入力した山岳名・rationale 編集内容・名称修正はブラウザの localStorage に保存し、再訪時も維持する
    - キー: 埋め込みデータの `metadata.generated_at` を含む文字列
    - 新しいパイプライン実行で `generated_at` が変わった場合、前回の入力が残っていれば「前回の入力内容が残っています（解析日時: XXX）。引き継ぎますか？」と警告・選択を促す
  - **エクスポート機能**（エクスポートアイコン展開メニューに 3 ボタンを配置）:
    - **「申請書」ボタン**（[FR-011](#fr-011-申請書-xlsx-生成) 準拠）: SheetJS を使い申請書 XLSX を**単独**ブラウザダウンロードする
      - 出力行: `追加`（GeoJSON の new・dominant ピーク + 入力山岳名。仮サミットコードを山岳IDとして使用）・`削除`（GeoJSON の delete サミット、すなわち summit feature の `match_status="delete"`）・`変更`（`is_band_change_candidate=true` の matched ピーク。列構成は [FR-011 参照](#fr-011-申請書-xlsx-生成)）
      - XLSX 列 I（根拠）: 各フィーチャの `rationale` プロパティ値（編集済みの場合は編集後の値、未編集の場合は自動生成値）を転記する（詳細は [FR-011 参照](#fr-011-申請書-xlsx-生成)）
    - **「申請エビデンス」ボタン**（[FR-021](#fr-021-申請エビデンス-zip-生成) 準拠）: JSZip を使い申請エビデンス ZIP をブラウザダウンロードする。詳細は [FR-021 参照](#fr-021-申請エビデンス-zip-生成)
    - **「公開用 HTML」ボタン**（[FR-020](#fr-020-公開用-html-ビューア生成) 準拠）: localStorage の入力内容（山岳名JP/EN・rationale 編集値・名称修正）を埋め込みデータにマージした **閲覧専用 HTML** を**単独**ブラウザダウンロードする。詳細は [FR-020 参照](#fr-020-公開用-html-ビューア生成)

---

#### FR-011: 申請書 XLSX 生成

- **対応 UR**: [UR-004](01_URD.md#ur-004)
- **概要**: 申請書 XLSX は **HTML ビューア（FR-013）がブラウザ内で生成・ダウンロード**する。Python バッチは XLSX を生成しない。
- **入力**: merged.geojson 埋め込みデータ + HTML ビューア上のユーザー入力（山岳名・rationale 編集値）
- **出力**: 申請書 XLSX（ブラウザダウンロード。テンプレート列 A〜J 構成）
- **詳細**:
  - テンプレート列構成（`ref/SOTA-Summit-list-revision-request.xlsx` 準拠）:
    - 1シート構成
    - カラム: 既存山岳ID または仮サミットコード / アクション / 変更前（山岳名JP・EN・標高m） / 変更後（山岳名JP・EN・標高m） / 変更の根拠 / MT使用欄
  - アクション別カラムマッピング（テンプレート列 A〜J）:

| アクション | A: 山岳ID/仮サミットコード | B: アクション | C: 変更前 名JP | D: 変更前 名EN | E: 変更前 標高 | F: 変更後 名JP | G: 変更後 名EN | H: 変更後 標高 | I: 根拠 |
|---|---|---|---|---|---|---|---|---|---|
| 追加（new） | summit_code（仮サミットコード）| 追加 | 空白 | 空白 | 空白 | 山岳名JP ※1 | 山岳名EN ※1 | peak_elev | ※2 |
| 追加（dominant）| summit_code（仮サミットコード）| 追加 | 空白 | 空白 | 空白 | 山岳名JP ※1 | 山岳名EN ※1 | peak_elev | ※2 |
| 削除 | SummitCode | 削除 | summit_name_jp ※3 | summit_name | sota_alt_m | 空白 | 空白 | 空白 | ※4 |
| 変更 | SummitCode | 変更 | summit_name_jp | summit_name | sota_alt_m | summit_name_jp（同値） | summit_name（同値） | floor(peak_elev) | ※5 |

  - **※1**: HTML ビューアの入力フィールドで記入する（[FR-013 参照](#fr-013-html-ビューア生成)）
  - **※2**: FR-009 が自動生成する `rationale` プロパティ値（追加根拠）をそのまま転記する。HTML ビューアで編集した場合は編集後の値を使用する。フォーマット定義は [FR-009 参照](#fr-009-sotaリスト突合match_status-判定)
  - **※3**: summit_name_jp（geojson_v{N} から自動取得）。空文字の場合はビューアの入力フィールドで記入すること
  - **※4**: FR-009 が自動生成する `rationale` プロパティ値（削除根拠）をそのまま転記する。HTML ビューアで編集した場合は編集後の値を使用する。フォーマット定義は [FR-009 参照](#fr-009-sotaリスト突合match_status-判定)
  - **※5**: FR-009 が自動生成する `rationale` プロパティ値（変更根拠）をそのまま転記する。HTML ビューアで編集した場合は編集後の値を使用する。フォーマット定義は [FR-009 参照](#fr-009-sotaリスト突合match_status-判定)

#### FR-012: サミット一覧（申請内容反映版）生成

- **対応 UR**: [UR-005](01_URD.md#ur-005), [UR-011](01_URD.md#ur-011)
- **概要**: `merged.geojson`（[FR-009](#fr-009-sotaリスト突合match_status-判定) 出力の中心成果物）から、FR-019 でユーザーが編集した山岳名・rationale を反映したサミット一覧（申請内容反映版）を **HTML ビューア（FR-019）内でブラウザ生成**する。生成した XLSX は申請エビデンス ZIP（[FR-021](#fr-021-申請エビデンス-zip-生成)）に同梱してダウンロードする（UR-005 対応）。
- **入力**: ローカル HTML ビューアに埋め込まれた GeoJSON データ + localStorage の編集内容（[FR-019](#fr-019-html-ビューア機能仕様) が管理）
- **出力**: `merged_summit_revised.xlsx`（HTML ビューアからブラウザダウンロード。申請エビデンス ZIP 内に同梱）
- **詳細**:
  - FR-009 出力の `merged_summit.xlsx`（サミット一覧（突合後）・バッチ生成時点）とは異なり、ユーザーが HTML ビューアで入力した山岳名・rationale 編集内容を反映する（フェーズ5 で生成）
  - **Point フィーチャのみが行に変換される**（Polygon / LineString フィーチャは含めない）
  - `rationale` プロパティは含めない（申請書根拠テキストは HTML ビューアで確認・編集し XLSX に直接反映する。サミット一覧（申請内容反映版）は座標・標高・突合結果のみを記録する）
  - **出典シート**: XLSX の最後に「出典」シートを設け、「地理院タイル（標高タイル）を加工して作成。出典: 国土地理院 (https://maps.gsi.go.jp/development/ichiran.html)」を記載する（[UR-011](01_URD.md#ur-011)・[ADR-URD-014](decisions/ADR-URD-014-gsi-tile-attribution-policy.md) 準拠）
  - **出力カラム**:

| カラム | 説明 |
|---|---|
| match_status | SOTAリスト突合結果（matched/new/dominant） |
| stability | 解析品質（confirmed/unstable/-） |
| summit_code | サミットコード（例: JA/TK-001）。matched の場合は正式コード、new / dominant の場合は仮サミットコード（例: JAx/XX-A01）または ZZ/ZZ-A01（海上・未判定） |
| summit_name | サミット名（SOTA リストから・英語/ローマ字） |
| summit_name_jp | 日本語山岳名（geojson_v{N} から取得。未取得時は空文字） |
| peak_lat | ピーク緯度 |
| peak_lon | ピーク経度 |
| peak_elev | ピーク標高（m） |
| points | 標高バンドに基づくポイント数（1/2/4/6/8/10）。`peak_elev` を切り捨て（floor）した整数 m でバンド判定（[GLOSSARY 参照](00_GLOSSARY.md#標高バンドpoints-算出表)） |
| col_lat | コル緯度 |
| col_lon | コル経度 |
| col_elev | コル標高（m） |
| prominence | プロミネンス（m） |
| key_col_resolved | コル確定フラグ（true=確定 / false=未確定、[FR-006 参照](#fr-006-コル検出プロミネンス計算)） |
| col_margin_px | コルのメッシュ端マージン（px） |
| analysis_count | このピークが含まれた解析回数 |
| expected_count | このピークが含まれるべき期待解析回数 |
| sota_lat | SOTA リスト登録緯度（matched・dominant のみ。new は空欄） |
| sota_lon | SOTA リスト登録経度（matched・dominant のみ。new は空欄） |
| sota_alt_m | SOTA リスト登録標高（m）。matched・dominant のみ。new は空欄 |
| sota_points | 標高バンドに基づくポイント数（1/2/4/6/8/10）。`sota_alt_m`（整数 m）でバンド判定（[GLOSSARY 参照](00_GLOSSARY.md#標高バンドpoints-算出表)）。matched・dominant のみ。new は空欄 |
| is_band_change_candidate | Points バンド遷移フラグ（bool）。`points ≠ sota_points` の場合 true（変更申請対象）。matched のみ。それ以外は空欄 |
| municipality | 市区町村名（例: "根室市"・"標津町"）。N03-{n03_year}_municipalities.geojson 未存在時は空文字 |
| dominant_peak_code | 従属ピークコード（dominant のみ） |
| dominant_peak_dist_m | 主ピークから削除候補サミット座標までの距離 m（Haversine 公式）。dominant のみ |

---

#### FR-020: 公開用 HTML ビューア生成

- **対応 UR**: [UR-006](01_URD.md#ur-006)
- **概要**: ローカル HTML ビューア（FR-013）上の「公開用 HTML」ボタンで、外部公開用の閲覧専用 HTML を生成し**単独で**ブラウザダウンロードする。
- **入力**: ローカル HTML ビューアに埋め込まれた GeoJSON データ + localStorage の編集内容（[FR-019](#fr-019-html-ビューア機能仕様) が管理）
- **出力**: 公開用 HTML ファイル（ブラウザダウンロード経由）
- **詳細**:
  - 公開用は閲覧専用（編集 UI なし・XLSX エクスポートなし・localStorage 不使用）
  - localStorage の入力内容（山岳名JP/EN・rationale 編集値・名称修正）を埋め込みデータにマージしたうえで、公開用テンプレートに GeoJSON データを埋め込み、自己完結型 HTML を生成する
  - ユーザーはダウンロードした HTML を GitHub Pages 等の静的ホスティングに配置することで外部公開できる
  - 公開用 HTML に使用する別テンプレート（閲覧専用）をソースコードに同梱する（詳細は HLD）
  - 詳細仕様は [6.13 出力: 公開用 HTML](#613-出力-公開用-html閲覧専用ブラウザダウンロード) を参照

---

#### FR-021: 申請エビデンス ZIP 生成

- **対応 UR**: [UR-005](01_URD.md#ur-005)
- **概要**: ローカル HTML ビューア（FR-013）上の「申請エビデンス」ボタンで、サミット一覧（申請内容反映版）XLSX と 4 カテゴリ GeoJSON を 1 つの ZIP にまとめてブラウザダウンロードする。
- **入力**: ローカル HTML ビューアに埋め込まれた GeoJSON データ + localStorage の編集内容（[FR-019](#fr-019-html-ビューア機能仕様) が管理）
- **出力**: 申請エビデンス ZIP（ブラウザダウンロード経由）
- **詳細**:
  - ZIP に同梱するファイル一覧:

| ファイル名 | 内容 | 判定ロジック |
|---|---|---|
| `merged_summit_revised.xlsx` | サミット一覧（申請内容反映版）（[FR-012](#fr-012-サミット一覧申請内容反映版生成) 準拠） | — |
| `new.geojson` | 新規ピーク候補 | peak の `match_status="new"` |
| `dominant.geojson` | 削除候補ピークおよびその従属サミット | peak の `match_status="dominant"` ＋ 対応する `match_status="delete"` の summit フィーチャ |
| `changed.geojson` | ポイントバンド変更候補 | peak の `match_status="matched"` かつ `is_band_change_candidate=true` ＋ 対応する summit フィーチャ |
| `unchanged.geojson` | 変更なし既存サミット | peak の `match_status="matched"` かつ `is_band_change_candidate=false` ＋ 対応する summit フィーチャ |

  - 各 GeoJSON には `metadata`（`summitslist_date` / `gsi_tile_latest_date` / `generated_at` / `attribution` / `source_url` / `license_url`）を複製する（`attribution` 等は [UR-011](01_URD.md#ur-011) 準拠の固定値。詳細は [FR-013 メタデータ定義](#fr-013-html-ビューア生成) を参照）
  - 各 GeoJSON の関連フィーチャ（col / activation_zone / delete_zone / prominence_range / coord_diff）は同一 `summit_code` で紐付けて同梱する
  - GeoJSON の生成は localStorage の編集内容（山岳名JP/EN・rationale 編集値）を埋め込みデータにマージしたうえで行う
  - JSZip ライブラリを使用して ZIP をブラウザ内で生成する

---

## 5. 非機能要件

### NFR-001: 精度（プロミネンス判定）

- **対応 UR**: [UR-002](01_URD.md#ur-002)
- プロミネンス ≥ 150m のピークを出力すること
- 一次フィルタは 130m（解析範囲境界付近でコルが範囲外に出る場合、プロミネンスが過小評価される可能性があるため、20m のマージンを設けている）
- 最終 150m 判定は FR-008（フェーズ3）で実施
- 使用する標高データは DEM5（5m解像度）。地理院地図が優先表示する DEM1（1m解像度）より解像度は低く、標高値が数m程度異なることがある。ただしプロミネンス 150m 判定への実質的な影響は軽微である（参照: [ADR-SRS-004](decisions/ADR-SRS-004-level14-max-pooling-isolated-peaks.md)）

### NFR-002: メモリ使用量

- **対応 UR**: [UR-001](01_URD.md#ur-001), [UR-002](01_URD.md#ur-002), [UR-003](01_URD.md#ur-003), [UR-007](01_URD.md#ur-007)
- 本ツールの動作前提マシン（物理メモリ 62.72GB + スワップ 39.7GB、詳細は [environment.md](environment.md)）で動作すること
- 実測値（9メッシュ最大解析時）: 解析ピーク時 90.8%（≒56.9GB）、スワップ使用あり・完走確認済み

### NFR-003: 再現性（決定論的出力）

- **対応 UR**: [UR-004](01_URD.md#ur-004), [UR-005](01_URD.md#ur-005)
- 同一タイルキャッシュ・同一パラメータで実行した場合、すべての出力（CSV・GeoJSON・XLSX）が同一の内容になること（`metadata.generated_at` 等の実行時タイムスタンプを除く。ピーク座標・標高・突合結果・採番が同一であることを保証する）
- ピークのソート順を決定論化するため、標高降順ソートに 2次キー（x→y 座標）を設ける
- 仮サミットコードの採番順序は [FR-009](#fr-009-sotaリスト突合match_status-判定) で規定する（標高降順 → プロミネンス降順 → `peak_lat` 降順 → `peak_lon` 昇順）。同一の入力 per-mesh CSV 集合からは常に同一の仮サミットコードが得られる

### NFR-004: アクセスマナー

- **対応 UR**: [UR-010](01_URD.md#ur-010)
- タイル取得時に User-Agent（HTTPリクエストでサーバー側にアクセス元を伝えるための識別文字列）を必ず付与すること。地理院側で問題発生時に連絡が取れるよう、メールアドレスを含む形式とする
  - 形式: `findsummits/<バージョン番号> (mailto:<メールアドレス>)`（詳細は HLD）
- リクエスト間隔: パラメータファイルで指定（デフォルト: 100ms、詳細は HLD）
- HTTP 429/503（サーバーが「アクセスが多すぎる」と返すエラー）受信時は待機時間を段階的に延ばしながらリトライする

### NFR-005: 処理時間目標

- **対応 UR**: [UR-001](01_URD.md#ur-001)
- タイル取得: 全176メッシュで12時間以内（約4分/メッシュ）を目標とする
- 解析: 全176メッシュで24時間以内（約8分/メッシュ）を目標とする
- 参考: 1メッシュ当たり数分〜十数分（メッシュの地形・解析範囲による）

### NFR-006: 可搬性・環境

- **対応 UR**: [UR-008](01_URD.md#ur-008)
- [environment.md](environment.md) に定めるマシンスペック（物理メモリ 62.72GB 等）と同等以上の環境で動作すること
- サーバー構成・マルチユーザー運用は対象外

### NFR-007: ログ出力

- **対応 UR**: [UR-008](01_URD.md#ur-008)
- 本ツールのすべてのバッチ処理（タイル取得・行政区域前処理・解析・CSV統合・出力生成）においてログを出力すること
- 各処理のログに必ず含めること:
  - 処理開始時刻・終了時刻・所要時間
  - 処理対象（メッシュコード・ファイル名等）
  - 処理結果サマリ（成功件数・失敗件数・スキップ件数等）
  - エラー・警告が発生した場合はその内容と理由
- 長時間処理（タイル取得・解析）では途中経過（進捗）も記録すること
- 出力先: stdout およびパラメータファイル（`params/config.ini`）で設定したログディレクトリに同時出力すること
- ログディレクトリが存在しない場合は自動生成すること
- ログファイル命名規則・ログディレクトリの設定キー名の詳細は HLD で定める

### NFR-008: UI レスポンス

- **対応 UR**: [UR-006](01_URD.md#ur-006)
- HTML ビューア（FR-013）上のボタンクリック・地図操作などの UI 操作に対して、1秒以内に応答すること
- XLSX エクスポート: 5秒以内に完了すること
- 公開用 HTML エクスポート・GeoJSON ダウンロード: 10秒以内に完了すること

---

## 6. 外部インターフェース仕様

### 6.1 入力: 地理院標高タイル

| 項目 | 仕様 |
|---|---|
| 形式 | PNG（RGB エンコード） |
| ズームレベル | DEM5a/5b/5c: 15 / DEM10b: 14 |
| タイルサイズ | 256×256 px |
| 標高算出式 | `elev = (R×65536 + G×256 + B) / 100.0`（RGB ピクセル値から標高（m）を計算） |
| 無効値（NODATA）の定義 | R=128, G=0, B=0 のピクセルは標高値なし（海・データ未整備等）を示し、-9999.0m として扱う |
| タイル URL | ベース: `https://cyberjapandata.gsi.go.jp/xyz/{サービス名}/{z}/{x}/{y}.png`<br>サービス名: DEM5a=`dem5a_png` / DEM5b=`dem5b_png` / DEM5c=`dem5c_png` / DEM10b=`dem_png` |
| キャッシュ保存先 | `$DATA_DIR/tiles/{z}/{x}/{y}_{suffix}.png`<br>`{suffix}`: `a`=DEM5a / `b`=DEM5b・DEM10b / `c`=DEM5c（DEM5b と DEM10b はズームレベル 15/14 で区別） |

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
| 生成方式 | HTML ビューア（FR-013）の「申請書エクスポート」ボタンによるブラウザダウンロード（Python バッチは XLSX を生成しない） |
| カラム構成・アクション | [FR-011 参照](#fr-011-申請書-xlsx-生成) |

### 6.4 出力: サミット一覧（申請内容反映版）

| 項目 | 仕様 |
|---|---|
| ファイル | `merged_summit_revised.xlsx`（申請エビデンス ZIP 内に同梱。ブラウザダウンロード） |
| 生成元 | HTML ビューア（[FR-012](#fr-012-サミット一覧申請内容反映版生成) が merged.geojson の Point フィーチャからブラウザ内で生成。FR-019 でのユーザー編集内容を反映） |
| フォーマット | XLSX（単一シート・データ表） |
| 含む情報 | Point フィーチャの属性のみ（Polygon / LineString は除外。`rationale` 列は含めない） |
| カラム | [FR-012 参照](#fr-012-サミット一覧申請内容反映版生成) |

### 6.5 出力: サミット一覧（突合後）

| 項目 | 仕様 |
|---|---|
| ファイル | `$DATA_DIR/results/merged_summit.xlsx` |
| 生成 | フェーズ4 末尾（[FR-009](#fr-009-sotaリスト突合match_status-判定) が merged.geojson と同時に生成） |
| 用途 | バッチ生成時点（ユーザー編集前）のサミット一覧を確認するための XLSX。[サミット一覧（申請内容反映版）](#64-出力-サミット一覧申請内容反映版) はユーザー編集内容を反映した版 |
| フォーマット | XLSX（単一シート・データ表） |
| 含む情報 | Point フィーチャの属性のみ（Polygon / LineString は除外。`rationale` 列は含めない） |
| カラム | [FR-012 参照](#fr-012-サミット一覧申請内容反映版生成)（カラム構成は同一） |

### 6.6 出力: GeoJSON・作業用 HTML ビューア

**GeoJSON**（中心成果物）

| 項目 | 仕様 |
|---|---|
| 生成 | フェーズ4 末尾（[FR-009](#fr-009-sotaリスト突合match_status-判定) が生成する中心成果物。同時に `merged_summit.xlsx`（[サミット一覧（突合後）](#65-出力-サミット一覧突合後)）も生成） |
| ファイル | `$DATA_DIR/results/merged.geojson` |
| 座標参照系 | WGS84（EPSG:4326） |
| メタデータ | トップレベルに `metadata` オブジェクト（`summitslist_date`: サミットリスト基準日、`generated_at`: パイプライン実行日時 ISO 8601 形式、例: `"2026-05-13T14:30:00+09:00"`、不備フラグ群）を付与 |
| フィーチャ構成 | [FR-013 参照](#fr-013-html-ビューア生成)（Point / Polygon / LineString 全フィーチャ含む） |
| `rationale` プロパティ | new / dominant ピーク Point に ※2 フォーマット、match_status=delete サミット Point に ※4 フォーマット、`is_band_change_candidate=true` の matched ピーク Point に ※5 フォーマットで付与。HTML ビューアで編集可能。フォーマット定義は [FR-009 参照](#fr-009-sotaリスト突合match_status-判定) |

**作業用 HTML ビューア**（`merged_viewer.html`）

画面サンプル: [`docs/mockup/viewer_mockup.html`](mockup/viewer_mockup.html)

| 項目 | 仕様 |
|---|---|
| ファイル | `$DATA_DIR/results/merged_viewer.html` |
| 用途 | 山岳名入力・目視確認・申請書 / 申請エビデンス / 公開用 HTML のエクスポートを行うローカル作業用ビューア |
| HTML テンプレート | 作業用テンプレートファイル（詳細は HLD） |
| 使用ライブラリ | Leaflet（地図・CDN 経由）・SheetJS/xlsx.js（XLSX エクスポート・CDN 経由）・JSZip（ZIP 生成・CDN 経由） |
| 背景タイル | 国土地理院標準地図・国土地理院淡色地図・OSM・OpenTopoMap（切り替え可能） |
| GeoJSON 参照方式 | HTML 内に JavaScript 変数として埋め込み（外部ファイル参照なし） |
| 動作環境 | `file://` で直接開くだけで動作（HTTP サーバ不要） |

### 6.7 中間ファイル: メッシュ別ピーク候補 CSV

| 項目 | 仕様 |
|---|---|
| 役割 | C 解析エンジン（findsummits）がメッシュごとに出力するピーク候補の中間ファイル。[FR-008](#fr-008-per-mesh-csv-統合)（Python 統合処理）がこれを読み込んで全国分を統合する |
| ファイル | `$DATA_DIR/results/csv/<meshcode>.csv`（`<meshcode>` は4桁の1次メッシュコード） |
| エンコーディング | UTF-8 |
| カラム | [FR-007 参照](#fr-007-プロミネンスフィルタper-mesh-csv-出力) |

### 6.8 出力: 標高地形図（Terrain-RGB PNG）

| 項目 | 仕様 |
|---|---|
| 用途 | 解析範囲の標高データが正しく読み込まれているかを目視確認するための画像 |
| ファイル | `$DATA_DIR/images/<meshcode>_terrain.png` |
| 形式 | PNG |
| 解像度 | 長辺 6000px に縮小（アスペクト比保持、最近傍サンプリング）（参照: [ADR-SRS-012](decisions/ADR-SRS-012-terrain-image-downscaling-method.md)）|
| 色分け | 標高に応じたグラデーション（詳細は HLD） |
| 生成タイミング | `findsummits` 実行時（標高タイル読み込み完了後・解析開始前） |

### 6.9 中間ファイル: メッシュ別ピーク域 GeoJSON

| 項目 | 仕様 |
|---|---|
| 役割 | C 解析エンジン（findsummits）がメッシュごとに出力するアクティベーションゾーンポリゴンおよび delete判定ゾーンポリゴンの中間ファイル。メッシュ別 CSV（6.7）と同タイミングで生成され、[FR-018](#fr-018-per-mesh-activationgeojson-統合)（Python 統合処理）が全国分を統合する |
| ファイル | `$DATA_DIR/results/csv/<meshcode>_activation.geojson`（`<meshcode>` は4桁の1次メッシュコード） |
| 形式 | GeoJSON（RFC 7946） |
| 座標参照系 | WGS84（EPSG:4326） |
| フィーチャタイプ | Polygon（2種）: アクティベーションゾーン（`feature_type="activation_zone"`）・delete判定ゾーン（`feature_type="delete_zone"`） |
| プロパティ（アクティベーションゾーン） | `feature_type="activation_zone"`, `peak_lat`, `peak_lon`, `peak_elev`, `area_complete`（解析範囲内で完結している場合 true、境界で途切れた場合 false） |
| プロパティ（delete判定ゾーン） | `feature_type="delete_zone"`, `peak_lat`, `peak_lon`（対応ピーク特定用）, `area_complete`（[FR-016](#fr-016-ピーク域ポリゴン生成) により常に true 想定）。`key_col_resolved=false` のピークでも `peak_elev - delete_zone_max_drop` を閾値として delete判定ゾーンを生成する（[ADR-SRS-011](decisions/ADR-SRS-011-delete-zone-polygon.md)） |

### 6.10 入力: N03 前処理済みファイル（FR-017 生成）

**N03-{n03_year}_regions.geojson**（都道府県/振興局単位）

| 項目 | 仕様 |
|---|---|
| 用途 | 新規ピーク候補への SOTA エリアコード（例: TK・KS）と仮サミットコードの自動付与（FR-009） |
| ファイル | `$DATA_DIR/ref/N03-{n03_year}_regions.geojson` |
| 形式 | GeoJSON（RFC 7946） |
| 座標参照系 | WGS84（EPSG:4326） |
| フィーチャ数 | 61（47都道府県 + 北海道14振興局） |
| プロパティ | `assoc`（JA/JA5/JA6/JA8）、`area_code`（例: TK、IS）、`region_name`（都道府県名/振興局名） |
| 生成方法 | 前処理スクリプト（FR-017）を実行して生成 |
| 省略時の動作 | 詳細は [FR-017 参照](#fr-017-n03-行政区域前処理データ準備) |

**N03-{n03_year}_municipalities.geojson**（市区町村単位）

| 項目 | 仕様 |
|---|---|
| 用途 | 各ピーク・削除候補サミットの所在地（市区町村名）を申請書・サミット一覧（突合後）に付与（FR-009） |
| ファイル | `$DATA_DIR/ref/N03-{n03_year}_municipalities.geojson` |
| 形式 | GeoJSON（RFC 7946） |
| 座標参照系 | WGS84（EPSG:4326） |
| フィーチャ数 | 約2,000（全国市区町村） |
| プロパティ | `prefecture`（都道府県名）、`municipality`（市区町村名）、`code`（N03_007 行政区域コード） |
| 生成方法 | 前処理スクリプト（FR-017）を実行して生成 |
| 省略時の動作 | 詳細は [FR-017 参照](#fr-017-n03-行政区域前処理データ準備) |

**N03-{n03_year}_excluded_tiles.txt**（北方領土除外タイルリスト）

| 項目 | 仕様 |
|---|---|
| 用途 | 北方領土に該当するタイルを解析対象から除外するためのリスト。標高デコード時に参照し、リスト内のタイルは全ピクセルを無効値として扱う（FR-003） |
| ファイル | `$DATA_DIR/ref/N03-{n03_year}_excluded_tiles.txt` |
| 形式 | テキスト（1行1タイル、`x y` 形式の整数ペア、ズームレベル15） |
| 生成方法 | 前処理スクリプト（FR-017）を実行して生成（北方領土6村に相当する行政区域コード（N03_007: 01696〜01701）のポリゴン内タイルを列挙） |
| 省略時の動作 | 詳細は [FR-017 参照](#fr-017-n03-行政区域前処理データ準備) |

### 6.11 入力: SOTA 既存サミット GeoJSON（geojson_v{N}）

| 項目 | 仕様 |
|---|---|
| 用途 | matched・delete サミットの日本語山岳名（`summit_name_jp`）取得（FR-009） |
| ファイル | `ref/geojson_v{N}/ja0.geojson` 〜 `ja9.geojson`（{N} は `params/config.ini` の `geojson_version` パラメータで指定） |
| ファイル分割 | 全国サミットデータが10ファイルに分割されている（分割方針は出典元データに依存。詳細は出典元に確認） |
| 形式 | GeoJSON（RFC 7946） |
| 座標参照系 | WGS84（EPSG:4326） |
| `name` プロパティ形式 | `"JA/XX-NNN(山岳名)"` — SOTA コードと日本語山岳名を括弧区切りで格納 |
| 省略時の動作 | 未存在の場合、`summit_name_jp` を空文字として処理続行 |

### 6.12 中間ファイル: 全国統合済みピーク域 GeoJSON

| 項目 | 仕様 |
|---|---|
| 役割 | メッシュ別ピーク域 GeoJSON（6.9）を全国分統合した**内部中間ファイル**（通常 per-mesh のみを統合。広域モードは GeoJSON を生成しない）。[FR-009](#fr-009-sotaリスト突合match_status-判定) の point-in-polygon 突合に使用する。物理出力を残す目的はデバッグ・差分検査用。詳細: [ADR-SRS-013](decisions/ADR-SRS-013-merged-geojson-as-central-data.md) |
| ファイル | `$DATA_DIR/results/merged_activation.geojson` |
| 形式 | GeoJSON（RFC 7946） |
| 座標参照系 | WGS84（EPSG:4326） |
| フィーチャタイプ | Polygon（2種）: アクティベーションゾーン（`feature_type="activation_zone"`）・delete判定ゾーン（`feature_type="delete_zone"`） |
| プロパティ（アクティベーションゾーン） | `feature_type="activation_zone"`, `peak_lat`, `peak_lon`, `peak_elev`, `area_complete`（解析範囲内で完結している場合 true、境界で途切れた場合 false） |
| プロパティ（delete判定ゾーン） | `feature_type="delete_zone"`, `peak_lat`, `peak_lon`, `area_complete`（[FR-016](#fr-016-ピーク域ポリゴン生成) により常に true 想定。[ADR-SRS-011](decisions/ADR-SRS-011-delete-zone-polygon.md)） |

### 6.13 出力: 公開用 HTML（閲覧専用・ブラウザダウンロード）

画面サンプル: [`docs/mockup/viewer_mockup.html`](mockup/viewer_mockup.html)（作業用ビューアと共通のモックアップ。公開用は編集 UI・XLSX エクスポートなし）

| 項目 | 仕様 |
|---|---|
| 生成方式 | 作業用 HTML ビューア（`merged_viewer.html`）の「公開用 HTML」ボタンによるブラウザダウンロード（Python バッチは生成しない） |
| 用途 | GitHub Pages 等の静的ホスティングによる外部公開（申請先への証跡共有等） |
| HTML テンプレート | 閲覧専用テンプレートファイル（詳細は HLD） |
| 使用ライブラリ | Leaflet（地図・CDN 経由） |
| 特徴 | 自己完結型（GeoJSON 埋め込み・編集 UI なし・XLSX エクスポートなし・localStorage 不使用） |

---

## 7. 外部システム依存関係・環境

本ツールが依存する外部システムを以下に示す。ライブラリ・ハードウェア等の環境詳細は [`environment.md`](environment.md) を参照。

| 外部システム | 用途 |
|---|---|
| 国土地理院 標高タイル配信（`cyberjapandata.gsi.go.jp`） | DEM5a/5b/5c/DEM10b タイル取得（FR-001） |
| SOTA データベース（`sotadata.org.uk`） | サミットリスト CSV 取得（6.2） |
| 国土数値情報 N03（国土交通省） | 行政区域データ取得（FR-017） |
| 地図タイル配信（OSM・OpenTopoMap） | HTML ビューアの背景地図（6.6） |
| CDN（Leaflet・SheetJS） | HTML ビューアの地図・XLSX エクスポートライブラリ（6.6） |

---

## 8. 制約・前提条件

URD セクション 5 より:

- 標高データは国土地理院タイルのみ使用（DEM5a/5b/5c/DEM10b の優先順）
- サミット判定基準は SOTA ルール（プロミネンス ≥ 150m）に従う
- 申請書フォーマットは SOTA 日本支部指定の XLSX テンプレートに従う
- 解析対象は日本国内の 1 次メッシュ全 176 メッシュ（参照: [`ref/SOURCES.md`](../ref/SOURCES.md) — 第1次地域区画定義）
- 一部の 1 次メッシュには北方領土が含まれるが、北方領土に所在するピークは SOTA 日本支部の管轄外のため解析対象外とする
- 竹島（島根県）が含まれる 1 次メッシュ 5531 は `params/mesh_list_japan.txt` から除外済みのため解析対象外とする（根拠: [`decisions/ADR-URD-009-takeshima-exclusion.md`](decisions/ADR-URD-009-takeshima-exclusion.md)）
- 以下の参照データはツールが自動取得しない。ユーザーが手動で管理することが前提:
  - `$DATA_DIR/ref/N03-{n03_year}.geojson` — サイズが大きいため git 管理外。国土交通省 国土数値情報サイトから手動ダウンロードして配置する
  - `ref/summitslist.csv` / `ref/geojson_v{N}/` — サイズが小さいため git 管理（`ref/` 配下）。SOTA データベースの更新に合わせてユーザーが手動で差し替える（取得元は [`ref/SOURCES.md`](../ref/SOURCES.md) 参照）
- タイル取得時はインターネット接続が必要（解析・出力生成はオフライン可）
- 地理院サーバへのアクセスは[国土地理院コンテンツ利用規約](https://www.gsi.go.jp/kikakuchousei/kikakuchousei40182.html)に従い、サーバへの過度な負荷を避けること
- 本ツールは [`environment.md`](environment.md) に定める仕様と同等以上の環境での動作を前提とする

---

## 9. スコープ外

URD セクション 5 より:

- 日本以外の SOTA 申請
- 北方領土に所在するピーク（SOTA 日本支部の管轄外のため。除外方法は FR-003・FR-017 で規定）
- 竹島に所在するピーク（韓国 SOTA サミット HL/GB-430 として登録済み。SOTA 日本支部の管轄外のため。除外方法は [`ADR-URD-009`](decisions/ADR-URD-009-takeshima-exclusion.md) で規定）
- 新規サミットの山岳名取得（解析結果から自動取得する手段がなく技術的に困難なため対象外。HTML ビューアで OSM・国土地理院地図を参照しながら人間系で確認・記入すること）。既存サミットの日本語山岳名は SOTA 山名 GeoJSON（geojson_v{N}）から自動取得する
- 既存サミットの**名称変更**申請（名称はピーク解析と無関係なため自動識別しない）
- 既存サミットの**座標変更**申請（座標は概ね正しいと判断し自動識別しない）
- **バンドをまたがない標高変動**（Points 値が変わらないため SOTA 本部にとって意味のない変更）
- DEM1a（データ量が DEM5 の 25 倍、精度向上が僅少なため採用しない。根拠: [ADR-SRS-002](decisions/ADR-SRS-002-dem-hierarchy-fallback.md)）
- SOTA 申請書の提出・承認プロセス（ツールは申請書生成まで。提出は手動）
- サーバー側リアルタイム処理（解析・GeoJSON・CSV 生成はバッチ処理。申請書 XLSX は HTML ビューアでの操作によりクライアントサイドで生成）
- 地形の現地確認（目視確認は GeoJSON または HTML ビューアを使って地図上で行う）

---
