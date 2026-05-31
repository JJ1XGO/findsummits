# viewer モックアップ機能追加と SRS 整合

## Context

viewer モックアップ（`docs/mockup/viewer_mockup.html`）に複数の UI 改善要望が出た。本セッション前半で次の 3 件は既に編集済み：

- 地理院（標準）/（淡色）併記化
- メッシュコードラベルのフォントサイズ可変（ズーム連動）
- メッシュコードラベルをビューポート ∩ メッシュ にクランプ（ズームイン時も常時表示）

ここから扱うのは UI の残作業 4 件と、それに伴う SRS（`docs/02_SRS.md` FR-011/FR-012/FR-019/FR-020）の整合修正。エクスポート機能はユーザー再検討の結果、3 ボタン構成に刷新する。

## モックアップ修正（`docs/mockup/viewer_mockup.html`）

### M1: ヘッダー表示の拡張
- 現状: `サミットリスト基準日: 2026-01-01　解析日時: 2026-05-13 09:00 JST`
- 修正後: `サミットリスト基準日: 2026-01-01 (UTC)　地理院タイル更新日（提供元）: 2026-04-28　解析日時: 2026-05-13 09:00 JST`
- ダミーデータ `GEOJSON_DATA.metadata` に `gsi_tile_latest_date` フィールド追加（モックアップ用サンプル値）

### M2: レイヤー選択アイコンを `topright` へ移動（zoom の上）
- 現状: `L.control.layers(..., { position: "topleft" })`（L449）
- 修正後: `position: "topright"`、かつ `L.control.zoom()` より **先に** addTo（Leaflet は同一 position で追加順に上から並ぶ）
- 並び（topright 上から）: レイヤー選択 → ズーム → エクスポート

### M3: コントロール展開時挙動（アイコン残し・右隣縦展開）
- 現状: 展開時 toggle アイコンが `display: none` に
- 修正後 CSS 方針:
  - `.export-control`（既存）と `.leaflet-control-layers`（デフォルト）の両方に `display: flex; flex-direction: row; align-items: flex-start;` 等を当てる
  - 展開時もアイコン（`.leaflet-control-layers-toggle`）を表示維持
  - リスト（`.leaflet-control-layers-list`）はアイコン右隣に縦展開
- 対象: レイヤーコントロール／エクスポートコントロール 両方

### M4: エクスポートメニューを 3 サブボタン化
- 現状: 「申請書エクスポート」「公開用エクスポート」の 2 サブボタン
- 修正後（3 サブボタン）:

| # | ボタンラベル | 出力 | 説明 |
|---|---|---|---|
| 1 | 申請書 | `<basename>.xlsx`（単独）| SOTA-Summit-list-revision-request 準拠の XLSX |
| 2 | 申請エビデンス | `<basename>.zip`（同梱: `merged_summit_revised.xlsx` + `new.geojson` + `dominant.geojson` + `changed.geojson` + `unchanged.geojson`）| 申請内容の裏付け資料一式 |
| 3 | 公開用 HTML | `<basename>.html`（単独）| 閲覧専用 HTML |

- モックアップではボタン押下時の動作は `alert()` で出力概要を示す（実 XLSX/ZIP 生成は実装フェーズ）

## SRS 修正（`docs/02_SRS.md`）

| # | 該当箇所 | 修正内容 |
|---|---|---|
| S1 | FR-019 L660 「背景タイル切り替え機能」 | タイル一覧に **地理院淡色地図** を追加（既にモックアップで実装済み・SRS が追従） |
| S2 | FR-019 L665-667 「埋め込みデータの metadata から〜表示」 | ヘッダー表示項目に **地理院タイル更新日（提供元）** を追加。サミットリスト基準日に **(UTC)** 補記。`metadata` に新フィールド `gsi_tile_latest_date` を追加する旨を明記 |
| S3 | FR-019 L683-689 「エクスポート」セクション | **3 ボタン構成に刷新**（申請書 / 申請エビデンス / 公開用 HTML）。各ボタンの責務・出力ファイル種別を明示 |
| S4 | FR-011 概要・出力 | 既存通り「ブラウザ内 SheetJS で生成・単独ダウンロード」。zip 同梱はしない旨を明確化 |
| S5 | FR-012 L720-727 概要・出力 | 出力先を `$DATA_DIR/results/...` から **「HTML ビューアからブラウザダウンロード（申請エビデンス zip 内）」** に変更。生成方法を「ブラウザ内で生成」と明示。merged_summit_revised.xlsx の zip 内パスを規定 |
| S6 | FR-020 L760-769 公開用 HTML ビューア生成 | 出力を **公開用 HTML 単独** に変更（GeoJSON 言及は削除し S7 へ移動） |
| S7 | 新規セクションまたは FR-019 末尾「申請エビデンス zip 仕様」 | zip 同梱 5 ファイルの内訳・4 GeoJSON のカテゴリ判定ロジックを定義 |

### S7: 4 GeoJSON のカテゴリ判定（提案）

