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

### 結論（2026-07-05時点、未確定）

- `terrain`: 低地〜中地の視認性が良いが、色相自体の好みは分かれる
- `gist_earth`: 高地の視認性が良いが、低地の視認性は `terrain` に劣る
- 一長一短のため、両者のハイブリッド（`terrain` 低〜中域 + `gist_earth` 中〜高域）を検討中

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
