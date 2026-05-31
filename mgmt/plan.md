# viewer モックアップ UI 改善（ヘッダー2行化・フィルター2行化・検索機能追加・アイコン幅統一）

## Context

前回コミット `9264a93`（viewer モックアップ機能追加・SRS エクスポート 3 ボタン化・FR-021 新設）後の追加 UI 要望をまとめる。要望はモックアップ（`docs/mockup/viewer_mockup.html`）への 5 項目と、それに伴う SRS（`docs/02_SRS.md` FR-019）・ISSUE 起票。

要望出元: 2026-05-31 セッションの対話で確定。

---

## モックアップ修正（`docs/mockup/viewer_mockup.html`）

### M1: ヘッダーを 2 行化、地理院タイル更新日に `(UTC)` 追記

**現状（L142）:**
```html
<span id="meta">サミットリスト基準日: 2026-01-01 (UTC)　地理院タイル更新日（提供元）: 2026-04-28　解析日時: 2026-05-13 09:00 JST</span>
```

**修正後（イメージ）:**
```
サミットリスト基準日:　　　　　2026-01-01 (UTC)　　解析日時: 2026-05-13 09:00 JST
地理院タイル更新日（提供元）:　2026-04-28 (UTC)
```

**実装方針:**
- `#meta` を `<span>` から `<div>` 化（または内部に 2 つの行ブロックを置く構造）
- 内部レイアウトは **CSS grid 2 列 × 2 行**（ラベル列 + 値列）で日付の縦揃えを実現
  - `grid-template-columns: max-content max-content;`
  - 1 行目右の「解析日時: ...」は値セル内に通常テキストとして併記（追加カラム化はしない）
- `#header` 全体は `align-items: center` のまま（2 行になった `#meta` 全体が縦中央寄せされる）
- L142 のサンプル文字列に `2026-04-28 (UTC)` を反映

### M2: フィルター（表示項目）を 2 行化、順序変更

**現状（L144-150）:** 1 行・順序 `新規 / 削除 / 変更あり / 変更なし`

**修正後:**
- HTML 順序を `新規 → 変更あり → 削除 → 変更なし` に並び替え
- `#filters` を `display: grid; grid-template-columns: auto auto; row-gap: 4px; column-gap: 10px;` に変更
- 自動で:
  - 1 行目: 新規・変更あり
  - 2 行目: 削除・変更なし
- 「表示:」ラベルは `grid-row: 1 / span 2;` で 2 行ぶち抜き縦中央配置
- 既存の checkbox / data-cat / イベントハンドラは無変更

### M3: 検索ボックスを追加（`#meta` と `#filters` の間）

**配置:**
```html
<div id="header">
  <h1>...</h1>
  <div id="meta">...（2行）</div>
  <span class="spacer"></span>
  <div id="search">...</div>          ← 新規
  <span class="spacer"></span>
  <div id="filters">...（2行）</div>
</div>
```
- `.spacer` を 2 つに分け、検索ボックスを左右の余白でセンタリング
- 検索ボックス本体: 幅 280px 固定（CSS で `width: 280px`）

**検索仕様:**
| 入力種別 | 判定 | 動作 |
|---|---|---|
| 数値 + カンマ/空白 + 数値（例: `35.68, 139.76` / `35.68 139.76`）| 正規表現 `^\s*[-+]?\d+\.?\d*\s*[,\s]\s*[-+]?\d+\.?\d*\s*$` で緯度経度判定 | `map.flyTo([lat, lon], 14)` |
| それ以外（文字列）| `GEOJSON_DATA.features` を走査して **部分一致**（大文字小文字無視）| サジェスト・ドロップダウン候補表示 |

**検索対象フィールド（部分一致）:**
- `summit_code`（仮コード含む）
- `name_ja`（山岳名 日本語）
- `name_en`（山岳名 アルファベット）
※ 該当プロパティ名がモックアップのダミーデータと異なる可能性あり → 実装時に GEOJSON_DATA を確認して合わせる

**サジェスト UI:**
- 入力欄下に絶対配置ドロップダウン（`position: absolute`、`z-index: 1000` 以上で地図より前面）
- 各候補に **コード ／ 名前(JA) ／ 名前(EN) ／ 標高** を1行表示
- 候補上限 10 件（先頭一致を優先、それ以外を末尾）
- 候補クリック / ↑↓ + Enter で確定
- 確定時の動作: 該当 feature の座標に `flyTo` + マーカーポップアップ `openPopup()`
  - 該当 feature が現在のフィルターで非表示の場合は警告表示（または対象カテゴリのフィルターを自動 ON にする → 仕様確認は実装フェーズで）

**実装規模:** HTML 約 10 行・CSS 約 30 行・JS 約 80 行

### M4: Leaflet コントロールのアイコン幅を 30×30px に統一

**現状:**
- `.export-control .leaflet-control-layers-toggle`: **26×26px**（L82-92 で明示）
- `.leaflet-bar a`（zoom）: Leaflet デフォルト **26×26px**
- `.leaflet-control-layers-toggle`（レイヤー選択）: Leaflet デフォルト **36×36px** （hamburger アイコン）

