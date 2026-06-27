# モックアップのアイコンを地理院シンボルに差し替える

## Context（なぜ）

`viewer_mockup.html` は現在 FontAwesome（`fa-caret-up`▲ / `fa-caret-down`▽ / `fa-circle`●）を
Leaflet divIcon で描画している。FontAwesome のアイコンフォントは「画像 URL」ではないため、
GeoJSON を地理院地図サイトにドラッグ&ドロップする方式（`_iconUrl`）では再現できない（ADR-SRS-026:49）。

申請エビデンスを地理院地図上で見せる用途を見据え、**ビューアのアイコンも地理院提供シンボル画像
（`https://maps.gsi.go.jp/portal/sys/v4/symbols/{番号}.png`）に統一**する。番号は旧申請データ
`ref/geojson_v31/ja0_v31.geojson` を踏襲（サミット）＋ユーザー指定（ピーク・コル）で確定済み。

**今回のスコープは `viewer_mockup.html`（モックアップ）の見た目修正のみ。** 本番出力の対応は
HLD で色スキームを固めてからまとめて行う（下記「先送り項目」参照）。

## 確定アイコンマッピング（pt → symbols番号・全18種・実在200確認済み）

| pt | ピーク（▲） | コル（▽） | サミット（●＝v31踏襲） |
|---|---|---|---|
| 1pt | 398 | 697 | 826 |
| 2pt | 077 | 093 | 102 |
| 4pt | 400 | 699 | 828 |
| 6pt | 078 | 094 | 103 |
| 8pt | 079 | 095 | 104 |
| 10pt | 076 | 092 | 101 |

- URL は `https://maps.gsi.go.jp/portal/sys/v4/symbols/{番号}.png`
- **頭ゼロは3桁必須**（`077.png`=200、`77.png`=404。`092/093/094/095/077/078/079/076` が該当）

## なぜ output_geojson.py を今回やらないか（判断記録）

1. output_geojson.py 自体が SRS 未追従の暫定コード（todo.md に未対応追従タスク多数: 旧名 `merged.csv`・`is_tile_top`・`col→key_col` 等）
2. GeoJSON 生成本体は `mesh_analyze.c` / `merge.py` にも分散（todo.md L41-45）。単独修正は不整合を生む
3. 現アイコンは `match_status` 別（matched/new/deleted）3 種。新方式は `feature_type` 別×標高バンドで**データ構造が別物**
4. 色スキームは「HLD で規定」が既定（SRS:720/800・todo.md L42）。HLD 未作成

→ 本番反映は HLD 確定後に `mesh_analyze.c`/`merge.py`/`output_geojson.py`＋config.ini 化をまとめて実施。

## 変更対象: `docs/mockup/viewer_mockup.html`（単一ファイル）

### 1. アイコン定数を冒頭に追加（現 `pointsToColor` 付近 L350-358）
```js
// 地理院シンボルアイコン（pt → symbols番号）。本番(mesh_analyze.c/merge.py)では config.ini で上書き可能にする予定
const GSI_SYMBOL_URL = n => `https://maps.gsi.go.jp/portal/sys/v4/symbols/${n}.png`;
const ICON_SYMBOLS = {
  peak:   { 1:'398', 2:'077', 4:'400', 6:'078', 8:'079', 10:'076' },
  col:    { 1:'697', 2:'093', 4:'699', 6:'094', 8:'095', 10:'092' },
  summit: { 1:'826', 2:'102', 4:'828', 6:'103', 8:'104', 10:'101' },
};
const ICON_PX = 24;
```

### 2. アイコン生成関数を L.divIcon → L.icon に置換（L360-377）
`peakIcon`/`colIcon`/`sotaIcon` を、色引数ではなく **pt 引数**を取り地理院シンボル画像を返す形へ:
```js
function gsiIcon(kind, pts) {
  const num = ICON_SYMBOLS[kind][pts] || ICON_SYMBOLS[kind][10];
  return L.icon({ iconUrl: GSI_SYMBOL_URL(num), iconSize: [ICON_PX, ICON_PX], iconAnchor: [ICON_PX/2, ICON_PX/2] });
}
function peakIcon(pts) { return gsiIcon('peak', pts); }
function colIcon(pts)  { return gsiIcon('col', pts); }
function sotaIcon(pts) { return gsiIcon('summit', pts); }
```
- 不要になる定数 `WHITE_OUTLINE`・`CIRCLE_PX`・`TRIANGLE_PX` を削除（アイコン以外で未使用）

### 3. 呼び出し側を pt 渡しに変更（3箇所）
- L968 `peakIcon(color)` → `peakIcon(p.points)`
- L1069 `colIcon(color)` → `colIcon(p.points)`
- L1112 `sotaIcon(sotaColor)` → `sotaIcon(p.sota_points)`

### 維持するもの（変更しない）
- `pointsToColor()`：ポリゴン（activation_zone/delete_zone）色・ポップアップ内装飾・フィルタ色で引き続き使用
- **ポップアップ内の `<i class="fa-caret-*">` 装飾アイコン**：ビューア専用 HTML なので FontAwesome のまま
- フィルタラベル色（CSS・カテゴリ色）

## 先送り項目の追跡（実装フェーズで `mgmt/todo.md` に追記）

issue 登録は不要（決定済み方針の反映作業であり、未解決の設計判断＝課題ではない。
発生ステージ SRS（ADR-SRS-026/013）・対応予定ステージ HLD）。すべて todo.md に一本化する。

todo.md に追記する内容（**元 ISSUE-106 タスク L41-45 に紐づけ**・HLDフェーズ実施）:
- 確定18番号マッピング（上表）を明記。モックは JS 定数で先行実装済みである旨を注記
- HLD の色スキームに18番号体系（中心データ／ビューア用・pt 別）を記載
- 中間 GeoJSON（`merged_peak.geojson`・SOTA ポイント未割当）は標高ベースの番号スキームを HLD で別途規定
  （ADR-SRS-026 L50-51 の既定に沿う）
- ADR-SRS-026 L49（FontAwesome 非対応で厳密再現せず）の記述を、地理院シンボル統一方針に合わせて整える
- 本番コード反映（`mesh_analyze.c`/`merge.py`/`output_geojson.py`）＋ `params/config.ini` `[icons]` で設定可能化（前例 ADR-SRS-031）

## 検証

1. `docs/mockup/viewer_mockup.html` をブラウザで直接開く（ローカル）
2. 地図上で確認:
   - ピーク=▲系(398/077/400/078/079/076)、コル=▽系(697/093/699/094/095/092)、サミット=●系(826/102/828/103/104/101)
   - 各マーカー図柄が pt に応じ切替（富士山=10pt・新規580m=2pt 等で目視）
   - ポップアップが従来どおり開き、装飾アイコン色が一致
3. アンカー/サイズ（[12,12]・24px）の見た目を確認し、ずれていれば微調整
