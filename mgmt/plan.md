# モックアップ 既存サミットを Canvas 描画へ（sotlas 風・軽量化）

## Context

divIcon に戻しても、変更なし既存サミット7067件が「1個ずつの DOM 要素」のままで重い。sotlas は同じデータをベクター/Canvas に直接描画して DOM 要素を作らないため軽い。これに倣い、**変更なし既存サミット7067件を Canvas 描画（`L.circleMarker` + `preferCanvas`）に切り替え**、軽量化する。形状（●）・pt 色・ズーム連動サイズは保つ。解析結果（peak▲/col▽/sample summit●）は件数が少なく形状区別が重要なので **divIcon のまま維持**（ハイブリッド）。ユーザーは実機で見て最終判断したい意向。

## 方式（ユーザー向けの言い方: 「sotlas と同じ軽い描き方」）

| 対象 | 描き方 | 理由 |
|---|---|---|
| 変更なし既存サミット 7067件（`ALL_SUMMITS` 由来・`match_status="matched"`） | **Canvas 円**（pt色・ズーム連動サイズ） | DOM を作らず軽い。元々●なので見た目はほぼ同じ |
| 解析結果（new/dominant/changed/delete の peak/col/summit） | **現状 divIcon（▲▽●）維持** | 少数・形状区別が申請確認に重要 |

## 実装（`docs/mockup/viewer_mockup.html`）

1. **Canvas 描画を有効化**: `L.map` の options に `preferCanvas: true` を追加（L415付近）。
2. **参照データ印を付ける**: `ALL_SUMMITS` をマージする箇所（`GEOJSON_DATA.features.push` 付近）で、各 feature の properties に `_ref = true` を付与してから push。
   ```js
   window.ALL_SUMMITS.features.forEach(f => f.properties._ref = true);
   GEOJSON_DATA.features.push(...window.ALL_SUMMITS.features);
   ```
3. **renderFeatures の summit 分岐を分岐**（L1107付近）:
   - `p._ref === true`（変更なし既存サミット）→ `L.circleMarker([lat,lon], { radius: summitRadius(), color: sotaColor, fillColor: sotaColor, fillOpacity: 0.85, weight: 1 })` で描画。popup は現状の参照サミット用（コード/読み/標高/エリア＋全データ）。`_refMarkers` 配列に push（ズーム連動用）。
   - それ以外（解析結果サミット）→ 現状の `L.marker` + `sotaIcon`（divIcon）を維持。
4. **ズーム連動サイズ**:
   ```js
   const _refMarkers = [];
   function summitRadius(){ const z=map.getZoom(); return z<=6?2 : z<=9?3 : z<=12?4 : 5; }
   map.on('zoomend', () => { const r=summitRadius(); _refMarkers.forEach(m=>m.setRadius(r)); });
   ```
5. circleMarker も `filterGroups[cat]`（unchanged）に追加し、既存のフィルター・検索（ユニーク化済み）と整合させる。

## 継続（触らない）

地図中心=明石・線太く・検索拡張・仮コード `ZZ/ZZ-*`・ピーク↔コルジャンプ・全データ折りたたみ・AZ緑/delete赤一律・AZを上に（azPane）。

## 検証（ブラウザで `docs/mockup/viewer_mockup.html` を開く）

- **軽さ**: 変更なしサミット表示時のパン・ズームが体感で軽いか（最重要・実機判断）。
- 変更なしサミットが●（pt色）で表示され、ズームアウトで小さく・ズームインで大きくなる。
- 解析結果は ▲▽● のまま。クリックで両方とも popup が出る。
- 検索で変更なしサミット（北岳等）が選択でき、対象へ飛ぶ。

## 補足

- 効果が想定通りなら、この方式を SRS の FR-013 ビューア表示仕様（描画方式・ズーム連動）として反映する候補。
- circleMarker（円）以外の形（▲▽）も Canvas で描けるが、まず最小実装（既存サミット=円）で軽さを確認し、必要なら拡張。
