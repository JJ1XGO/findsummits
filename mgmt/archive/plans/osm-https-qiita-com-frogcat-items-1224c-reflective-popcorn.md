# モックアップに等高線オーバーレイを追加（OSM 選択時の等高線欠落を補完）

## Context

`docs/mockup/viewer_mockup.html` の背景タイル切替で **OSM** を選ぶと等高線情報が失われ、山岳確認の視認性が落ちる（地理院標準・地理院淡色・OpenTopoMap はいずれも等高線入り。OSM だけが等高線なし。`docs/decisions/ADR-SRS-006-viewer-background-tile-selection.md` Consequences 行 47 で明記）。

frogcat 氏の手法（https://qiita.com/frogcat/items/1224c4c8f1bc308c4b42）で **国土地理院 標高タイルを Canvas で処理して等高線をピクセル単位で描画**すれば、基図に依存しない独立オーバーレイとして等高線を提供できる。本タスクではモックアップにこの方式の等高線レイヤー（**主用途は OSM 時の補完**、ただし overlay として独立 ON/OFF）を追加し、現実的な精度（dem5a/dem1a 利用）・色（赤茶系）・重ね順（基図→等高線→GeoJSON）で動作することを確認する。

合わせて、ビューアが扱う GeoJSON データ自体が地理院標高タイル由来であることを反映し、基図の選択に関わらず `© 国土地理院` を attribution に常時表示する規約を SRS で明確化する。

## 方針サマリー

| 項目 | 内容 |
|---|---|
| 等高線データ取得 | ブラウザから GSI 標高タイル URL を直接 `fetch`（プロジェクトの prefetch キャッシュには依存せず、ブラウザ HTTP キャッシュに任せる） |
| ズーム別タイル | ≤14: `dem_png` z=14 / 15: `dem5a_png` z=15（404 時 5b→5c→dem_png z=14）/ 16〜: `dem1a_png` z=17（404 時 dem5a 拡大） |
| 描画方式 | `L.GridLayer` サブクラス。1タイル取得→256×256 Canvas で標高デコード→隣接セル比較で等高線判定→ピクセル塗り |
| 色（赤茶系） | 計曲線 `#a04020`（やや太め）／ 主曲線 `#c87850`（細め） |
| 等高線間隔 | ズーム ≤12: 100m/500m、ズーム 13〜14: 50m/250m、ズーム 15: 20m/100m、ズーム ≥16: 10m/50m（dem1a 取得時は 5m/25m 検討） |
| 重ね順 | `map.createPane('contourPane')` + `zIndex=250`、`pointerEvents='none'` で基図(200) と overlayPane(400) の間に挿入 |
| トグル | レイヤーコントロールの overlay に「等高線」を追加（デフォルト OFF・主用途は OSM 選択時。他の基図と二重表示になる可能性あり） |
| 出典常時表示 | `map.attributionControl.addAttribution('<a href="https://maps.gsi.go.jp/" target="_blank">© 国土地理院</a>')` を初期化時に固定追加。基図切替・等高線 ON/OFF いずれでも残る |

## 変更対象ファイル

| ファイル | 変更内容 |
|---|---|
| `docs/mockup/viewer_mockup.html` | 等高線レイヤー（GridLayer 派生 + Canvas 等高線描画）追加・`contourPane` 定義・レイヤーコントロールに「等高線」トグル追加・`© 国土地理院` を常時 attribution 化 |
| `docs/02_SRS.md` | FR-013（ビューア機能・行 925〜966 周辺）に「等高線オーバーレイ」項目追加、「地図帰属表示」項目（行 946）を「基図選択に関わらず `© 国土地理院` を常時表示」に修正 |
| `docs/decisions/ADR-SRS-015-contour-overlay.md` | 新規作成（独自 Canvas 描画方式の採用理由、ズーム別タイル選択方針、ADR-SRS-006 との関係） |
| `mgmt/tracker/`（ISSUE 登録） | HLD で詰めるべき実装詳細を ISSUE として記録（下記参照） |

## SRS に反映する範囲（仕様レベル）

`docs/02_SRS.md` FR-013 に以下を追記する：

1. **等高線オーバーレイ機能**（新規項目）
   - 「地理院標高タイル（dem5a/dem1a/dem10b）をブラウザから取得し、Canvas で等高線を描画するオーバーレイレイヤーを設ける。**OSM 基図選択時の等高線欠落を補う用途**として位置づける。レイヤーコントロールから ON/OFF 可能、デフォルト OFF」
   - 「重ね順は基図 → 等高線 → GeoJSON（ポリゴン・線・マーカー）の順」
   - 色・間隔の数値は HLD に委ねる旨を注記

2. **地図帰属表示**（行 946 の修正）
   - 「`© 国土地理院` は GeoJSON データ（peak/col/summit/AZ/delete_zone 等）が地理院標高タイル由来であるため、**基図選択に関わらず常時表示**する。リンク先は `https://maps.gsi.go.jp/`」
   - 等高線レイヤー ON 時も同 attribution が継続して有効である旨を併記

3. **ADR-SRS-006 への参照追記**（行 930 周辺）
   - 等高線オーバーレイが OSM の「等高線なし」制約を補完することを ADR-SRS-006 との関係として明記（地理院淡色・OpenTopoMap・地理院標準は元々等高線あり）