| ファイル | 判定ロジック | 含むフィーチャ |
|---|---|---|
| `new.geojson` | peak の `match_status="new"` | 該当 peak とその関連 features（col / activation_zone / delete_zone / prominence_range） |
| `dominant.geojson` | peak の `match_status="dominant"` | 該当 peak と関連 features ＋ 対応する `match_status="delete"` の summit feature |
| `changed.geojson` | peak の `match_status="matched"` かつ `is_band_change_candidate=true` | 該当 peak と関連 features ＋ 対応する `match_status="matched"` の summit feature |
| `unchanged.geojson` | peak の `match_status="matched"` かつ `is_band_change_candidate=false` | 該当 peak と関連 features ＋ 対応する summit feature |

- `metadata`（summitslist_date / generated_at / gsi_tile_latest_date）は各 GeoJSON に複製
- モックアップの「変更あり」フィルター（現状: localStorage に編集ありで判定）も `is_band_change_candidate` ベースに揃えるかは別タスク（本プラン外）

## モックアップに反映しない（SRS 規定なし・UI 細部）

- レイヤー選択アイコン配置（SRS は配置を規定していない）
- アイコン展開挙動（CSS のみの問題）
- メッシュラベルのフォントサイズ可変／ビューポート内クランプ（SRS は表示方式を規定していない）

これらは本プランで mockup だけ更新し、SRS には反映しない。

## 重要な後続課題（本プラン外）

| ID | 内容 |
|---|---|
| 新規 ISSUE-A | パイプライン側で `gsi_tile_latest_date` を `merged.geojson.metadata` に格納する実装（タイルキャッシュの mtime を集計）。`scripts/prefetch_tiles.py` or `scripts/merge.py` のどちらで集計するかは別途設計 |
| 既存 ISSUE-062（FR-012 実装）| スコープを「ブラウザ内で `merged_summit_revised.xlsx` 生成 → 申請エビデンス zip に同梱」へ更新 |
| 新規 ISSUE-B | 4 GeoJSON 分割生成（ブラウザ内）の実装。ISSUE-062 と統合可 |
| 新規 ISSUE-C | モックアップ「変更あり」フィルターを `is_band_change_candidate` ベースに合わせる |
| 既存 ISSUE-059（実装側ファイル名追従）| 影響なし（既存スコープのまま） |

## 検証

### モックアップ検証（ブラウザで `viewer_mockup.html` を開く）
- [ ] ヘッダーに 3 項目が `(UTC)` 付き・タイル更新日付き で表示される
- [ ] レイヤー選択アイコンが画面右上、ズームボタンの上にある
- [ ] レイヤー選択／エクスポート両方で、アイコン上にマウスホバー → アイコンが残ったままリストが右隣に縦展開
- [ ] エクスポートアイコンに 3 サブボタン（申請書 / 申請エビデンス / 公開用 HTML）がある
- [ ] 各サブボタン押下で alert に出力概要が出る

### SRS 検証
```bash
grep -n "地理院淡色" docs/02_SRS.md           # 期待: FR-019 で出現
grep -n "gsi_tile_latest_date" docs/02_SRS.md  # 期待: FR-019 メタデータ箇所
grep -n "(UTC)" docs/02_SRS.md                 # 期待: サミットリスト基準日記述
grep -n "申請エビデンス" docs/02_SRS.md        # 期待: FR-019/FR-012/FR-020/S7 で複数箇所
grep -n "merged_summit_revised\.xlsx" docs/02_SRS.md  # 期待: FR-012 + S7（zip 内パス）
grep -nE "公開用エクスポート|公開用 HTML" docs/02_SRS.md  # 期待: FR-019/FR-020 で残存・GeoJSON 同梱記述は消滅
```
- FR-019 のエクスポート節が 3 ボタン構成で記述されている
- FR-011 単独 DL／FR-012 zip 同梱／FR-020 単独 DL が明示されている
- 既存 GeoJSON 公開用同梱の記述が削除されている

### コミット
- 単一コミット: `docs(srs): viewer エクスポート 3 ボタン化と地理院タイル更新日メタデータ追加`
- モックアップ修正と SRS 修正を同コミットに含める（CLAUDE.md「同一作業内のコード＋ドキュメントは同コミット可」）
- ISSUE トラッカー更新（ISSUE-062 スコープ調整・新規 ISSUE 起票）は別コミットとして分離

## 関連ファイル

- `docs/mockup/viewer_mockup.html`（モックアップ。M1〜M4）
- `docs/02_SRS.md`（SRS。S1〜S7）
- `mgmt/tracker/data/issues.json`（ISSUE-062 update + 新規 ISSUE 起票）
- `mgmt/plan.md`（本プランに更新）

## 留意点

- **仕様優先原則**: モックアップ（実装の一種）が先行している箇所（淡色追加・メッシュラベル可変・クランプ）は SRS に反映するが、SRS が UI 配置を規定しないものは SRS に書かない
- **gsi_tile_latest_date のメタデータ追加**は SRS では「ヘッダー表示要件」として記述するに留め、生成元の Python パイプライン改修は新規 ISSUE で扱う
- **「変更あり」カテゴリの定義**は SRS では `is_band_change_candidate=true` ベース（FR-012 と整合）で記述。モックアップフィルターは現状 localStorage ベースで定義が乖離しているが、本プランでは触れず別 ISSUE で扱う
