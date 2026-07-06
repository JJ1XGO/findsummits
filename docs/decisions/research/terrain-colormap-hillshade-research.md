# 標高地形図カラーマップ・陰影起伏(hillshade)検証記録

本ページは `docs/20_SRS.md` 6.2.3（標高地形図の色分け仕様、暫定記述）確定に向けた調査記録であり、
正式仕様ではない。仕様確定時はこの記録を参照しつつ SRS/ADR 側に反映する。

## 目視参照画像の実態

ユーザー提供の参照画像 `analysis/5338-00_15-28945-12867_15-29036-12942.jpg`（`findsummits4sotaja`
生成）を調査した結果、以下が判明した（コミット `fef2e22`・`findsummits4sotaja/findsummits/png2image.py`
で確認、画像タイムスタンプとコミット時刻の突合で当該コミット内容が生成時点のコードと一致することも確認済み）。

- カラーマップは `matplotlib.cm.gist_earth`（`terrain` ではない）
- `matplotlib.colors.LightSource(azdeg=180, altdeg=90).shade(..., cm.gist_earth)` で
  陰影起伏(hillshade)を合成している
- 前セッションで「疑似標高データで `terrain` が `gist_earth` より優位」と判定していたのは、
  hillshade なしでの比較だったため前提が異なっていた

## matplotlib 上での再現・パラメータ調整

`analysis/terrain_colormap_demo.c` で生成した 9 メッシュ結合キャッシュ
（`/data/images/5338_cmp_bigtile.cache`）を `analysis/colormap_test/gen_terrain_hillshade.py`
で読み込み、`LightSource.shade()` のパラメータを検証した。

### dx/dy（ピクセル間隔）

`np.gradient()` は既定 `dx=dy=1` だと「標高差(m) ÷ 1px」という桁違いに急な勾配として計算し、
法線がほぼ水平になって陰影が二極化する（尾根だけ発光、大部分が黒つぶれ）。
ズーム15・stride=12 の実効解像度 `3.8812 m/px × 12 ≈ 46.57 m/px` を `dx`/`dy` に設定することで解消。

### vert_exag（標高誇張率）

実距離ベースの `dx`/`dy` にすると今度は明るすぎる（白飛び）方向に振れる。
`vert_exag=3/8/15` を比較し、`vert_exag=8` が最も自然な陰影バランスだった。

### 海(NODATA)の扱い

`gist_earth` は t=0 が黒(`RGB(0,0,0)`)で、`shade()` の既定 `blend_mode='overlay'` は
「素の色が黒なら陰影をどう当てても黒のまま」という性質を持つ
（`colors.py: blend_overlay()` — `low = 2*intensity*rgb`、rgb=0なら常に0）。
このキャッシュには `-9999` センチネルは残っておらず、C側で海・NODATAは既に `0.0` に丸め込み
済み（実測: `min=0.0`、`-9000以下の値は0件`）。`mesh_analyze.c` の `elev_to_rgb()` も
「0m以下は一律海」という同じ判定をしているため、`elevs <= 0.0` をマスクとして検出し、
カラーマップ・陰影の対象から外して固定色で上書きする方式を採用。
海の色はユーザー希望により薄いピンク `#FFE4E9` に決定（試行: 現行C実装の海色 `#1a4f72` →
`5339_terrain.png` 実測の `#B7E5FA` → 桜色 `#FFB7C5` → さらに薄い `#FFE4E9`）。

### 独自ハイブリッド配色の試行と収束断念（2026-07-06）

上記「一長一短のため両者のハイブリッドを検討中」を受け、低地はカスタム定義の9ストップ配色
（Fable設計、その後彩度低下版のA案「オリーブ経由」を採用）、山岳部（一定標高以上）は
gist_earthの中〜高域を抽出してブレンドする方式を試作した。実データ（5338メッシュ）で
以下を含む十数回のパラメータ調整を行った:

- Fable設計の3配色案（A: オリーブ経由／B: 低彩度セージ／C: 平野も緑）の比較
- 低地〜山岳のブレンド境界標高の調整（600-1200m → 400-900m → 200-600m → 100-400m。
  房総半島・三浦半島のような低い丘陵にも緑感を持たせるため段階的に引き下げ）
- gist_earth抽出域（t=0.3〜1.0 等）の下限・上限調整（濃い緑・高所の赤みの抑制）
- ブレンドカーブの smoothstep 化、山岳ゾームの陰影強度低減（谷の黒つぶれ対策）
- 全体への白tint・彩度スケール・色相回転・青み加算（色被り対策）

結果、「境目のまだら感」「谷の黒つぶれ」「全体の色被り（緑・オリーブ・青緑の色被り感）」が
同時に解消せず収束しなかった。実装の停滞を踏まえ、独自ハイブリッド配色は断念し、
**gist_earth標準方式（カラーマップ全域をgist_earthのみで完結）への回帰**をユーザーが決定した。

### t0=0.25 + 海マスク分離方式の採用（2026-07-06）

gist_earth標準方式に戻すにあたり、上記「海(NODATA)の扱い」で判明した構造的トレードオフ
（t0=0.00だと標高0m付近がgist_earthの最暗部＝ほぼ黒にマッピングされ低地が暗く沈む一方、
t0を上げると海も同じカラーマップで明るくなり陸地の低地と同化する）を、海面マスクの導入で
回避できることを実データで確認した。

