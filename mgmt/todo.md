# TODO

> ⚠️ 長期タスクは `.claude/mgmt/issue-system/issue.py` で管理。
> このファイルはセッション内の作業メモ・完了チェックリストとして利用する。
> 承認済みアーキテクチャ刷新計画: `.claude/mgmt/plan.md`（旧計画は `.claude/mgmt/plan_v1.md`）

## 方針（2026-04-22 承認）

- `BORDER_TILES=8` 廃止 → 3×3 最小矩形方式に
- タイル取得は C から切り離し、`scripts/prefetch_tiles.py` の事前 prefetch に一本化
- `rank` int8 化 + `peak_id` ハッシュマップ化でメモリ削減
- `cmp_elev_desc` に 2 次キー (x→y) を追加して座標を決定論化
- 中心メッシュフィルタ廃止、combined 内の全ピークを CSV 出力
- CSV に `col_margin_px`, `center_mesh` を追加
- `prominence` は C 側 130m、Python 側で 150m 最終判定
- 既存 9 メッシュ CSV は削除し新方式で再解析

## 実装ステップ（アーキテクチャ刷新）

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

## フルパイプ検証（9メッシュ: 5238〜5440）

- [x] ステップ1: `prefetch_tiles.py` でタイル取得（dem5完了済み）
- [x] ステップ2: `findsummits` で9メッシュ解析（2026-04-25完了）
  - 出力: `/data/results/csv/<meshcode>.csv`（9ファイル）合計4006ピーク
- [x] ステップ3前提確認（2026-04-26完了）
  - deleted の地理的スコープ: mesh_bbox() によるbboxフィルタを実装
  - 出力列設計: match_status と stability を分離
- [ ] **ステップ3**: `merge.py` でCSV統合・SOTA突合 → **ISSUE-001**
  - `python3 scripts/merge.py --mesh-list params/mesh_5339_neighbors.txt`

## インフラ

- [x] swap 領域を増やす: `/swapfile`（32GB, NVMe）追加、合計39.7GB（2026-04-25）

---

## terrain PNG カラーマップ変更仕様メモ

`mesh_analyze.c` のカラーマップ変更時の参照用:

- **0m以下（海・水深）**: 青系グラデーション
  - -6000m: `#0938BF` / -2500m: `#50D9FB` / -1m: `#B7E5FA`
- **0m以上（陸地）**: 緑→茶→白グラデーション
  - 0m: `#1F4806` / 100m: `#68E36B` / 150m: `#98D685` / 300m: `#F9EFCD`
  - 800m: `#E0BB7D` / 1000m: `#D3A62D` / 2500m: `#997618` / 3000m: `#705B10`
  - 3500m: `#5F510D` / 4000m: `#A56453` / 5000m: `#5C1D09` / 5500m+: `#FFFAFA`
- 参考: https://memomemokun.hateblo.jp/entry/2019/02/14/085141
