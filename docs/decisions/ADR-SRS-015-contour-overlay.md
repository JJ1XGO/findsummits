# ADR-SRS-015: HTML ビューアへの等高線オーバーレイ追加

| 状態 | 採用・モックアップ実装済み |
| 決定日 | 2026-06-02 |

## Context

HTML ビューア（[FR-013](../20_SRS.md#fr-013-html-ビューア生成)）の背景タイルには国土地理院標準地図・地理院淡色・OSM・OpenTopoMap の4種が選択可能だが（[ADR-SRS-006](ADR-SRS-006-viewer-background-tile-selection.md)）、OSM は等高線を含まず山岳部での視認性が低い。ADR-SRS-006 Consequences 行 47 でもこの制約が明記されていた。

地理院標準・地理院淡色・OpenTopoMap はいずれも等高線入りであるため、OSM 選択時のみ等高線情報が失われる。この欠落を補う手段として等高線オーバーレイの追加を検討した。

参考実装:

- frogcat 氏の手法（等高線ピクセル描画）: <https://qiita.com/frogcat/items/1224c4c8f1bc308c4b42>
- タイル境界の解消手法: <https://www.openstreetmap.org/user/a2021/diary/405245>

## Decision

**地理院標高タイルを Canvas でピクセル処理する独自等高線オーバーレイ**を HTML ビューアに追加する。

主な仕様：

- データソース: 地理院標高タイル（dem5a_png/dem5b_png/dem5c_png/dem_png）をブラウザから直接 fetch（プロジェクトの prefetch ローカルキャッシュには依存せず、ブラウザ HTTP キャッシュに任せる）
- ズーム別タイル選択: マップズーム ≤14 は `dem_png`（z=14）/ ズーム 15 は `dem5a_png`（z=15、404時 dem5b→dem5c→dem_png z=14 縮小補完）/ ズーム ≥16 は z=15 タイルを Leaflet `maxNativeZoom` 機能で拡大表示
- dem1a（z=17、1m メッシュ）はカバレッジが限定的なため現時点では採用しない。HLD で改めて評価する
- 描画レイヤー: `contourPane`（z-index=250、`pointerEvents=none`）を独自追加し、基図（z=200）と overlayPane（z=400）の間に挿入
- デフォルト OFF・レイヤーコントロールから ON/OFF 可能
- 主用途は OSM 選択時の補完だが、独立 overlay として他の基図との同時 ON も可能（等高線入り基図との組み合わせでは二重表示になる）
- 帰属表示: GeoJSON データ自体も地理院標高タイル由来であるため、基図選択に関わらず `© 国土地理院`（`https://maps.gsi.go.jp/`）を常時表示するよう変更する

タイル境界での等高線の途切れ解消: 主タイル・右隣・下隣の3タイルを並列取得して 257×257 の拡張グリッドを構築し、境界ピクセルの比較を可能にする（参考: <https://www.openstreetmap.org/user/a2021/diary/405245>）。同一 URL は Map キャッシュ（上限 300 件）で管理し重複 fetch を防ぐ。

描画パラメータの HLD 向け暫定値（`docs/mockup/viewer_mockup.html` で確認済み）:

- 色: 計曲線 `#a04020` (alpha=120) / 主曲線 `#c87850` (alpha=70)
- 間隔: z≤11: 500m/100m、z≤13: 200m/50m、z≤14: 100m/20m、z≥15: 50m/10m
- 最終値は HLD で確定する（ISSUE-070）

## Alternatives

**(1) OSM を選定から外す**（不採用）  
街・道路・地名の確認という OSM 固有の補完用途（ADR-SRS-006 Decision 参照）を失う。

**(2) Thunderforest Outdoors 等の等高線付き外部タイルを追加採用**（不採用）  
API キー取得・管理コストが生じる。ADR-SRS-006 で既に検討済み。

**(3) 地理院 陰影起伏図（hillshade）オーバーレイ**（不採用）  
等高線ではなく陰影表現のため、標高の絶対値が読み取れず申請根拠確認用途に不向き。ADR-SRS-006 Alternatives で言及済み。

## Consequences

- dem5a/dem5b/dem5c/dem10b のフォールバックにより、GSI タイル整備状況に応じた自動切替が実現される
- ブラウザ標準 HTTP キャッシュに依存するため、同一タイルへの再アクセスはネットワーク通信なしで処理される
- ADR-SRS-006 Consequences 行 47「OSM は等高線なしで山岳部の視認性が低い」という制約が本 ADR 導入で事実上解消される
- 等高線入り基図（地理院標準・地理院淡色・OpenTopoMap）と等高線オーバーレイを同時 ON にすると二重表示になる。運用上の注意として文書化する
- dem1a（z=17）の採用可否は HLD フェーズで再評価する