- t0=0.15/0.25/0.30 で比較（マスクは海面(elev≤0)に別途固定色を適用するため、t0を上げても
  海と陸の混同は発生しない）
- t0=0.25 が、平野の視認性向上と高地の色バランスの両立で最も妥当と判断
- 海マスク色は薄紫 `#E6D9F5`（前回セッション決定）から**白 `#FFFFFF`** に変更（ユーザー決定）

### vmax の相対値化（2026-07-06）

前回セッションでは vmax を固定値 3800m とする方針だったが、低山メッシュ（最大標高が低い
地域）では標高レンジ全体が gist_earth の暗い側に圧縮され、低地・海の同化問題が再発しうる
リスクが指摘されていた。本セッションで、**vmax を解析範囲標高グリッドの実測最大標高
（メッシュ毎の相対値）とする**方針に変更した。これにより低山メッシュでもカラーマップ全域が
使われ視認性が確保されるが、トレードオフとしてメッシュ間で「同一標高=同一色」が
成立しなくなる（[ADR-SRS-047](../ADR-SRS-047-terrain-color-scheme-gist-earth-hillshade.md)
のConsequencesに明記）。

### 最終決定

- カラーマップ: `gist_earth`
- 光源: `LightSource(azdeg=180, altdeg=77.7)`、`vert_exag=8`
- 正規化: `t0=0.25`（`vmin = -t0・vmax/(1-t0)`）、`vmax`は解析範囲の実測最大標高（相対値）
- 海マスク: 標高0m以下を白 `#FFFFFF`
- 採用サンプル:
  `analysis/colormap_test/5338_hillshade_gist_earth_ve8_alt77.7_t0-0.25_mask-white.png`
  （生成: `gen_terrain_gistearth_ocean_variants.py`）

確定内容は [ADR-SRS-047](../ADR-SRS-047-terrain-color-scheme-gist-earth-hillshade.md) に記録した。

## Fable への設計評価委託（実装方式）

「本番は C++/OpenCV 移行予定（`ADR-SRS-010`）なのに、今 C で hillshade を実装すると
使い捨てにならないか」というユーザーの懸念を受け、Fable に設計評価を委託した。

**結論（Fable、2026-07-05）**:

1. stops/colors のような「データ」は言語非依存でC→C++にそのまま流用できるが、hillshade の
   ような「ロジック」は使い捨て度が中程度。OpenCVには `LightSource.shade()` 相当の組み込みが
   なく、Phase 3 でも自前実装が必要になる（ただし C の素朴なループは Phase 2 の機械翻訳で
   ほぼ逐語的に C++ へ移せるため、完全な捨てコードではない）
2. 進め方は **(a) 色テーブルのみ今 C (`elev_to_rgb()`) に反映し、hillshade 実装は
   `ADR-SRS-010` Phase 3（OpenCV移行後）へ先送り**を推奨。理由: 今回の作業の本質は
   「見た目の仕様確定」であり、matplotlib 検証結果を SRS 6.2.3 に記録すれば仕様確定は完結する。
   C実装は確定の必須条件ではない
3. `ADR-SRS-010` は「Phase 3は内部実装の置換のみで仕様変更なし」と明記しているため、
   hillshade 追加のような「見た目が変わる変更」を Phase 3 に黙って含めるのは不可。
   正しい順序は「先に SRS/[FR-015](../../20_SRS.md#fr-015-標高地形図出力) を改訂して仕様確定 →
   `ADR-SRS-010` に『Phase 3 は改訂後の [FR-015](../../20_SRS.md#fr-015-標高地形図出力) を
   実装する』と前提を追記」

## cv::applyColorMap の制約（WebSearch で確認、2026-07-05）

Fable の指摘（「`cv::applyColorMap` は8bit入力前提でカスタム標高stopsをそのまま扱えない」）を
[OpenCV公式ドキュメント](https://docs.opencv.org/4.13.0/d3/d50/group__imgproc__colormap.html)・
[LearnOpenCV解説](https://learnopencv.com/applycolormap-for-pseudocoloring-in-opencv-c-python/)
で裏取りした。

- `applyColorMap()` は `CV_8UC1`（グレースケール）または `CV_8UC3` のみ受け付ける。
  float型の標高値（現行 `-9999〜3800m` 程度の実数）をそのまま渡せない
- カスタムカラーマップは 256×1 の LUT（Look-Up Table）方式（0〜255の各整数値→RGB）。
  現行の「任意の実数標高値を stops 配列で線形補間する」方式とは構造が異なる
- そのため Phase 3 では「標高値(float) → 0-255への正規化・量子化」という前処理ロジックを
  新規追加する必要があり、単純に現行 stops/colors 配列を持ち込むだけでは済まない
- `applyColorMap` 自体は色付けのみで hillshade（陰影合成）機能は持たないため、
  陰影ロジックは OpenCV 採用後も別途自前実装が必要な点は変わらない

## 関連ファイル

- `analysis/terrain_colormap_demo.c`（C試作: terrain/monotone/landscape/ref15/ref15_swapped）
- `analysis/colormap_test/gen_terrain_hillshade.py`（matplotlib試作: terrain/gist_earth + hillshade）
- `src/mesh_analyze.c` の `elev_to_rgb()`（本番実装、45-91行付近）
- [`ADR-SRS-010`](../ADR-SRS-010-cpp-opencv-migration.md)（C++/OpenCV移行、Phase 3が本テーマの本実装先）
