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
- [ ] 既存 CSV 退避・削除（`/mnt/findsummits/results/csv/*.csv`）← 5339.csv のみ残存
- [ ] `params/fetch_config.ini.example` 作成、`.gitignore` に実設定を追加
- [ ] 小規模 9 メッシュでフルパイプ検証（prefetch → C 解析 → merge.py）
- [ ] コミット（残り分：fetch_config.ini.example 等）

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

## 保留・将来対応

- タイル取得「最後の 1 枚」ハング問題は prefetch に移しての再現性確認
- Union-Find の CPU 並列化
- README.md「使用する標高データについて」セクションの移動
- `.claude/settings.json` / `.mcp.json` の扱い（空ファイル、放置中）