**修正後（CSS 追加）:**
```css
/* 3 コントロールのアイコン幅統一 */
.leaflet-bar a,
.leaflet-control-layers-toggle {
  width: 30px !important;
  height: 30px !important;
  line-height: 30px !important;
}
.export-control .leaflet-control-layers-toggle {
  width: 30px;
  height: 30px;
  font-size: 18px;  /* 既存 */
}
```
- レイヤー選択 toggle の hamburger 画像（`background-image`）は Leaflet 標準のままで、`background-size: 30px 30px;` を追加してフィット
- border-radius は Leaflet 標準（4px）のまま

---

## SRS 修正（`docs/02_SRS.md` FR-019）

| # | 該当箇所 | 修正内容 |
|---|---|---|
| S1 | FR-019 メタデータ表示記述 | `gsi_tile_latest_date` 表示時に `(UTC)` 補記を必須とする旨を追記 |
| S2 | FR-019 UI 要件 | **検索機能の要件を新規追記**: 検索対象（サミットコード / 山岳名 和・英 / 緯度経度）、部分一致、サジェスト機能、ヒット時の動作（flyTo + ポップアップ） |
| S3 | FR-019 UI 要件（任意） | ヘッダー表示の **2 行化** とフィルターの **2 行化** は SRS で配置を規定していないため記述しない（モックアップのみ）|

S3 はモックアップ実装のみで SRS には反映しない（仕様優先原則の運用ルールに従う）。

---

## トラッカー更新（`mgmt/tracker/data/issues.json`）

| 操作 | ID | 内容 |
|---|---|---|
| update | ISSUE-064 | パイプラインで `gsi_tile_latest_date` を生成する際の **フォーマット規定**（`YYYY-MM-DD` ＋ ヘッダー表示時に `(UTC)` 付記）を notes に追加 |
| 新規起票 | ISSUE-NEW1 | FR-019 検索機能実装: 実 GeoJSON データでの動作確認（モックアップで合わせた検索仕様を本実装で再現） |

---

## 検証

### モックアップ検証（ブラウザで `viewer_mockup.html` を開く）

- [ ] ヘッダー 1 行目に `サミットリスト基準日: 2026-01-01 (UTC)　解析日時: 2026-05-13 09:00 JST`
- [ ] ヘッダー 2 行目に `地理院タイル更新日（提供元）: 2026-04-28 (UTC)`
- [ ] 「サミットリスト基準日」と「地理院タイル更新日（提供元）」の日付開始位置が縦に揃う
- [ ] フィルター 1 行目に `新規`・`変更あり`、2 行目に `削除`・`変更なし`、「表示:」ラベルは縦中央
- [ ] ヘッダー中央に検索ボックス（幅 280px）が配置される
- [ ] 検索ボックスに `富士` と入力 → サジェスト候補に該当サミットが出る
- [ ] 候補クリック → 地図がそのサミットに flyTo・ポップアップが開く
- [ ] 検索ボックスに `35.68, 139.76` と入力 → Enter で当該座標に flyTo
- [ ] 右上 3 コントロール（レイヤー選択・ズーム・エクスポート）のアイコン幅が同じ（30×30px）
- [ ] レイヤー選択の hamburger アイコンが 30×30 にフィットして崩れない

### SRS 検証
```bash
grep -nE "gsi_tile_latest_date.*UTC|UTC.*gsi_tile_latest_date" docs/02_SRS.md  # 期待: FR-019 で出現
grep -nE "検索|search" docs/02_SRS.md                                          # 期待: FR-019 で新規追記が出現
```

### コミット方針
- 単一コミット: `feat(viewer): モックアップ UI 改善（2行ヘッダー・フィルター2行・検索機能・アイコン統一）`
- モックアップ修正と SRS 修正・トラッカー更新を同コミット（CLAUDE.md「同一作業内のコード＋ドキュメントは同コミット可」）

---

## 関連ファイル

- `docs/mockup/viewer_mockup.html`（M1〜M4 / 約 200 行追加・変更）
- `docs/02_SRS.md`（FR-019: S1・S2 追記）
- `mgmt/tracker/data/issues.json`（ISSUE-064 update + 新規 ISSUE 起票）
- `mgmt/plan.md`（本プランに更新 ※ ExitPlanMode 後、`.claude/plans/` から `mgmt/plan.md` へ移動）

---

## 留意点

- **仕様優先原則**: UI 配置・フィルター順は SRS で規定しないため、モックアップのみで先行実装する（SRS には書かない）。検索機能は機能要件なので SRS に記述する
- **検索プロパティ名のマッピング**: モックアップのダミーデータが `name_ja` / `name_en` / `summit_code` のキー名で揃っているかは実装時に確認（揃っていなければマッピング層を 1 つ挟む）
- **検索ヒット時のフィルター扱い**: 「該当 feature が現在のフィルターで非表示の場合どうするか」は実装時に最終確認（候補に「(非表示中)」と表示するだけ・自動 ON にする、の 2 案）
- **アイコン幅 30px の根拠**: 26px だと現状の zoom/export と揃うがレイヤー選択 hamburger アイコンが小さく見える可能性。30px なら 3 つとも違和感なし。実装後に微調整可
- **モデル切り替え**: 実装フェーズで Sonnet 4.6 に `/model` 切り替えを促す（feedback_model_switching.md に基づく）
