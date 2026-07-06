# ADR-SRS-047: 標高地形図の配色をgist_earth+陰影起伏方式に確定

| 項目 | 内容 |
|---|---|
| 状態 | 採用・未実装 |
| 決定日 | 2026-07-06 |
| 調査資料 | [`research/terrain-colormap-hillshade-research.md`](research/terrain-colormap-hillshade-research.md) |

---

## Context

[FR-015](../20_SRS.md#fr-015-標高地形図出力)（標高地形図出力）の配色仕様は、`docs/20_SRS.md` 6.2.3節に
「Japan Topo スキーム」（8ストップの固定線形補間、陰影起伏なし）として暫定記載されていたが、
`docs/CLAUDE.md` 冒頭の仕様優先原則にある通り「最終形が見えないまま書いた暫定のもの」で
正式仕様ではなかった。

実データ（5338メッシュ・富士山を含む3×3結合キャッシュ）を用いてmatplotlibで多数のカラーマップ・
陰影起伏(hillshade)パラメータを試作・比較した結果、以下が判明した。

- 旧Japan Topoスキーム（陰影なし・固定ストップ線形補間）は低地の地形起伏がまったく視認できない
- `gist_earth`カラーマップ + `LightSource.shade()`による陰影合成は、高地の視認性・自然な見た目の点で
  `terrain`カラーマップ単体より優れる
- `gist_earth`は深海〜高山という地球規模の標高レンジを想定した配色のため、単純なNormalize調整
  （vmin/vmaxのt0スキャン）だけでは「海を暗くすると陸地の低地も同化し、陸地を明るくすると海も
  緑化する」という構造的トレードオフが解消できない
- 標高0m以下を固定色（マスク）で塗りつぶす方式により、この構造的トレードオフを回避できることが
  判明した（gist_earthのカラーマップ・陰影を海面から完全に切り離せる）

さらに、上記の方向性が定まった後、「低地部分は独自の落ち着いた配色にし、山岳部分のみ
gist_earthを使う」というハイブリッド配色（低地カスタム9ストップ + gist_earth山岳域の
ブレンド）を設計・試作したが、Fable設計による3候補案の提示を経てもなお「境目のまだら感」
「谷の黒つぶれ」「彩度・色相の色被り」が同時に解消せず、十数回のパラメータ調整（tint・
彩度・色相回転・青み加算等）を経ても収束しなかった。実装・検討の停滞を踏まえ、
gist_earth標準方式（カラーマップ全体をgist_earthのみで完結させる）へ回帰する判断をユーザーが下した。

## Decision

標高地形図の配色を以下のパラメータで確定する。

- **カラーマップ**: `matplotlib.cm.gist_earth`（256色のLUTとして抽出可能）
- **陰影起伏**: `LightSource(azdeg=180, altdeg=77.7).shade()`（`blend_mode`は既定の`overlay`）。
  `altdeg=77.7`は日本の代表緯度（5338メッシュ中央緯度35.6667度）における夏至の太陽南中高度の
  理論最大値であり、物理的根拠がある光源位置
- **標高誇張率**: `vert_exag=8`
- **正規化**: `vmin = -t0・vmax/(1-t0)`、**t0=0.25**（標高0mをgist_earthのt=0.25位置にアンカーし、
  低地を暗すぎない側にマッピングする）
- **vmax**: **解析範囲標高グリッドの実測最大標高**（メッシュ毎の相対値・動的に決定する）
- **海マスク**: 標高0m以下を**白`#FFFFFF`**で塗りつぶす（カラーマップ・陰影の対象外とし、
  gist_earthの構造的トレードオフから完全に切り離す）
- **陰影計算の幾何条件**: `dx`/`dy`は出力グリッドの実ピクセル距離（メッシュ中央緯度における
  Webメルカトル解像度 × 間引き率）を用いる。1px=1mとして扱うと勾配が実際より遥かに急峻になり
  陰影が二極化する（尾根だけ発光し大部分が黒つぶれる）ため、実距離換算が必須。陰影は
  縮小後グリッドに対して計算する
- **採用サンプル**: `analysis/colormap_test/5338_hillshade_gist_earth_ve8_alt77.7_t0-0.25_mask-white.png`
  （生成: `analysis/colormap_test/gen_terrain_gistearth_ocean_variants.py`）

なお、[FR-003](../20_SRS.md#fr-003-標高デコードnodata-処理)により海面・負標高・NODATAはすべて
0mに統一されるため、白マスクの適用対象にはNODATA相当の画素も含まれる（NODATAの個別識別は
本方式でも不可のまま）。

実装（`src/mesh_analyze.c`の`elev_to_rgb()`改修）は本ADRの決定事項に含めず、
[ADR-SRS-010](ADR-SRS-010-cpp-opencv-migration.md)（C++/OpenCV移行）のPhase 3へ委ねる。
現行のC実装（旧Japan Topoスキーム）はPhase 3着手まで残存し、SRSとの乖離が生じる。

## Alternatives

| 案 | 却下理由 |
|---|---|
| 旧Japan Topo固定ストップ方式（8ストップ線形補間、陰影なし） | 陰影起伏がなく低地の地形が視認できない。出典は[`research/dem_colormap.html`](research/dem_colormap.html) |
| `terrain`カラーマップ単体 | `gist_earth`の高地表現をユーザーが選好したため不採用 |
| `gist_earth` + マスクなしNormalize調整（t0を0〜0.8でスキャン） | 海を暗くすると陸地の低地も同化し、陸地を明るくすると海も緑化するという構造的トレードオフが実証され、マスクなしでは解消不可能と判明 |
| 独自ハイブリッド配色（低地カスタム9ストップ + gist_earth山岳域のブレンド） | Fable設計による3候補案（オリーブ・低彩度セージ・平野も緑）を含め十数回試行錯誤したが、「境目のまだら感」「谷の黒つぶれ」「全体の色被り」が同時に解消せず収束しなかった |
| vmax固定3800m（絶対値、全メッシュ共通） | 低山メッシュ（最大標高が低い地域）で標高レンジ全体がgist_earthの下側（暗い側）に圧縮され視認性が落ちる。「同一標高=同一色」というメッシュ間の一貫性より、低山メッシュでの視認性確保を優先（ユーザー判断） |
| 光源高度`altdeg=90`（天頂） | 日本の緯度では夏至でも太陽が天頂に来ることは物理的にない |
| matplotlibをC++から呼ぶ（Python embedding / `matplotlib-cpp`） | [ADR-SRS-001](ADR-SRS-001-hybrid-c-python-architecture.md)のハイブリッド分担原則（性能要求はC/C++、出力フォーマットはPython）に反する。`matplotlib-cpp`は内部でPythonインタプリタを起動し実行時にPython+matplotlibが必須になる依存であり、かつ`LightSource`のような陰影合成APIには対応していない（`plot()`等のグラフ描画APIのラッパーのみ） |

## Consequences

### Positive

- 低山メッシュでも標高レンジ全体がgist_earthの広い色域を使うため、色による標高判読性が確保される
- 海と陸地の分離が、Normalize調整に頼らずマスク処理で構造的に保証される
- 光源パラメータ（`altdeg=77.7`）に物理的根拠があり、恣意的なパラメータでない

### Negative

- **vmaxが相対値のため、メッシュ間で「同一標高=同一色」が成立しない**。あるメッシュの1500mと
  別メッシュの1500mが異なる色になりうる。旧Japan Topoスキームおよび検討初期のvmax固定案は
  これを保証していたが、低山メッシュでの視認性を優先してこの一貫性を手放した
- 白マスクの特性: 海の色が図の余白・背景と紛れやすい（ユーザー承知の上で選択）
- 端ケースの前提: 解析範囲グリッドは常に陸地画素を含む前提とする。陸地画素が存在せず
  `vmax <= 0`となる場合（メッシュ全体が海面下等）は、カラーマップの正規化が不能となり
  全面マスク色になる（実装時に個別のガード処理が必要）
- OpenCVには`LightSource.shade()`相当のhillshade APIがなく、[ADR-SRS-010](ADR-SRS-010-cpp-opencv-migration.md)
  Phase 3で自前実装が必要（`cv::applyColorMap`は8bit入力のLUT方式であり、陰影合成機能を持たない。
  WebSearchで裏取り済み: [OpenCV公式ドキュメント](https://docs.opencv.org/4.13.0/d3/d50/group__imgproc__colormap.html)）
- 実装完了までの間、`src/mesh_analyze.c`の`elev_to_rgb()`は旧Japan Topoスキームのまま残り、
  SRSとコードの間に意図的な乖離が生じる（[ADR-SRS-010](ADR-SRS-010-cpp-opencv-migration.md)のPhase 3で解消予定）

## 関連ドキュメント

- [`research/terrain-colormap-hillshade-research.md`](research/terrain-colormap-hillshade-research.md)
- [ADR-SRS-010: C++/OpenCVへの全面移行](ADR-SRS-010-cpp-opencv-migration.md)（Phase 3が本ADRの実装先）
- [ADR-SRS-012: 標高地形図の縮小方式](ADR-SRS-012-terrain-image-downscaling-method.md)
- [SRS FR-015: 標高地形図出力](../20_SRS.md#fr-015-標高地形図出力)
- [SRS 6.2.3: 標高地形図](../20_SRS.md#623-標高地形図)
