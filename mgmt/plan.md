# モックアップを申請カテゴリ5分類へ追従

## Context

前セッション（ISSUE-156 / ADR-SRS-044）で、`merged_summit.geojson` のフィーチャ分類を
**旧4分類（new / dominant / changed / unchanged）** から
**申請カテゴリ5分類（add / band_change / no_change / delete / review）** へ再編した。
新設計では FR-009 が各フィーチャに `category` プロパティを付与し、消費側（ビューア・XLSX・ZIP）は
そのプロパティを参照するだけで分類できる（導出ロジックを持たない）。

`docs/mockup/viewer_mockup.html`（HTML ビューアのプロトタイプ）は旧4分類のままで仕様と乖離している。
`docs/CLAUDE.md`「モックアップの扱い」に従い、仕様（SRS/ADR）を正として
モックアップを5分類へ同期させる。サンプルデータは仕様外だが、`category` 参照を成立させるため
各フィーチャに `category` を付与し、review の例示も追加する。

対象ファイル: `docs/mockup/viewer_mockup.html`（単一ファイル。`summits_data.js` は不変更）。

## 方針（確定事項）

- **判定方式**: 各サンプルフィーチャに `category` を付与し、ビューアは `p.category` を直接参照する
  （`categorize()` / `peakCategory` / `catByCode()` の導出機構を撤去・簡素化）。ADR-SRS-044 に忠実。
- **review 例示**: 孤立既存サミット（unmatched）1件をサンプルに追加し、新フィルタ・配色・popup を確認可能にする。

## カテゴリ対応マッピング

| 旧 | 新 | 主語/由来 |
|---|---|---|
| new + dominant | `add` | 新設ピーク（peak.match_status ∈ {new, dominant}） |
| changed | `band_change` | matched ∧ is_band_change_candidate=true |
| unchanged | `no_change` | matched ∧ is_band_change_candidate=false |
| （dominant に同梱） | `delete` | delete サミット + 親ピーク→delete サミット線（独立カテゴリ化） |
| （該当なし） | `review` | unmatched 孤立サミット（新規） |

per-feature 割り当て（FR-009 §category 算出ルール）: peak は由来条件で判定。
key_col / activation_zone / delete_zone / prominence_range・matched summit・matched の coord_diff は
**親ピークの category を継承**。delete summit と親ピーク→delete サミット線（mockup では `coord_diff`）は `delete`。
unmatched summit は `review`。

## 実装タスク（すべて Sonnet — HTML/JS の機械的な追従編集）

### 1. サンプルデータに `category` を付与（`viewer_mockup.html` 行 261-361）

- 各フィーチャの `properties` に `category` を追加:
  - new クラスタ（JA/ZZ-A01, JA/ZZ-A02）の peak/AZ/key_col/delete_zone/prominence_range → `"add"`
  - matched 変更なし（JA/SO-001）の peak/AZ/delete_zone/key_col/prominence_range/coord_diff → `"no_change"`
  - matched 変更あり（JA/SZ-001）の peak/AZ/delete_zone/key_col/**matched summit**/prominence_range/coord_diff → `"band_change"`
  - dominant クラスタ（JA/YN-A00）の peak/AZ/key_col/delete_zone/prominence_range → `"add"`、
    **delete summit（JA/ZZ-D02）と peak→delete summit の coord_diff → `"delete"`**
- `window.ALL_SUMMITS` の `_ref` サミット（行 366-368）: push 時に `f.properties.category = "no_change"` を併せて付与
  （変更なしベースラインとして表示するため）。

### 2. review 例示フィーチャを追加（サンプルデータ末尾）

- feature_type=`summit`, match_status=`unmatched`, category=`review` の孤立既存サミット Point を1件追加
  （`summit_code`・`summit_name`/`summit_name_jp`・`sota_alt_m`・`sota_points`・`municipality` を持つ。
  peak/col/zone/line は付けない）。既存クラスタと離れた座標に配置。

### 3. フィルタ UI を5分類化（行 237-243）

- チェックボックス4個→5個。data-cat と表示ラベル:
  `add`=追加 / `band_change`=変更あり / `no_change`=変更なし / `delete`=削除 / `review`=要確認（全て checked）。

### 4. フィルタ配色 CSS（行 60-63）

