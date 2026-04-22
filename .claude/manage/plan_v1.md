# SOTA日本支部 サミット最新化ツール — 実装計画

作成日: 2026-04-20  
対象ブランチ: devel

---

## 1. 現状評価

### できていること
- Union-Find による山頂・コル検出（数学的に正確）
- 8方向オーバーラップで境界ピークを正確に処理
- DEM5a/b/c → DEM10 フォールバック
- 並列タイルダウンロード
- 1次メッシュ → CSV 出力

### 未実装（申請に必須）
- SOTA 既存サミットリストとの突合（新規/変更/削除の判定）
- XLSX 出力（申請用）
- GeoJSON 出力（目視確認用エビデンス）
- 複数メッシュの統合（重複ピーク除去）
- パラメータのハードコード問題

### 既知のバグ・懸念
- `is_tile_top` フラグの扱いが未完成（複数メッシュ統合時に問題）
- メモリリーク・エラーパス処理が未検証
- DEM10 フォールバックのオーバーラップ精度が未確認

---

## 2. アーキテクチャ方針

```
C エンジン（高速計算）
  タイル取得 → 標高デコード → 山頂/コル検出 → per-mesh CSV

Python パイプライン（申請用出力）
  全 CSV 統合 → SOTA リスト突合 → XLSX / GeoJSON 生成
```

**判断理由:**
- C に XLSX / GeoJSON ライブラリを持ち込むのはコスト大
- Python 側 (`findsummits4sotaja`) に XLSX/GeoJSON コードが既存
- 性能が必要な部分（タイル読み込み・Union-Find）は C が適切

---

## 3. フェーズ一覧

| フェーズ | 内容 | 優先度 | 目安工数 |
|---------|------|--------|--------|
| Ph.0 | 基盤整備（設定ファイル・品質確認） | 高 | S |
| Ph.1 | 複数メッシュ対応・重複除去 | 高 | M |
| Ph.2 | SOTA リスト突合 | 最高 | M |
| Ph.3 | XLSX 出力 | 最高 | M |
| Ph.4 | GeoJSON 出力 | 高 | S |
| Ph.5 | 統合テスト・実データ検証 | 高 | M |

---

## 4. 詳細タスク

---

### Phase 0: 基盤整備

**目標:** 後続フェーズの土台を固める。コードのハードコード値を除去し、信頼性を確認する。

#### Task 0-1: 設定ファイル実装
- **内容:** `config.ini` (INI形式) を導入し、ハードコード値を外出し
- **外出し対象:**
  - タイルキャッシュディレクトリ (`/mnt/findsummits/tiles/`)
  - 結果出力ディレクトリ (`/mnt/findsummits/results/`)
  - 最小プロミネンス (150m)
  - タイルダウンロードスレッド数
  - DEM タイル URL テンプレート
- **影響ファイル:** `src/main.c`, `src/mesh_analyze.c`, `src/fetch.c`、新規 `src/config.c/h`
- **テスト:** 設定値が正しく読み込まれることを確認

#### Task 0-2: メモリリーク・エラーハンドリング確認
- **内容:** `valgrind` または静的解析で主要パスのリークを確認、エラーパスでの `destroy` 漏れを修正
- **影響ファイル:** `src/elevation.c`, `src/analyze.c`, `src/mesh_analyze.c`
- **テスト:** `valgrind --leak-check=full ./findsummits 4929` でリーク 0 を確認

#### Task 0-3: DEM10 フォールバックの動作確認
- **内容:** DEM5 が存在しないタイルで DEM10 が正しく機能するか実データで確認
- **影響ファイル:** `src/elevation.c`（`elev_load_with_overlap_8dir_with_dem10`）
- **テスト:** DEM10 が必要なメッシュを指定し、CSV 出力が妥当か目視確認

---

### Phase 1: 複数メッシュ対応・重複除去

**目標:** 複数メッシュを一括処理し、境界をまたぐ重複ピークを除去する。

#### Task 1-1: 複数メッシュ一括実行スクリプト
- **内容:** メッシュコードのリストを受け取り、`findsummits` を順次実行するシェルスクリプト
- **ファイル:** 新規 `scripts/run_all.sh`
- **インタフェース:** `./scripts/run_all.sh meshcodes.txt`
- **テスト:** 隣接する 2 メッシュ（例: 4929 と 4930）を実行し CSV が生成されること

#### Task 1-2: 複数 CSV マージ・重複除去
- **内容:** Python スクリプトで複数 CSV を統合し、隣接メッシュ境界のピーク重複を除去
- **ロジック:**
  - 全 CSV を読み込み、(lat, lon) で近傍検索（半径 50m）
  - 重複判定: 同一山頂 = 緯度経度が 50m 以内、かつ標高差 < 5m
  - 重複時はプロミネンスが大きい方を採用
  - `is_tile_top=1` のピークは重複確認を必須とする
