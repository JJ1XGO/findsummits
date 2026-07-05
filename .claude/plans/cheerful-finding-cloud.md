# 標高地形図カラーマップ確定 — SRS/ADR反映

## Context

標高地形図PNG（`FR-015`）の色分け仕様は、`docs/20_SRS.md` 6.2.3節に「Japan Topo スキーム」として
暫定記載されていたが、`docs/CLAUDE.md`冒頭の仕様優先原則メモにある通り「最終形が見えないまま書いた
暫定のもの」で正式仕様ではなかった。今セッションで実データ（5338メッシュ9枚結合キャッシュ）を用いて
matplotlibで多数のカラーマップ・陰影起伏(hillshade)パラメータを試作・比較し、以下を確定した:

- カラーマップ: `gist_earth`
- 陰影起伏: `LightSource(azdeg=180, altdeg=77.7)`（夏至の太陽南中高度、日本の代表緯度で算出。物理的根拠あり）
- 標高誇張率: `vert_exag=8`
- 標高0m以下（海）は固定色（薄紫 `#E6D9F5`）でマスク
- 採用サンプル: `analysis/colormap_test/5338_hillshade_gist_earth_ve8_alt77.7_t0-0.00_mask-purple.png`

このセッションでの検討過程（terrain単体との比較、gist_earthのt0スキャン0〜0.8、光源位置の物理的妥当性検証、
マスク方式の実装と残存課題の発見等）は `docs/decisions/research/terrain-colormap-hillshade-research.md` に
一部記録済みだが、セッション後半の検討（t0=0-0.4微調整、マスク実装、150m地点の色の実測）は未反映。

正式仕様として確定するため、SRS本体・ADR・research資料を整合させる。あわせて、以前Fableに設計評価を
委託した「C実装 vs C++/OpenCV移行（[ADR-SRS-010](../../docs/decisions/ADR-SRS-010-cpp-opencv-migration.md)）」
との整合も取る（Fable推奨: 色定義はSRS確定のみで完結させ、hillshadeの実装自体はOpenCV移行のPhase 3へ
先送り。ただし ADR-SRS-010 の「Phase 3は内部実装の置換のみで仕様変更なし」という前提と矛盾するため、
そこも更新が必要）。

## ユーザー確認済みの決定事項

- **標高→色の正規化基準(vmax)は固定値 `3800m`** を採用する（ユーザー選択。動的値=解析範囲内の実測最大
  標高、というAI推奨案ではなく、旧Japan Topoスキームと同様に全メッシュで同一標高=同一色を保証する方を
  優先）。**既知のリスク**: 最大標高が低いメッシュ（低山地域）では、標高レンジ全体が gist_earth の
  暗いt値帯（t=0付近）に押し込まれ、今回発見した「海と低地の同化」問題が再発する可能性がある（未検証）。
  ADR-SRS-047 の Consequences に明記し、フォローアップ調査 issue を起票する
- `analysis/colormap_test/` 配下のファイル（スクリプト+生成画像、計24ファイル・約266MB）は全てコミット
  する（ユーザー明示指示）。サイズが大きい点は実行前にユーザーへ一言報告する

## 変更対象ファイル

1. **新規ADR**: `docs/decisions/ADR-SRS-047-terrain-color-scheme-gist-earth-hillshade.md`
   - Context: 旧Japan Topoスキームの限界（暫定記述だった経緯、実データ比較で判明した低地視認性の弱さ）
   - Decision: 上記の確定パラメータ一式（colormap/light source/vert_exag/vmax=3800m固定/海マスク色）
   - Alternatives（却下案とその理由、`research/terrain-colormap-hillshade-research.md`から要約引用）:
     - 旧Japan Topo固定ストップ方式 — 却下理由: 低地の視認性が悪く実データでの目視評価が悪かった
     - `terrain`カラーマップ単体 — 却下理由: ユーザー選好（gist_earthの高地表現を評価）
     - `gist_earth`+マスクなしNormalize調整（t0を0〜0.8でスキャン） — 却下理由: 海を暗くすると陸地低地も
       同化し、陸地を明るくすると海も緑化する、という構造的トレードオフが解消不可能と実証済み
     - 光源高度`altdeg=90`（天頂） — 却下理由: 日本の緯度では夏至でも太陽が天頂に来ることは物理的にない
       （日本の代表緯度における夏至の太陽南中高度の理論最大値は約77.7度）
   - Consequences: vmax固定によるリスク（上記）、gist_earthのt=0近傍が本質的に暗いため0m超の低地
     （標高0〜数十m程度）も海と紛らわしく見える残存課題（実データで確認済み: 150m地点でも青系の`#0d0de3`
     になる）、OpenCV移行後もhillshade相当は自前実装が必要（`cv::applyColorMap`はLUT方式でhillshade機能を
     持たないとFable指摘・WebSearchで裏取り済み）
   - 関連ドキュメント: `research/terrain-colormap-hillshade-research.md`、`ADR-SRS-010`、
     `ADR-SRS-012`（縮小方式、独立した既存ADR）
   - Alternativesの「旧Japan Topo固定ストップ方式」項で `docs/decisions/research/dem_colormap.html`
     （旧スキームの出典）への参照を引き継ぐ（SRS本文からは出典行を削除するため、参照はここに一本化）
   - Consequencesに、vmax固定という選択がプロジェクトの原則「実データ先行検証」（低山メッシュでの
     視認性は未検証のまま確定）から外れる例外である旨と、ユーザーがそれを承知の上で優先した判断である
     旨を明記する

