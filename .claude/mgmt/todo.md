# TODO

承認済みアーキテクチャ刷新計画: `.claude/manage/plan.md`（旧計画は `.claude/manage/plan_v1.md`）

## 方針（2026-04-22 承認）

- `BORDER_TILES=8` 廃止 → 3×3 最小矩形方式に
- タイル取得は C から切り離し、`scripts/prefetch_tiles.py` の事前 prefetch に一本化
- `rank` int8 化 + `peak_id` ハッシュマップ化でメモリ削減
- `cmp_elev_desc` に 2 次キー (x→y) を追加して座標を決定論化
- 中心メッシュフィルタ廃止、combined 内の全ピークを CSV 出力
- CSV に `col_margin_px`, `center_mesh` を追加
- `prominence` は C 側 130m、Python 側で 150m 最終判定
- 既存 9 メッシュ CSV は削除し新方式で再解析

## 実装ステップ

- [x] `src/mesh.c/h`: `MeshSet` と隣接計算ヘルパ追加（c8417b1）
- [x] `src/unionfind.c/h`: `rank` int8 化、`peak_id` ハッシュマップ化（c8417b1）
- [x] `src/analyze.c/h`: 2 次キー x→y、プロミネンス 130m、`col_margin_px` 計算、`PeakResult` 拡張（c8417b1）
- [x] `src/mesh_analyze.c/h`: 3×3 最小矩形、中心フィルタ撤廃、全ピーク出力、CSV 新列（c8417b1）
- [x] `src/fetch.c/h` 完全削除、`src/elevation.c` の fetch 呼出除去（c8417b1）
- [x] `Makefile`: `-lcurl` 除去、`test_fetch` ターゲット撤去（c8417b1）
- [x] `src/main.c`: MeshSet 読み込み、`min_prominence=130.0f`（c8417b1）
- [x] ビルド・単体テスト確認（5540・5539 単体で動作確認済み）
- [x] `scripts/prefetch_tiles.py` 新規作成（If-Modified-Since、並列4、backoff、フォールバック a→b→c）（d4c2946・26aaa2c）
- [x] `scripts/merge.py` 改修（期待解析回数、col_elev 最小、col_margin_px、150m 最終判定）（d4c2946）
- [x] 既存 CSV 退避・削除（`/mnt/findsummits/results/csv/*.csv`）← 5339.csv のみ残存
- [x] `params/fetch_config.ini.example` 作成、`.gitignore` に実設定を追加
- [ ] 小規模 9 メッシュでフルパイプ検証（prefetch → C 解析 → merge.py）
  - 対象メッシュ: `params/mesh_5339_neighbors.txt`（5238〜5440 の9メッシュ、5339含む）
  - [x] ステップ1: `prefetch_tiles.py` でタイル取得（dem5完了済み）
    - dem10b は URL 修正済み（`dem10b_png` → `dem_png`）。`/data/tiles/14/` 残骸削除済み。次回 prefetch で正しく取得される
  - [ ] ステップ2: `findsummits` で9メッシュ解析
    - `make && ./build/findsummits params/mesh_5339_neighbors.txt`
    - 出力: `/data/results/csv/<meshcode>.csv`（9ファイル）
    - ※ 実行前に dem10b prefetch 推奨: `python3 scripts/prefetch_tiles.py params/mesh_5339_neighbors.txt`
  - [ ] ステップ3: `merge.py` でCSV統合・SOTA突合
    - `python3 scripts/merge.py`
    - プロミネンス ≥ 150m でフィルタ、`ref/summitslist.csv` と突合
    - 出力結果を目視確認
- [ ] コミット（今日の修正分を含む）
  - 整数オーバーフロー修正（analyze.c, unionfind.h/c, elevation.c）
  - Pixel配列 → インデックス配列によるメモリ削減（analyze.c）
  - 標高カラーマップをterrain_viz.c配色に統一（mesh_analyze.c）
  - dem10b URL修正（prefetch_tiles.py）

## 申請用出力フェーズ（フルパイプ検証後に着手）

- [ ] **着手前に設計を確認**: plan_v1.md の Ph.2〜Ph.5 の設計（別スクリプト構成）が現在のアーキテクチャ（merge.py 統合など）に合っているか Sonnet と相談してから実装を始める

### SOTA突合（merge.py 拡張）

- [ ] match_status 判定を merge.py に実装
  - `NEW`: 検出結果が既存リストにない → 新規候補
  - `MATCH`: 既存サミットと座標・標高が一致 → 変更なし
  - `MOVED`: 既存サミットが検出結果と 150m 超離れている → 座標変更候補
  - `ELEV_CHANGE`: 同一位置だが標高差あり → 標高変更候補
  - `DELETED`: 既存サミットが prominence < 150m になった → 削除候補
  - 突合しきい値: 距離 ≤ 150m かつ標高差 < 20m で同一サミットとみなす
- [ ] 解析範囲内サミットのみ deleted 対象とするフィルタ（deleted スコープ問題の解決）
- [ ] 突合結果の列: `match_status, summit_code, summit_name, peak_lat, peak_lon, peak_elev, col_lat, col_lon, col_elev, prominence, orig_lat, orig_lon, orig_elev`

### XLSX出力（output_xlsx.py 新規）

- [ ] openpyxl で申請用 XLSX を生成
  - 新規シート: `match_status=NEW` のサミット
  - 変更シート: `match_status=MOVED/ELEV_CHANGE`（既存値と新値を並列表示）
  - 削除シート: `match_status=DELETED`
  - 変更なしシート: `match_status=MATCH`（エビデンス用）