- **ファイル:** 新規 `scripts/merge_csv.py`
- **テスト:** 重複が発生する境界ケースで正しく 1 件になること

#### Task 1-3: C 側 — `is_tile_top` フラグの意味を文書化・整理
- **内容:** `is_tile_top=1` のピークが merge_csv.py でどう扱われるかの仕様を確定し、コメントで明文化
- **影響ファイル:** `src/mesh_analyze.c`, `src/unionfind.h`

---

### Phase 2: SOTA リスト突合

**目標:** 既存 SOTA JA サミットリストと検出結果を突合し、新規/変更/削除を判定する。

#### Task 2-1: SOTA サミットリスト読み込み
- **内容:** SOTA JA の CSV サミットリストを読み込むモジュール
- **入力形式:** `/mnt/findsummits/ref/` に置いてある SOTA サミット CSV
  - 列: SummitCode, SummitName, AltM, Latitude, Longitude, Points 等
- **ファイル:** 新規 `scripts/sota_list.py`
- **テスト:** 読み込んだレコード数が既知件数と一致すること

#### Task 2-2: 突合ロジック実装
- **内容:** 検出結果 CSV と SOTA リストを緯度経度で突合
- **突合ルール:**
  - 既存サミットとの最近傍を探索
  - 距離 ≤ 150m かつ標高差 < 20m → **同一サミット** とみなす
  - 距離しきい値・標高差しきい値は設定ファイルで調整可能にする
- **判定結果:**
  - `NEW`: 検出結果が既存リストにない → 新規候補
  - `MATCH`: 検出結果が既存リストと一致 → 変更なし
  - `MOVED`: 既存サミットが検出結果と 150m 超離れている → 座標変更候補
  - `ELEV_CHANGE`: 同一位置だが標高が変わった → 標高変更候補
  - `DELETED`: 既存サミットがプロミネンス 150m 未満になった → 削除候補
- **ファイル:** 新規 `scripts/match_sota.py`
- **テスト:** テスト用小規模データで 5 種類の判定が正しく出力されること

#### Task 2-3: 突合結果 CSV 出力
- **内容:** 突合済みの統合 CSV を出力
- **列:** `match_status, summit_code, summit_name, peak_lat, peak_lon, peak_elev, col_lat, col_lon, col_elev, prominence, orig_lat, orig_lon, orig_elev`
- **ファイル:** `scripts/match_sota.py`（上記の続き）
- **出力先:** `/mnt/findsummits/results/csv/matched_summits.csv`

---

### Phase 3: XLSX 出力（申請用）

**目標:** SOTA 日本支部の申請フォーマットに従った XLSX ファイルを生成する。

#### Task 3-1: SOTA 申請書フォーマットの仕様確認
- **内容:** SOTA JA から提供されている申請用テンプレート Excel のシート構成・列定義を確認し文書化
- **作業:** 既存テンプレートを `/mnt/findsummits/ref/` から確認
- **成果物:** `tasks/xlsx_spec.md`（列定義・書式ルール）

#### Task 3-2: 新規申請シート生成
- **内容:** `match_status=NEW` のサミットを申請用 XLSX の「新規」シートに出力
- **列構成（暫定）:**
  - SummitCode (空欄 or ZZ/ZZ-XXX)
  - SummitName (空欄)
  - AltM
  - Latitude, Longitude
  - Peak AltM, Peak Lat, Peak Lon
  - Col AltM, Col Lat, Col Lon
  - Prominence
- **ライブラリ:** `openpyxl`
- **書式:** プロミネンス < 150m の行を黄色ハイライト（警告用）
- **ファイル:** 新規 `scripts/output_xlsx.py`
- **テスト:** 出力された XLSX を Excel/LibreOffice で開いて視覚確認

#### Task 3-3: 変更・削除シート生成
- **内容:** `match_status=MOVED/ELEV_CHANGE` → 変更シート、`DELETED` → 削除シート
- **列構成:** 既存値と新値を並べて差分が一目でわかる形式
- **ファイル:** `scripts/output_xlsx.py`（上記の続き）

#### Task 3-4: MATCH シート（確認用）
- **内容:** `match_status=MATCH` のサミットを「変更なし」シートとして追加（エビデンス用）
- **ファイル:** `scripts/output_xlsx.py`

---

### Phase 4: GeoJSON 出力（目視確認用）

**目標:** 国土地理院地図で視覚確認できる GeoJSON を生成する。

#### Task 4-1: GeoJSON 仕様確認
- **内容:** 地理院地図が受け付けるカスタムプロパティ仕様を確認（`findsummits4sotaja` の出力を参考）
- **確認項目:** `_markerType`, `_iconUrl`, `_iconSize`, `_iconAnchor`, `_color`, `_opacity`, `_weight`

#### Task 4-2: Summit / Peak / Col の Point Feature 生成
- **内容:** 各サミットについて Point を 3 つ生成
  - Summit（アイコン付き、SOTA スコア別色分け）
  - Peak（DEM 上の最高点）
  - Col（キーコル）
