# ADR-URD-014: 国土地理院タイル出典表示の方針

| 状態 | 採用・未実装 |
| 決定日 | 2026-05-31 |

## Context

国土地理院標高タイル（DEM5a/5b/5c, DEM10b）は[地理院タイル一覧](https://maps.gsi.go.jp/development/ichiran.html)で「2. 基本測量成果以外で出典記載のみで利用可能」（区分2）に分類されており、利用に測量法に基づく申請は不要である。

ただし[国土地理院コンテンツ利用規約](https://www.gsi.go.jp/kikakuchousei/kikakuchousei40182.html)（PDL1.0 相当）に基づき、以下の義務がある：

1. **出典の明示**: 「国土地理院」または「地理院タイル」と一覧ページへのリンクを記載
2. **加工の明示**: 編集・加工して二次データを作成・公開する場合は「加工して作成」の旨を明示（例：「地理院タイル（標高タイル）を加工して作成」）

本プロジェクトで生成する成果物（GeoJSON / CSV / XLSX / README）は標高タイルの標高値を解析した**派生データ**に当たるため、これらへの出典・加工明示が規約上の義務である。

URD に既存の [UR-010](../10_URD.md#ur-010)（タイル取得時の規約遵守）は取得時の責務を規定するにとどまっており、成果物公開時の出典表示義務は含まれていない。そのため、出典表示要件を独立したユーザー要件として位置付ける（→ [UR-011](../10_URD.md#ur-011) 新設: [URD](../10_URD.md)）。

### 現状のギャップ

- HTML ビューア（`docs/mockup/viewer_mockup.html`）の Leaflet attribution には `© 国土地理院` リンクを実装済み（SRS [FR-021](../20_SRS.md#fr-021-申請エビデンス-zip-生成) 規定）
- その他の成果物（README / `merged_summit.geojson` / `merged.csv` / ビューア出力 XLSX）には出典情報がない

## Decision

### 1. UR-011 を URD に新設

取得時の規約遵守（[UR-010](../10_URD.md#ur-010)）とは独立したユーザー要件として、成果物への出典・加工明示義務を [UR-011](../10_URD.md#ur-011) として定義する。

### 2. 出典埋め込みの範囲

「中央データ＋公開資材に徹底」方針を採用する：

| 成果物 | 出典埋め込み方法 |
|---|---|
| `README.md` | 「データソース・出典」セクションとして明示 |
| `merged_summit.geojson`（中央データ） | top-level `metadata` プロパティ |
| 配布版 GeoJSON（ビューアと一緒に配布） | 同上（output_geojson.py で出力） |
| `merged.csv` | ファイル先頭の `#` コメント行（3行） |
| ビューア出力 XLSX | 「出典」シートまたは先頭行 |
| HTML ビューア背景タイル attribution | 既実装（変更不要） |

申請書 XLSX（SOTA 側書式: `SOTA-Summit-list-revision-request.xlsx`）は SOTA 日本支部が指定するテンプレートを使用するため対象外。エビデンスは GeoJSON / CSV 側でカバーする。

### 3. 標準文面

| 用途 | 文面 |
|---|---|
| 短形式（CSV コメント・XLSX） | `地理院タイル（標高タイル）を加工して作成。出典: 国土地理院 (https://maps.gsi.go.jp/development/ichiran.html)` |
| GeoJSON metadata キー | `attribution`（文面）+ `source_url`（URL）+ `license_url`（規約URL） |
| README セクション | 本文で「加工して作成」と明示し、出典URL・利用規約URLをリンク形式で記載 |

### 4. GeoJSON 仕様への適合

`metadata` を top-level に置く構造は RFC 7946 の foreign members として許容される。`FeatureCollection` の必須プロパティ（`type`, `features`）は変更しない。

## Alternatives

### A. UR-010 を拡張して一本化（不採用）

既存の [UR-010](../10_URD.md#ur-010)「タイル取得時の規約遵守」を「取得時および成果物公開時の規約遵守」に拡張する案。

却下理由：取得時の責務（サーバ負荷・User-Agent 付与など）と公開時の責務（出典表示・加工明示）はスコープが異なる。一本化するとトレーサビリティが不明瞭になり、SRS の各出力機能要件から [UR-010](../10_URD.md#ur-010) を参照する際に混乱が生じる。

### B. README のみで対応（不採用）

プロジェクト全体の出典宣言を README に一括記載し、各成果物への埋め込みは省略する案。

却下理由：GeoJSON / CSV ファイルが単体で配布・参照される場合に出典情報が失われ、規約違反リスクが残る。ADR-SRS-013 で `merged_summit.geojson` が中央成果物と位置付けられており、中央データ自体に出典を持たせることが自然かつ確実。

### C. 全成果物に均一に埋め込む（不採用）

per-mesh 中間 CSV・ログファイル等の内部成果物も含めてすべてに出典を埋め込む案。

却下理由：内部成果物は公開対象でなくオーバーエンジニアリング。メンテナンスコストに対してリターンがない。

## Consequences

- `scripts/output_geojson.py`（L154）の FeatureCollection 構築に `metadata` キーを追加する小変更が必要
- `scripts/merge.py` の CSV 出力にヘッダコメント行（`#`）を追加する小変更が必要
- `merged.csv` に `#` コメント行が入るため、`output_geojson.py` で CSV を読み込む場合は `#` 行をスキップする処理が必要
- `docs/mockup/viewer_mockup.html` の XLSX エクスポート処理に出典シートを追加する小変更が必要
- 申請書 XLSX（SOTA 側書式）は対象外のため変更不要
- HTML ビューアの Leaflet attribution は実装済みのため変更不要