## ADR-SRS-015（新規作成）に書く内容

| 項目 | 概要 |
|---|---|
| Context | OSM には等高線がなく山岳視認性が落ちる課題（ADR-SRS-006 Consequences 行 47 で残されていた制約。地理院標準・淡色・OpenTopoMap は等高線あり） |
| Decision | frogcat 方式（地理院標高タイル + Canvas）の独自等高線オーバーレイを採用。主用途は OSM 補完だが基図に依らない独立 overlay として実装 |
| Alternatives（不採用） | (1) OSM を選定から外す（街・道路確認の補完用途を失う） (2) Thunderforest Outdoors を導入（API キー管理コスト） (3) 地理院 陰影起伏図オーバーレイ（等高線ではなく陰影。標高数値の読み取り不可） |
| Consequences | dem1a/dem5a/dem10b の自動フォールバックでカバレッジを担保／ブラウザ標準キャッシュに依存し prefetch キャッシュとは独立／ADR-SRS-006 の Consequences 行 47 の OSM 制約が事実上解消／等高線入り基図（地理院・OpenTopoMap）と同時 ON すると二重表示になる旨を運用上注意 |

ADR-SRS-006 自体は廃止せず、本 ADR への相互参照を Consequences に追記する。

## HLD で詰める内容（ISSUE として記録）

以下を `venv/bin/python3 mgmt/tracker/track.py issue add` で登録（実装着手時の参照用）。SRS には数値を入れず、ここで管理する。

1. **ISSUE（高/HLD）**: 等高線レイヤー描画パラメータ確定
   - ズーム別の計曲線/主曲線間隔（上記サマリー表の暫定値を実画面で検証して確定）
   - 線色・線幅・透明度（`#a04020`/`#c87850` を出発点として OSM・地理院淡色との合成可読性で微調整）
   - 等高線描画アルゴリズム（隣接セル比較 vs marching squares）

2. **ISSUE（高/HLD）**: 標高タイル選択・フォールバック実装方針
   - ズーム別優先タイル（dem10b/dem5a/dem1a）の選択ロジック
   - 404 時のフォールバック順序（dem1a → dem5a → dem5b → dem5c → dem10b）
   - 取得失敗時の Canvas タイル扱い（透明 or 「等高線なし」表示）
   - 並列リクエスト数の上限（GSI 規約配慮）

3. **ISSUE（中/HLD）**: Leaflet pane と重ね順の整理
   - `contourPane`（z-index=250、pointerEvents=none）の正式採用
   - 他オーバーレイ（基準点 cp・1次メッシュ）との z-index 整合性確認
   - 「等高線」レイヤーのレイヤーコントロール位置（基図と overlay のどちらに置くか）

4. **ISSUE（中/HLD）**: attribution 制御方式
   - 初期化時の固定 attribution 追加方法
   - GeoJSON 由来データの attribution と等高線レイヤーの attribution の重複排除（Leaflet が自動でまとめるか確認）
   - 公開用 HTML エクスポート（FR-020）への引き継ぎ方

## 検証方法

モックアップを `docs/mockup/viewer_mockup.html` をブラウザで開いて確認：

1. **基図切替**: OSM を選んだ状態で「等高線」レイヤー ON にすると等高線が表示される（主用途）。他の基図に切替えても overlay として表示自体は可能（二重等高線になることを目視確認）
2. **色の視認性**: OSM 上で赤茶系の等高線が読み取れる
3. **重ね順**: ピーク（caret-up）・コル（caret-down）・サミット（circle）アイコンと AZ・delete_zone ポリゴンが等高線より上に描画されている
4. **操作性**: 等高線の上にあるピーク等のマーカーを普通にクリックしてポップアップを開ける（`pointerEvents=none` 効いている）
5. **ズーム別タイル選択**: ズーム 12 / 14 / 15 / 16 / 17 で適切なタイル種別（DevTools の Network タブで `dem_png`/`dem5a_png`/`dem1a_png` URL を確認）に切り替わる
6. **dem1a カバレッジ外**: dem1a が未整備な山岳エリア（高ズーム）で 404 → dem5a 拡大表示に fallback する
7. **出典表示**: 基図・等高線レイヤーのいかなる組み合わせでも、画面右下に `© 国土地理院` がリンク付きで常時表示される
8. **キャッシュ動作**: 同じタイルへの再アクセス時、Network タブで `(from disk cache)` または `(from memory cache)` になりブラウザキャッシュが効く

## 進め方（実装フェーズ）

1. `docs/mockup/viewer_mockup.html` に等高線レイヤー・pane・attribution 固定追加を実装
2. ブラウザで全検証項目を確認
3. 検証 OK 後、`docs/02_SRS.md` FR-013 を上記方針に沿って更新（モックアップ実装の参照を含める）
4. `docs/decisions/ADR-SRS-015-contour-overlay.md` を新規作成、ADR-SRS-006 の Consequences に相互参照追記
5. HLD 用 ISSUE（4 件）を `track.py issue add` で登録
6. ドキュメント変更を個別ファイル指定でコミット（Conventional Commits・本文日本語）
