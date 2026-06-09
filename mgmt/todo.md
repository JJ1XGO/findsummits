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

- [ ] **(元 ISSUE-016) merge.py: --mesh-list 指定時の絞り込み（FR-008 追従）**
  - `load_peaks()` に指定メッシュコードでの絞り込みを追加
  - 省略時は現行の glob 全読み込みを維持

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

- [ ] **docs/figures/phases_overview.drawio: FR-014 タイトル追従**
  - 「FR-014: 独立峰のコル探索」→「FR-014: 広域結合解析オーケストレーション」
  - 対象: `docs/figures/phases_overview.drawio` および `phases_overview.drawio.svg`（再エクスポート必要）
  - 根拠: ISSUE-075（2026-06-09 SRS 改訂）

- [ ] **(元 ISSUE-053) docs/figures/context.drawio: ラベル大文字統一**

- [ ] **(元 ISSUE-060) CLAUDE.md / README.md の merged.csv 言及箇所追従**
  - merged.csv → merged_peak.csv 等の最新命名に追従

- [ ] **(元 ISSUE-065) モックアップ「変更あり」フィルターを is_band_change_cand に変更**

---

## 保留

（現在なし）

---

## 履歴

このファイルは作業中リストのみ保持する。完了したものは削除し、git の履歴で追跡する。
