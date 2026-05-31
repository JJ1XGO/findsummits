# 国土地理院タイル出典表示の整備

## Context

国土地理院標高タイル（DEM5a/5b/5c, DEM10b）は[地理院タイル一覧](https://maps.gsi.go.jp/development/ichiran.html)で「2. 基本測量成果以外で出典記載のみで利用可能」（区分2）に分類されており、利用には**測量法に基づく申請は不要**である。ただし[国土地理院コンテンツ利用規約](https://www.gsi.go.jp/kikakuchousei/kikakuchousei40182.html)に基づき:

- **出典の明示**が必要（記載例:「国土地理院」または「地理院タイル」＋一覧ページへのリンク）
- 編集・加工した二次データを公開する場合は「**加工して作成**」の明示が必要（例:「地理院タイル（標高タイル）を加工して作成」）

現状は HTML ビューア（`docs/mockup/viewer_mockup.html:384-400`）の Leaflet attribution にのみ `© 国土地理院` リンクがあり、それ以外の成果物（README、merged.geojson、merged.csv、申請書XLSX以外のXLSX、配布版 GeoJSON）には出典情報が一切埋め込まれていない。SRS にも出典表示要件は明文化されていない（FR-006 等で attribution 表示の言及はあるが、ビューア背景タイルに限定）。

本タスクは規約遵守と再配布時の追跡性を確保するため、**URD に出典表示の要件を新設し、SRS で各成果物の出典埋め込み方法を規定したうえで、README と主要な成果物に出典表示を実装する**。

## 対応範囲（ユーザー確認済み）

- URD: UR-011 を新設
- SRS: 出典表示要件を明文化（実装の前提）
- 実装: 中央データ＋公開資材に徹底
  - README（プロジェクト全体の出典宣言）
  - `merged.geojson` の `metadata` フィールド（ADR-013 で中央データと規定済み）
  - 配布版 GeoJSON（HTML ビューアとともに公開）
  - `merged.csv` のヘッダコメント
  - HTML ビューアからエクスポートされる XLSX（申請書テンプレ XLSX は SOTA 側書式のため対象外）
- HTML ビューアの Leaflet attribution は既実装のため変更不要

## 修正対象ファイルと変更内容

### 0. ADR: `docs/decisions/ADR-014-gsi-tile-attribution-policy.md`（新設）

UR-011 新設および出典埋め込み方針の根拠を ADR として記録する。

**フォーマット**（CLAUDE.md「ADR 管理ルール」に準拠）:

```markdown
| 状態 | 採用・未実装 |
| 決定日 | 2026-05-31 |

## Context
- 国土地理院標高タイルは「区分2: 基本測量成果以外で出典記載のみで利用可能」（参照: ref/SOURCES.md）
- 規約上、編集・加工した二次データを公開する場合「加工して作成」の明示と出典記載が必要
- 現状は HTML ビューアの Leaflet attribution のみで、その他成果物には出典情報がない

## Decision
1. URD に UR-011 を新設し、生成・公開する成果物への出典・加工明示義務をユーザー要件として位置付ける
2. 埋め込み範囲は「中央データ＋公開資材に徹底」:
   - README（プロジェクト全体宣言）
   - merged.geojson の metadata
   - 配布版 GeoJSON
   - merged.csv のヘッダコメント
   - HTML ビューア出力 XLSX
3. 標準文面を定義:
   - 短形式: `地理院タイル（標高タイル）を加工して作成。出典: 国土地理院 (https://maps.gsi.go.jp/development/ichiran.html)`
   - GeoJSON metadata: `{"attribution": ..., "source_url": ..., "license": ...}`

## Alternatives
- **A. UR-010 を拡張して一本化**: 「規約遵守」を取得時から成果物公開時まで包含。却下理由 = 取得時の責務と公開時の責務はスコープが異なり、トレーサビリティが不明瞭になる
- **B. README のみで対応**: 派生データ単体配布時に出典が失われ規約違反のリスク。却下
- **C. ADR 不要として暗黙対応**: 文面・配置の判断履歴が残らず、将来の見直し時に再検討コストが発生。却下

## Consequences
- 各成果物のフォーマットに出典フィールドが追加される（GeoJSON は RFC 7946 foreign members として許容）
- merge.py / output_geojson.py / viewer_mockup.html の出力ロジックに小変更が必要
- merged.csv に `#` コメント行が入るため、後段で読む output_geojson.py はスキップ処理が必要
- 申請書 XLSX（SOTA 側書式）は対象外。SOTA 申請のエビデンスは GeoJSON / CSV 側でカバー
```

### 1. URD: `docs/01_URD.md`

UR-011 を新設（UR-010 の直下に追記）:

```
| UR-011 | 本ツールが生成・公開する成果物（README、GeoJSON、CSV、HTML ビューア、ビューアからエクスポートする XLSX 等）には、国土地理院コンテンツ利用規約に従い、出典（「国土地理院」または「地理院タイル」と一覧ページへのリンク）および「加工して作成」の旨を明示すること |
```

理由・スコープに必要な補記をスコープ外との重複を避けつつ追加。

### 2. SRS: `docs/02_SRS.md`

出典表示を機能要件として明文化（FR-013 周辺、出力ファイル仕様の文脈で追加）:

- 各出力（merged.geojson / merged.csv / 配布GeoJSON / ビューア出力XLSX）について、出典文字列の埋め込み位置と文面を明記
- 標準文面:
  - 短形式: `地理院タイル（標高タイル）を加工して作成。出典: 国土地理院 (https://maps.gsi.go.jp/development/ichiran.html)`
  - GeoJSON 用: `{"attribution": "地理院タイル（標高タイル）を加工して作成。出典: 国土地理院", "source_url": "https://maps.gsi.go.jp/development/ichiran.html"}`
- HTML ビューアの Leaflet attribution は SRS 661-677 行で既に規定済みのため、対応文面の整合だけ確認

本要件の根拠 ADR は `docs/decisions/ADR-014-gsi-tile-attribution-policy.md`（新設）に記録する。

### 3. README: `README.md`

末尾に「データソース・出典」セクションを追加:

```markdown
## データソース・出典

本ツールは [国土地理院](https://www.gsi.go.jp/) が提供する [地理院タイル](https://maps.gsi.go.jp/development/ichiran.html)（標高タイル DEM5a / DEM5b / DEM5c / DEM10b）を加工して作成しています。

- 出典: 国土地理院ウェブサイト (https://maps.gsi.go.jp/development/ichiran.html)
- 利用規約: [国土地理院コンテンツ利用規約](https://www.gsi.go.jp/kikakuchousei/kikakuchousei40182.html)

本ツールが生成する GeoJSON / CSV / XLSX には地理院タイルの標高値から解析した派生データが含まれます。再配布時も上記出典の明示をお願いします。
```

「ライセンス」セクションは既存（GPL-3.0）を維持。

### 4. GeoJSON 出力: `scripts/output_geojson.py`

154 行目の `FeatureCollection` 構築箇所を修正:

```python
geojson = {
    "type": "FeatureCollection",
    "metadata": {
        "attribution": "地理院タイル（標高タイル）を加工して作成。出典: 国土地理院",
        "source_url": "https://maps.gsi.go.jp/development/ichiran.html",
        "license": "https://www.gsi.go.jp/kikakuchousei/kikakuchousei40182.html",
    },
    "features": features,
}
```

`metadata` プロパティは GeoJSON 仕様（RFC 7946）の foreign members として許容される（パースに支障なし）。

### 5. CSV 出力: `scripts/merge.py`

`merged.csv` の先頭にコメント行（`#` 始まり）を3行追加:

```
# Source: 国土地理院 地理院タイル（DEM5a/5b/5c/DEM10b）
# Attribution: 地理院タイル（標高タイル）を加工して作成
# License: https://www.gsi.go.jp/kikakuchousei/kikakuchousei40182.html
```

`csv.DictWriter` 利用箇所を確認し、ヘッダ行の直前にコメントを書き込む。後段の `output_geojson.py` で読む際に `#` 行をスキップ処理する分岐を追加（数行）。

### 6. HTML ビューア出力 XLSX

`viewer_mockup.html` の XLSX エクスポート処理に、別シート「出典」または1行目のヘッダ上に出典文字列を埋め込む。具体的な実装箇所は XLSX エクスポートのロジック確認後に決定。

## 検証手順

1. **URD/SRS 整合確認**: `docs/01_URD.md` と `docs/02_SRS.md` を読み、UR-011 → SRS 要件 → 各成果物の対応がトレース可能であること
2. **README 表示確認**: GitHub 上で README が想定通り表示されること（編集後即時 commit）
3. **GeoJSON**: `venv/bin/python3 scripts/output_geojson.py ...` 実行 → 出力 JSON を `jq '.metadata'` で確認
4. **CSV**: merge.py 実行 → merged.csv 先頭3行が `#` コメントになっていること
5. **CSV → GeoJSON 連携**: output_geojson.py が `#` コメント行を正しくスキップして既存と同じレコード数を生成すること
6. **HTML ビューア XLSX**: ビューアでデータを読み込み XLSX エクスポート → 出力ファイルに出典シート/行が含まれること

## 作業順序

1. ADR-014 を先に作成（判断根拠の確定）
2. ドキュメント（URD → SRS）を確定し ADR を相互参照
3. 確定後、ユーザーに `/model` で Sonnet 切り替えを促す（記憶: `feedback_model_switching.md`）
4. README → output_geojson.py → merge.py → viewer_mockup.html の順で実装
5. 一連の作業完了後にまとめて commit（記憶: `feedback_commit_at_end.md`）

## 関連 ISSUE 登録

実装と並行して `mgmt/tracker/track.py issue add` で本作業を登録する（カテゴリ: 規約遵守 / ステージ: SRS）。