2. **`docs/20_SRS.md` 6.2.3節**: 「色分け仕様（Japan Topo スキーム）」を新スキームの記述に置き換え
   - カラーマップ名・光源パラメータ・vert_exag・vmin(0)/vmax(3800固定)・海マスク色を明記
   - 詳細な却下案・根拠は `ADR-SRS-047` へのリンクで委ねる（既存6.2.3が`ADR-SRS-012`をリンクする形式を踏襲）
   - 「-6000mストップ...Japan Topoカラースキームの下限定義として保持」の一文と「出典: Japan Topo
     カラースキーム（dem_colormap.html）」の出典行は、新スキームに存在しない旧概念のため削除する
   - モックアップのサンプル画像リンクはそのまま `docs/figures/5338_terrain.png` を使うが、中身を
     新スキームの画像に差し替える（下記4.）

3. **`docs/decisions/ADR-SRS-010-cpp-opencv-migration.md`**: Phase 3の記述を更新
   （方式自体はOpenCV決め打ちのままで問題ないとFableへの設計評価委託で確認済み。根拠: hillshadeは
   OpenCVに限らずどのC++選択肢でも自前実装になり、アルゴリズムは本セッションでmatplotlibソースから
   完全特定済み（勾配→法線→内積→overlay合成）のため未知のギャップはない。matplotlibをC++から
   呼ぶ案（embedding/matplotlib-cpp）はADR-SRS-001のハイブリッド分担原則に反し運用コストも高いため却下）
   - 68-77行目の段階実行プラン表: Phase 3 の内容を「カスタムLUT配色 + 自前hillshade実装
     （matplotlib `LightSource.shade`互換アルゴリズム、仕様は[ADR-SRS-047]/SRS 6.2.3）+
     `cv::resize` + `cv::imwrite`」に変更。規模を「小」→「中」に修正
   - 検証ポイントを「既存出力との視覚比較（同等の可読性であれば可）」から「**matplotlibリファレンス
     出力との画素単位数値照合（許容誤差付き）**」に強化する（自前移植の写し間違いを機械的に検出するため）
   - 77行目「Phase 1〜2は既存機能の動作維持が目的...Phase 3は内部実装の置換のみで仕様変更なし」の一文を、
     Phase 3は新スキーム分の視覚仕様変更を伴う旨に修正
   - 29-36行目のOpenCV適用範囲表「標高地形図PNG（色分け・縮小・出力）| cv::applyColorMap / LUT +
     cv::resize + cv::imwrite」の行に、陰影起伏(hillshade)は`cv::applyColorMap`のLUT方式では
     カバーされず自前実装が必要である旨の注記を追加する
   - Alternatives表に「matplotlibをC++から呼ぶ（Python embedding / matplotlib-cpp）」の却下行を追加
     （却下理由: ADR-SRS-001のハイブリッド分担原則違反、コンテナ環境でのPythonランタイム＋GILと
     既存pthread並列の共存コスト、matplotlib-cppはLightSource非対応）
   - Consequencesに「OpenCVにhillshade相当のAPIはなく、自前実装は意図的な設計判断である」旨を明記
     （後続セッションでの誤読防止）
   - 実装時の技術的注意点（Fable指摘、Phase3着手時のISSUEへ引き継ぐ）: `cv::Sobel`は`np.gradient`と
     非等価（3×3平滑化・スケール係数が異なる。中心差分での再実装が必要）／8bit量子化はLUT適用・
     overlay合成をfloatのまま行い最終出力のみ8bit化する／画像のy軸方向と光源ベクトルの符号を誤ると
     陰影が南北反転する／縮小してからshadeする順序とdx/dyの追従を維持する（研究資料に記録済みの
     「二極化」再発防止）。これらは`research/terrain-colormap-hillshade-research.md`に記録し、
     Phase3実装ISSUE化時に参照する

