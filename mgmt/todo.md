# TODO

> このファイルは **作業リスト** の保管庫。
> 文書・仕様の議論を伴うもの（SRS/HLD/LLD/ADR 等の改訂が必要なもの）は
> `venv/bin/python3 mgmt/tracker/track.py issue` で管理する。
> 詳細は `/workspace/CLAUDE.md` の「ToDo リスト運用ルール」を参照。

## 運用ルール（要約）

- 用途: 文書・仕様の議論を伴わない作業（実装タスク・ファイル名追従・ログ整備・運用作業 等）
- 判定: 「issue にすべきか todo にすべきか」迷ったら **「文書・仕様の議論を伴うか？」** で機械的に判定する
- 完了したものは消す（履歴は git で追える）
- 長期保留は「保留」セクションへ

---

## 次回着手（優先順）

### 高

- [ ] **(ISSUE-108 コード追従) FR-023 shell スクリプト新規実装**
  - Phase2（FR-004 全メッシュ）→ FR-008+FR-018 → FR-022 → N=4/5/6 ループ（FR-014→FR-008+FR-018→FR-022）→ Phase4（FR-009）を制御する shell スクリプト
  - N カウンタを一元管理。FR-014 に N を明示引数で渡す
  - 各フェーズ単独起動オプション・自動ループオプションを実装
  - 仕様詳細: `docs/20_SRS.md` FR-023・`docs/decisions/ADR-SRS-027-fr022-purification-fr023-pipeline-control.md`

- [ ] **(ISSUE-108 コード追従) FR-022 コード純化（N ループ・陸地最高峰・FR-014呼び出し削除）**
  - `scripts/merge.py` の FR-022 相当処理からエスカレーションループ・FR-014 呼び出し・FR-008 再実行・陸地最高峰処理を除去
  - 出力: `key_col_unresolved_peaks-<N>.csv`（N は呼び出し元から渡す）・未確定有無の返却のみ
  - 仕様詳細: `docs/20_SRS.md` FR-022

- [ ] **(ISSUE-108 コード追従) FR-014 入力変更: N は明示引数で受け取る（ファイル名から抽出しない）**
  - `scripts/merge.py` または C エンジンの FR-014 相当処理で N をファイル名から導出せず、引数として受け取るよう修正
  - 仕様詳細: `docs/20_SRS.md` FR-014 入力テーブル「解析範囲サイズ N」

- [ ] **(ISSUE-109 コード追従) FR-008: 陸地最高峰リスト読み込みと海面確定処理を追加**
  - `scripts/merge.py` の統合処理に `params/` 配下の陸地最高峰リスト読み込みを追加
  - merged_peak.csv 生成時に陸地最高峰リスト一致ピークを `key_col_resolved=true`・`col_elev=0` に更新する処理を追加
  - 仕様詳細: `docs/20_SRS.md` FR-008 入力・説明（陸地最高峰の海面確定）

- [ ] **(元 ISSUE-106) コード追従: 中間 GeoJSON へのピーク/コル可視化フィーチャ追加（ADR-SRS-026）**
  - `mesh_analyze.c` の GeoJSON 出力に `feature_type="peak"` Point・`feature_type="key_col"` Point（key_col_resolved=true のみ）・`feature_type="peak_col_link"` LineString を追加。地理院地図スタイル属性付与（色スキームは HLD で規定）
  - `merge.py` の `merged_activation.geojson` 生成処理に同様のフィーチャを追加（座標元は `merged_peak.csv` の col_lat/col_lon）
  - FR-009 の point-in-polygon 処理を `feature_type ∈ {activation_zone, delete_zone}` のポリゴンに絞るフィルタを追加
  - 仕様詳細: `docs/decisions/ADR-SRS-026-intermediate-geojson-peak-col-visualization.md`