- [ ] SOTA 申請用テンプレート（`ref/SOTA-Summit-list-revision-request.xlsx`）の列定義を事前確認
- [ ] prominence < 150m の行を黄色ハイライト（警告用）
- [ ] Excel/LibreOffice で開いて視覚確認

### GeoJSON出力（output_geojson.py 新規）

- [ ] Summit/Peak/Col ごとに Point Feature を生成
  - match_status 別アイコン/色: NEW=マゼンタ, MOVED=オレンジ, ELEV_CHANGE=水色, DELETED=グレー, MATCH=スコア色
  - SOTA スコア別色（MATCH のみ）: 10点=赤, 8点=黄, 6点=緑, 4点=シアン, 2点=青, 1点=紫
- [ ] Summit→Peak, Peak→Col の LineString を追加（関係を視覚化）
- [ ] 地理院地図にドロップして目視確認

### 統合テスト

- [ ] 九州・四国メッシュで end-to-end テスト
  - `findsummits4sotaja` の過去結果（`sotaJA_SummitPeakCol_All5.xlsx`）と比較
  - 許容差: ピーク座標 ±100m、標高 ±5m、プロミネンス ±20m
- [ ] 複数メッシュ境界付近の既知の山が GeoJSON で正しく 1 件表示されることを確認
- [ ] DEM5 なしメッシュで DEM10 フォールバックの結果が妥当か確認

---

## 突合仕様（要確認）

- [ ] **deleted の地理的スコープ問題**
  　現実装は JA 全サミットを突合に投入するため、解析対象メッシュ外のサミットも
  　deleted に入ってしまう。解析範囲内のサミットのみ対象に絞るフィルタが必要か確認する。
- [ ] **stability フィールドの申請書での扱い**
  　confirmed/unstable のまま申請書 status 列に使って良いか確認する。

## 将来実装候補

- [ ] **新規サミット仮コードの地域コード自動付与**
  　現在は `ZZ/ZZ-001` 形式の連番。座標から都道府県を判定し、
  　SOTA JA のアソシエーション/リージョンコード（北海道・本州・四国・九州）を
  　仮コードに反映できるか実装を検討する。

## フルパイプ検証後に着手

- [ ] `scripts/run_all.sh` の仕様確認・必要に応じて修正
  - フルパイプ検証（9メッシュ）が通ってから実施する
  - 理由: 検証前に仕様を詰めても実行時の問題で仕様変更になりやすい

## インフラ対応

- [ ] **swap領域を増やす**: 現在7.73GB。5339解析時にmax90.8%（≒56.9GB）使用しswapに入った可能性が高い。全国解析で同様ケースが繰り返されるため、16〜32GBへの拡張を推奨。

## 次セッション以降の優先順

1. **dem10b未取得タイルの調査・対応**: prefetch後も未キャッシュが残る原因を調査。GSI側で未整備なのか、取得漏れなのかを切り分ける
2. **terrain PNG 水平・垂直線バグの調査**: `5239_terrain.png` で確認済み。タイル境界接合処理が原因と推測。コード調査
3. **ログ出力の1メッシュ1ファイル化**: `mesh_analyze()` にFILEポインタを渡し `$DATA_DIR/logs/<meshcode>.log` に保存（Cコード修正）

---

## terrain PNG カラーマップ変更（次セッション以降）

- [ ] `mesh_analyze.c` のカラーマップを以下の仕様に変更する
  - **0m以下（海・水深）**: 青系グラデーション
    - -6000m: `#0938BF`（濃い青）
    - -2500m: `#50D9FB`（明るい青）
    -    -1m: `#B7E5FA`（薄い青）
  - **0m以上（陸地）**: 緑→茶→白グラデーション
    -     0m: `#1F4806`（濃い緑）
    -   100m: `#68E36B`（明るい緑）
    -   150m: `#98D685`（黄緑）
    -   300m: `#F9EFCD`（薄黄）
    -   800m: `#E0BB7D`（茶色）
    -  1000m: `#D3A62D`（濃い茶）
    -  2500m: `#997618`（暗い茶）
    -  3000m: `#705B10`（濃い茶）
    -  3500m: `#5F510D`（濃褐色）
    -  4000m: `#A56453`（赤茶）
    -  5000m: `#5C1D09`（黒茶）
    -  5500m+: `#FFFAFA`（白/snow）
  - 参考URL: https://memomemokun.hateblo.jp/entry/2019/02/14/085141
  - 注意: 日本の標高スケールに合わせてポイント間の補間を調整すること

## 保留・将来対応

- タイル取得「最後の 1 枚」ハング問題は prefetch に移しての再現性確認
- Union-Find の CPU 並列化

- **【解決済み】dem10b URLの誤設定**（2026-04-24修正）
  - 原因: `prefetch_tiles.py` の URL が `dem10b_png` だったが正しくは `dem_png`
  - 正しいURL: `https://cyberjapandata.gsi.go.jp/xyz/dem_png/{z}/{x}/{y}.png`
  - 修正内容: `GSI_DEM10B_URL` を `dem_png` に変更、動作確認済み（HTTP 200）
  - キャッシュ残骸: `/data/tiles/14/` に誤保存したエラーXMLが残存 → 次回prefetch前に削除推奨
- README.md の全体更新（申請用出力フェーズ完了後に実施）
  - 更新に適したタイミング: フルパイプ検証（prefetch → findsummits → merge.py）が通り、
    申請用出力（output_xlsx.py・output_geojson.py）の実装が完了した時点
  - 理由: それまではパイプラインが未完成で正確な使い方が書けない
  - 更新内容: インストール・実行手順（全ステップ）、出力ファイルの説明、
    「使用する標高データについて」セクションの整理
- `.claude/settings.json` / `.mcp.json` の扱い（空ファイル、放置中）