- **色分け（SOTA スコア）:**
  - 10点: 赤, 8点: 黄, 6点: 緑, 4点: シアン, 2点: 青, 1点: 紫, NEW: マゼンタ
- **ファイル:** 新規 `scripts/output_geojson.py`

#### Task 4-3: LineString Feature 生成
- **内容:** Summit→Peak, Peak→Col の LineString を追加（関係を視覚化）
- **ファイル:** `scripts/output_geojson.py`

#### Task 4-4: match_status 別マーカー差分化
- **内容:** `match_status` ごとにアイコン/色を変えて地図上で変更/削除候補が識別できるようにする
  - NEW: マゼンタ, MOVED: オレンジ, ELEV_CHANGE: 水色, DELETED: グレー, MATCH: スコア色
- **ファイル:** `scripts/output_geojson.py`

---

### Phase 5: 統合テスト・実データ検証

**目標:** 実際の申請に耐えられる品質を確認する。

#### Task 5-1: 九州・四国 メッシュで end-to-end テスト
- **内容:** 過去に申請した九州・四国のメッシュを使い、フルパイプラインを実行
- **期待値:** `findsummits4sotaja` の過去結果（`sotaJA_SummitPeakCol_All5.xlsx`）と大きな差異がないこと
- **許容差:** ピーク座標 ±100m、標高 ±5m、プロミネンス ±20m

#### Task 5-2: 境界ピーク精度確認
- **内容:** 複数メッシュの境界付近にある既知の山（地図上で確認できる山）が正しく 1 件だけ検出されているか確認
- **手順:** GeoJSON を地理院地図にドロップして目視

#### Task 5-3: DEM なしエリアの確認
- **内容:** DEM5 が存在しないメッシュで DEM10 フォールバックが動作し、結果が妥当であることを確認

#### Task 5-4: パイプライン自動実行スクリプト整備
- **内容:** `make pipeline MESH=4929` 一発で C 解析 → CSV マージ → SOTA 突合 → XLSX/GeoJSON 出力まで実行できるようにする
- **ファイル:** `Makefile` に `pipeline` ターゲット追加, `scripts/pipeline.sh`

---

## 5. ファイル構成（計画後）

```
findsummits/
  src/
    config.c/h            ← Task 0-1: 新規
    main.c, mesh.c, ...   ← 既存（一部修正）
  scripts/
    run_all.sh            ← Task 1-1: 新規
    merge_csv.py          ← Task 1-2: 新規
    sota_list.py          ← Task 2-1: 新規
    match_sota.py         ← Task 2-2/2-3: 新規
    output_xlsx.py        ← Task 3-2～3-4: 新規
    output_geojson.py     ← Task 4-2～4-4: 新規
    pipeline.sh           ← Task 5-4: 新規
  tasks/
    todo.md
    lessons.md
    xlsx_spec.md          ← Task 3-1: 新規
  tests/
    test_*.c              ← 既存
  plan.md                 ← 本ファイル
  Makefile                ← pipeline ターゲット追加
```

---

## 6. テスト方針まとめ

| フェーズ | テスト手法 | 合格基準 |
|---------|----------|--------|
| Ph.0 | valgrind、手動実行 | メモリリーク 0、CSV 出力正常 |
| Ph.1 | 隣接 2 メッシュで実行 | 重複ピーク 0、件数一致 |
| Ph.2 | 小規模テストデータ | 5 種類の match_status が正しく出力 |
| Ph.3 | Excel/LibreOffice 開く | 列・書式・シート構成が申請要件を満たす |
| Ph.4 | 地理院地図で GeoJSON ドロップ | 全サミットが地図上に正しく表示 |
| Ph.5 | 九州・四国 実データ | 過去申請結果と許容差内で一致 |

---

## 7. 未決事項・要確認

1. **SOTA 申請用 Excel テンプレートの正確な列定義**  
   → `/mnt/findsummits/ref/` にテンプレートがあるか確認が必要

2. **突合距離しきい値（150m）の妥当性**  
   → 既存サミットの「公式座標 vs DEM 最高点」の実際の誤差を確認してから調整

3. **SummitCode の採番ルール（新規サミット）**  
   → SOTA JA 事務局の採番方式に従う必要がある（暫定は ZZ/ZZ-XXX）

4. **複数申請範囲（どの 1 次メッシュを対象とするか）**  
   → ユーザーが指定するリストが必要

---

## 8. 実装の進め方

各 Phase を順番に実装し、各 Task 完了時に動作確認を行う。  
Ph.2 と Ph.3 は依存関係があるため順次実行。  
Ph.4 は Ph.3 と並行して進めることが可能。

**最初に着手する Task:** Task 0-1（設定ファイル）と Task 3-1（XLSX 仕様確認）を並行して行い、全体像を固める。