- `#filters label[data-cat=...]` を5分類へ。配色（HLD 確定前の暫定値、FR-019 の色系統に準拠）:
  - `add` → `#4D7A20`（緑系）
  - `band_change` → `#DC5D04`（橙系）
  - `no_change` → `#888`（灰系）
  - `delete` → `#C8101E`（赤系）
  - `review` → `#7B2FBE`（紫系。既存「陸地最高峰」バッジ色を流用）

### 5. `filterGroups` を5分類化（行 922-927）

- `add / band_change / no_change / delete / review` の5 `L.layerGroup` に置換。

### 6. 導出機構を撤去し `p.category` 直接参照へ（行 929-1212）

- `categorize()`（933-938）・`peakCategory`（930）・カテゴリ事前解決パス（968-973）・
  `catByCode()`（993-995）を削除。
- 各 `feature_type` 分岐で `const cat = p.category || "no_change"` を使う:
  - activation_zone / delete_zone / prominence_range / coord_diff / key_col → `p.category`
  - peak（行 1035）→ `p.category`
  - summit（行 1164）→ `p.category`（`?? (delete?dominant:unchanged)` を撤去）
- peakByCode / colByCode / summitByCode / azByCode のマップ（976-986）は popup 相互参照・
  turf 差分で引き続き使用するため維持。
- summit popup のバッジ（行 1165）: `match_status==="delete"`→`delete` /
  `match_status==="unmatched"`→`review` / それ以外→`matched`。review サミットの popup は
  「要確認（孤立サミット）」が分かる文言にする（rationale 編集 UI は付けない）。
- peak popup（1056-1134）は `p.match_status`（new/matched/dominant）で分岐したまま維持
  （match_status は存続。badge 表記は現状維持で可）。

### 7. 検索の自動フィルタ ON（行 1367）

- `peakCategory[...] ?? (delete?dominant:unchanged)` を、検索対象フィーチャ自身の
  `feature.properties.category || "no_change"` 参照へ置換。検索インデックス（1274-1283）は
  peak + summit を収集しており review サミット（summit）も category=`review` を保持するため対応可。

### 8. saveInput の再描画コメント修正（行 1231）

- category はデータ固定で「変更あり/なし の再分類」は発生しないため、コメントを実態
  （localStorage 保存後の再描画）に合わせて修正。`renderFeatures()` 呼び出し自体は維持。

### 9. エビデンス ZIP の alert 文言更新（行 1412）

- 同梱 GeoJSON 一覧を FR-021 準拠の5ファイルへ:
  `add.geojson` / `delete.geojson` / `changed.geojson` / `unchanged.geojson` / `review.geojson`。

## 検証

1. ブラウザで `docs/mockup/viewer_mockup.html` を開く（`file://` 直開き）。
2. フィルタが5個（追加/変更あり/変更なし/削除/要確認）表示され、各配色が緑/橙/灰/赤/紫であること。
3. 各チェックボックス ON/OFF で対応フィーチャだけが表示/非表示になること。特に:
   - `delete` OFF で削除サミット（JA/ZZ-D02）と親ピーク→delete 線のみ消え、親ピーク（add）は残る。
   - `review` ON/OFF で追加した孤立サミットが切り替わる。
4. 追加した review サミットを検索（コード/名称）→ ジャンプし、非表示時に review フィルタが自動 ON、
   popup に「要確認」が出ること。
5. delete サミットの popup バッジが `delete`、review サミットが `review` であること。
6. エビデンスボタンの alert が add/delete/changed/unchanged/review.geojson を列挙すること。
7. `make lint`（lint-html: djlint）警告ゼロを確認。
8. 完了後、`docs/CLAUDE.md`「ドキュメント更新時のルール」に従い該当ターン内で commit
   （Conventional Commits、本文日本語。push は指示まで不要）。

## 仕様反映の要否（docs/CLAUDE.md「モックアップの扱い」）

- 観測可能挙動（5分類フィルタ・category 参照）は既に SRS（FR-019/FR-021）・ADR-SRS-044 に反映済み。
  本作業はモックアップを仕様へ追従させるのみで、SRS 追記は不要。
- 配色の具体値（紫=#7B2FBE 等）は HLD 確定前の暫定値としてモックアップが保持（HLD 起票時に抽出）。