- [ ] **(HLD フェーズ・元 ISSUE-106 派生) 地理院シンボル18番号体系の HLD 色スキーム規定**
  - HLD の色スキームセクションに下記マッピングを記載する（`viewer_mockup.html` では JS 定数として先行実装済み）
  - | pt | ピーク | コル | サミット |
    |---|---|---|---|
    | 1pt  | 398 | 697 | 826 |
    | 2pt  | 077 | 093 | 102 |
    | 4pt  | 400 | 699 | 828 |
    | 6pt  | 078 | 094 | 103 |
    | 8pt  | 079 | 095 | 104 |
    | 10pt | 076 | 092 | 101 |
  - URL: `https://maps.gsi.go.jp/portal/sys/v4/symbols/{番号}.png`（3桁ゼロパディング必須）

- [ ] **(HLD フェーズ・元 ISSUE-106 派生) 中間 GeoJSON の標高ベース色スキームを HLD で規定**
  - 中間 GeoJSON（merged_peak.geojson 等）は SOTA ポイント未割当のため pt ベース番号が使えない
  - ADR-SRS-026 L50-51 の「標高ベース等の代替スキームは HLD で規定」に基づき、HLD で番号体系を別途確定する

- [ ] **(HLD フェーズ・元 ISSUE-106 派生) ADR-SRS-026 L49 の記述整理**
  - L49「▲▽の厳密再現は地理院地図では保証しない（FontAwesome 非対応）」を、地理院シンボル統一方針（実装済み）に合わせて書き換える
  - 対象: `docs/decisions/ADR-SRS-026-intermediate-geojson-peak-col-visualization.md`

- [ ] **(HLD フェーズ・元 ISSUE-106 派生) 本番コードへの地理院シンボル反映と config.ini 化**
  - `mesh_analyze.c`・`merge.py`・`output_geojson.py` に 18番号体系を適用
  - `params/config.ini` に `[icons]` セクションを追加してシンボル番号を上書き可能にする（ADR-SRS-031 の前例に倣う）
  - モックアップ（`viewer_mockup.html`）の `ICON_SYMBOLS` 定数と同一番号を初期値として設定

- [ ] **(元 ISSUE-079) scripts/preprocess_pref_boundaries.py を FR-017 改訂版仕様に追従**
  - ZIP 自動検出方式（`$DATA_DIR/ref/` を `N03-(\d{8})_GML\.zip` で走査、YYYYMMDD 最大を採用）
  - ZIP 内市区町村版 GeoJSON のみを一時ディレクトリに展開して処理、処理後削除
  - dissolve 出力を 60 地域（46 都府県 + 14 振興局）に修正
  - `params/config.ini` の `n03_year` 設定参照を廃止
  - 採用 ZIP 名・YYYYMMDD・サイズをログに出力
  - 北方領土除外タイルリスト生成（ADR-SRS-018: NORTHERN_CODES = {01695..01700}）
  - 仕様詳細: `docs/20_SRS.md` FR-017・7.1.3・7.2.1・ADR-URD-005

- [ ] **(元 ISSUE-056) prefetch_tiles.py: FR-001 仕様追従**
  - `fetch_dem5_with_fallback()` の早期 break 撤廃 → dem5a/b/c を独立ジョブとして列挙
  - `enumerate_jobs()` に dem5b/dem5c を常に含める
  - `fetch_one()` で HTTP 404 受信時にローカルキャッシュを `os.unlink()` 削除
  - 北方領土除外タイルリスト読み込みとスキップ処理（ADR-SRS-018）
  - 対象: `scripts/prefetch_tiles.py` （185-199 行付近・131-180 行付近）

- [ ] **(元 ISSUE-059) 実装側ファイル名追従（merged_peak.csv / merged_summit.xlsx）**
  - `scripts/merge.py:50` の DEFAULT_OUTPUT → `merged_peak.csv`
  - `scripts/output_geojson.py:34` の DEFAULT_INPUT → `merged_peak.csv`
  - FR-009 に `merged_summit.xlsx` 出力ロジック追加（カラム構成は FR-012 と同じ）
  - 注: FR-012 実装（`merged_summit_revised.xlsx`）は ISSUE-062 として分離管理