4. **`docs/decisions/research/terrain-colormap-hillshade-research.md`**: 末尾に今回セッション後半の
   追加調査を追記（既存の「結論（未確定）」節を最終決定に置き換え）
   - t0スキャン0.00〜0.40（0.05刻み）の結果とマスク方式の採用経緯
   - 光源高度の物理的検証（azdeg/altdeg計算式、日本の緯度での夏至太陽南中高度=77.7度の算出根拠）
   - マスク版の実データ検証結果（標高=0.0が379万/2806万画素、0<標高≤5mが64万画素残存する等の実測値、
     150m地点の色`#0d0de3`の実測）— 残存課題として明記
   - 実装方式トレードオフ評価（Fable、本セッション）: OpenCV自前hillshade実装 vs matplotlib
     C++埋め込み の比較結果、および実装時の技術的注意点（`cv::Sobel`と`np.gradient`の非等価性・
     8bit量子化のタイミング・y軸符号・縮小順序とdx/dy追従）。Phase3実装ISSUE化時の参照用
   - 最終決定への確定リンク（`ADR-SRS-047`）

5. **`analysis/colormap_test/gen_terrain_hillshade.py`**: 冒頭docstring（1-11行目）を更新する。
   現状「gist_earthは構造的に合わずterrainを採用する」という古い結論が残っており、本セッションで
   確定した最終決定（gist_earth+hillshade+海マスク、vmax固定3800m）と矛盾している（Fable指摘、
   ファイル内容で確認済み）

6. **`docs/figures/5338_terrain.png`**: 現行ファイル（旧Japan Topoスキームのサンプル、2.1MB）を
   `analysis/colormap_test/5338_hillshade_gist_earth_ve8_alt77.7_t0-0.00_mask-purple.png` の内容で
   置き換える。既存の運用（「閲覧用に長辺1600pxへ再縮小」）に合わせ、venv の PIL で 1600px 長辺に
   縮小してから保存する

7. **GitHub issue起票**（`jj1xgo/findsummits`、`type:調査` `priority:低`）: 「固定vmax(3800m)採用時、
   低山メッシュ（最大標高が低い地域）でのgist_earth配色の視認性検証」。低山メッシュの実データで
   レンダリングし、海と低地の同化が起きないか確認する調査タスクとして登録（残作業に実データ検証・
   場合によっては再度の仕様判断が必要なため issue 判定基準を満たす）

## 実行順序

1. `gen_terrain_hillshade.py` の docstring を最終決定に合わせて更新
2. `analysis/colormap_test/` を `git add` して commit（研究用スクリプト＋生成画像、約266MB。
   コミット前にサイズをユーザーへ一言報告）
3. `docs/figures/5338_terrain.png` を新スキームサンプルで差し替え（PIL で1600px長辺に縮小）
4. `ADR-SRS-047` 新規作成
5. `docs/20_SRS.md` 6.2.3節を更新
6. `ADR-SRS-010` の Phase 3 記述を更新
7. `research/terrain-colormap-hillshade-research.md` に追加調査を追記
8. `make lint` で警告ゼロを確認
9. 上記docs変更を1コミットにまとめてコミット（`docs: 標高地形図の配色をgist_earth+陰影起伏方式に確定`
   等、Conventional Commits・日本語本文）
10. `/spec-panel 6.2.3`（および関連ADR）を実行し、指摘を洗い出す
11. 指摘のうち「作業実行」種別は即座に反映、「仕様検討」種別はユーザーに確認してから反映
12. 反映があれば再度 `make lint` → 追加コミット
13. `gh issue create` で低山メッシュ検証issueを起票

## 検証方法

- `make lint`（Markdown lint含む）で警告ゼロ
- SRS内リンク（`ADR-SRS-047`・研究資料・図版パス）が実在することを確認
- `docs/figures/5338_terrain.png` が新スキームの見た目になっていることを目視確認
- `/spec-panel 6.2.3` の指摘記録ファイル（`mgmt/spec-findings/`）が実際に生成されていることを確認