- [ ] **(元 ISSUE-064) gsi_tile_latest_date を merged.geojson の metadata に格納**
  - ローカルキャッシュタイルの mtime 最大値を集計し、YYYY-MM-DD 形式（UTC）で格納
  - 対象: `scripts/merge.py` または `scripts/prefetch_tiles.py`
  - 仕様: SRS FR-019（ビューア表示時は末尾に `(UTC)` を付記）

### 中

- [ ] **(元 ISSUE-013) prefetch_tiles.py 取得範囲を各メッシュの最小範囲に修正**
  - `enumerate_jobs()` の隣接メッシュ拡張ロジック（`nb not in mesh_set` の分岐）を削除
  - 各メッシュ単独の `mesh_to_tile_range(meshcode)` のみを取得

- [ ] **(元 ISSUE-016・ADR-SRS-023) merge.py: --mesh-list 指定時の絞り込み（FR-008 追従）**
  - `--mesh-list` 指定時は `load_peaks()` を**通常 per-mesh CSV（`3-<meshcode>.csv`）のみ**に絞り込み、**広域 per-mesh CSV（`4/5/6-<meshcode>-<コーナー>.csv`）は対象外**とする
  - 省略時（デフォルト）は現行の glob 全読み込み（通常+広域）を維持
  - `expected_count` 算出は `--mesh-list` の指定有無に関わらず**常に日本全土1次メッシュコードリスト（`params/mesh_list_japan.txt`）基準**で行う（絞り込みリストを expected_count 算出に流用しない）
  - 代表採用ロジックを ADR-SRS-023 の4段（key_col_resolved優先→col_elev降順→通常モード優先＋analysis_id昇順→全件false時はfalse維持）に合わせる
  - 根拠: [ADR-SRS-023](../docs/decisions/ADR-SRS-023-fr008-merge-input-mesh-list-semantics.md)

- [ ] **(ADR-SRS-022) mesh_analyze.c: per-mesh GeoJSON プロパティを join 方式に追従**
  - Feature properties を `peak_lat`/`peak_lon`/`feature_type`/`area_complete` の4フィールドのみに変更
  - プロミネンス・コル標高等の属性は GeoJSON から除去（per-mesh CSV に集約済み）
  - 根拠: [ADR-SRS-022](../docs/decisions/ADR-SRS-022-per-mesh-geojson-property-design.md)
  - 注: ADR-SRS-010 C++ 移行（ISSUE-037/038/040）と同時実施が効率的

- [ ] **(ADR-SRS-022) merge.py: merged_peak.csv と merged_activation.geojson の join 実装**
  - FR-009 突合処理で `peak_lat`/`peak_lon` を join キーとして merged_peak.csv と merged_activation.geojson を突合するロジックを実装
  - 根拠: [ADR-SRS-022](../docs/decisions/ADR-SRS-022-per-mesh-geojson-property-design.md)

- [ ] **(元 ISSUE-041) 実装側フラグ命名追従（is_tile_top → key_col_resolved + 真偽値反転）**
  - C 側: `src/analyze.h/c`, `src/mesh_analyze.c`, `tests/test_analyze.c`
  - Python 側: `scripts/merge.py`, `scripts/output_geojson.py`
  - 注: ADR-SRS-010 Phase 1/2（C → C++/OpenCV 移行）と同時実施が効率的（ISSUE-037/038 と連携）
  - CSV カラム名は C 側と Python 側で揃える必要あり（片方だけ変更するとパイプライン断絶）

- [ ] **(元 ISSUE-045) config.ini.example に delete_zone_max_drop = 250 追加**
  - `params/config.ini.example` の `[analysis]` セクション（または相当箇所）に追加
  - 値の根拠コメントとして ADR-SRS-011 を参照
  - 既存のローカル `params/config.ini`（gitignore）は手動で同期する旨をユーザーに通知

### 低

- [ ] **(元 ISSUE-086) src/mesh_analyze.h 内コメント「Terrain-RGB PNG」の修正**
  - 「Terrain-RGB」は Mapbox 由来の入力タイル形式の業界用語。出力ファイルの呼称として誤っている
  - SRS 6.8 のタイトル変更（標高地形図 PNG）に合わせて実装側コメントを追従する

- [ ] **(元 ISSUE-082) SRS/URD 全体の裸 FR-XXX/UR-XXX/NFR-XXX 参照の一括リンク化**
  - FR-015 周辺は対応済み。残りの SRS/URD 全体分（約 100 件規模）が対象
  - CLAUDE.md フォーマット標準「他 FR/UR/NFR/セクションへの参照は Markdown リンクで記述する」に基づく機械的変換

- [ ] **docs/figures/phases_overview.drawio: FR-014 タイトル追従**
  - 「FR-014: 独立峰のコル探索」→「FR-014: 広域結合解析オーケストレーション」
  - 対象: `docs/figures/phases_overview.drawio` および `phases_overview.drawio.svg`（再エクスポート必要）
  - 根拠: ISSUE-075（2026-06-09 SRS 改訂）

- [ ] **(FR-016 論点5) per-mesh GeoJSON 出力パスの命名整理検討**
  - 現在 `$DATA_DIR/results/csv/<meshcode>_activation.geojson` に出力しているが、csv/ 配下に geojson を置く命名が紛らわしい
  - CSV と GeoJSON で出力先を分けるか、サブディレクトリを設けるか検討する（ADR または todo で決定後に SRS FR-016 と mesh_analyze.c を更新）

- [ ] **(元 ISSUE-053) docs/figures/context.drawio: ラベル大文字統一**

- [ ] **(元 ISSUE-060) CLAUDE.md / README.md の merged.csv 言及箇所追従**
  - merged.csv → merged_peak.csv 等の最新命名に追従

- [ ] **(元 ISSUE-065) モックアップ「変更あり」フィルターを is_band_change_cand に変更**

### FR-013レビュー決着の機械反映（finding 3/4/6/7・用語変更）

以下は仕様確定済み。SRS/ADR は反映済み。実装追従のみ残る。

- [ ] **finding 6: `feature_type="col"` → `"key_col"` への統一（実装追従）**
  - `scripts/merge.py`: GeoJSON フィーチャ生成時の `feature_type` を `"col"` → `"key_col"` に変更
  - `scripts/output_geojson.py`（存在する場合）: 同上

- [ ] **finding 5（実装追従）: 不備ゲート発動時の xlsx セット出力**
  - `scripts/merge.py`: FR-009 の不備ゲート判定箇所で xlsx も必ず出力してから exit するよう修正
  - xlsx に `area_complete` 列を追加・`match_status` 値域に delete/unmatched を含める

- [ ] **finding 3（実装追従）: `is_band_change_candidate` を GeoJSON ピーク Point プロパティに追加**
  - `scripts/merge.py`: ピーク Point フィーチャの properties に `is_band_change_candidate` を出力

- [ ] **finding 2（実装追従）: FR-001 での `Last-Modified` → mtime 焼き込み**
  - `scripts/prefetch_tiles.py`: HTTP 200 取得時に `os.utime()` で `Last-Modified` を mtime に設定

- [ ] **finding 2（実装追従）: FR-009 での `gsi_tile_latest_date` 格納**
  - `scripts/merge.py`: 処理末尾で `$DATA_DIR/tiles/` 全 PNG の mtime 最大値を取得し metadata に格納

- [ ] **finding 7: 旧名 `merged.geojson` の追従漏れ確認**
  - `scripts/` 配下のコードに `merged.geojson` （`merged_summit`/`merged_peak` を除く）が残存していないか確認・修正

---

## 保留

（現在なし）

---

## 履歴

このファイルは作業中リストのみ保持する。完了したものは削除し、git の履歴で追跡する。
