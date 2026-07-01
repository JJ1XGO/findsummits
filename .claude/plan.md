# FR-015 標高地形図サンプルイメージ作成（富士山5338中心3×3・解析処理なし）

## Context

SRSがほぼ確定し、残作業として FR-015（標高地形図出力）のサンプルイメージ作成が必要。
富士山を含む1次メッシュコード5338を中心とした3×3メッシュの標高地形図PNGを1枚生成する。
**目的は画像のみで、ピーク/コル解析（Union-Find・CSV出力）は今回の目的に関係ないため実行しない。**

方針（ユーザー承認済み）: 既存Cプロトタイプ（`src/mesh_analyze.c`）の地形図生成部分を使う。
FR-015仕様（Japan Topo配色・長辺6000pxへの最近傍縮小、`ADR-SRS-012`）を既に満たしており、
追加インストールも不要。C++/OpenCV化（`ISSUE-039`、`planned_stage: LLD`）はLLD未確定のため
今回は着手しない。

## 調査で判明した制約（計画に反映済み）

1. **`mesh_analyze()` は地形図出力（307-313行目）の直後に必ず Union-Find 解析 + CSV 出力まで
   実行する**（分離オプションが無い）。`findsummits`/`test_mesh_analyze` のどちらを使っても
   解析を避けられない。→ 解析を呼ばない**専用の小さいツールを新規に用意**する
2. **3×3結合ロジックの再利用可否**: `mesh_analyze()` 内の隣接拡張ロジック自体は外部公開されて
   いないが、`load_mesh_tile()`（`src/mesh_analyze.h:29`）は
   「外部から巨大イメージを作成して使うための関数（視覚化用）」として**既に公開設計**されている。
   今回は9メッシュ固定（拡張判定不要）なので、9メッシュそれぞれの `mesh_to_tile_range()` の
   外接矩形を取るだけで combined range を計算できる（`mesh_analyze()` 260-283行と同種だが
   `mesh_set_contains` 判定が要らない分シンプル）
3. **画像保存関数の可視性**: `save_terrain_rgb_image()`（`src/mesh_analyze.c:97`)は現在 `static`。
   既存の色分け・縮小ロジックをそのまま再利用するため、`static` を外して `mesh_analyze.h` に
   宣言を追加する（本番 `mesh_analyze()` の呼び出し順序・動作は無変更、可視性のみの変更）
4. **タイルキャッシュ不足**: 西列3メッシュ（5237・5337・5437）のDEMタイルは未取得
   （既存prefetchログは全て `[5238,5239,5240,5338,5339,5340,5438,5439,5440]` で5339中心の3×3のみ）。
   事前に `scripts/prefetch_tiles.py` で取得が必要（GSIタイルサーバへの到達性は確認済み）
5. リポジトリへの変更は「可視性変更1箇所＋新規テストファイル1つ＋Makefileターゲット追加」のみ。
   `mesh_analyze()` 自体のロジックは無変更。docs/ を触らないため `/spec-panel` はスキップ対象

## 手順

1. **地形図専用ツールの新規作成**
   - `src/mesh_analyze.c`: `save_terrain_rgb_image()` の `static` を外す
   - `src/mesh_analyze.h`: `void save_terrain_rgb_image(const ElevTile *big, const char *path, FILE *logfp);` を宣言追加
   - `tests/test_terrain_image.c` を新規作成:
     - 9メッシュコード配列（5237, 5238, 5239, 5337, 5338, 5339, 5437, 5438, 5439）を定義
     - 各メッシュの `mesh_to_tile_range()` から外接矩形（combined range）を計算
     - `load_mesh_tile()` でタイル結合（既存公開関数を再利用）
     - `save_terrain_rgb_image()` でPNG保存（可視性変更後の既存関数を再利用）
     - `elev_destroy()` で解放。**Union-Find解析やCSV出力は一切呼ばない**
   - `Makefile` に `test_terrain_image` ターゲットを追加（既存 `test_mesh_analyze` ターゲットに準拠）

2. **DEMタイル事前取得**
   - 9メッシュを1行1コードで記載した一時メッシュリストファイルをスクラッチパッドに作成
     （リポジトリ内 `params/mesh_list_japan.txt` は変更しない）
   - `venv/bin/python3 scripts/prefetch_tiles.py --mesh-list <一時ファイル> --config params/fetch_config.ini --tile-dir $DATA_DIR/tiles --log-dir $DATA_DIR/logs`
   - 9メッシュ全部を対象に実行（既取得分は If-Modified-Since で高速スキップされるため安全。新規は西列3メッシュのみ）
   - 時間がかかる可能性があるため `run_in_background` で実行し、進捗を都度報告する

3. **ビルド・実行**
   - `make test_terrain_image`
   - `./build/test_terrain_image`（引数なし・9メッシュ固定）または出力先を明示できる形にする
   - 出力: `$DATA_DIR/images/5338_terrain.png` のみ生成。`$DATA_DIR/results/` には一切書き込まない

4. **出力確認・提示**
   - 生成PNGのファイルサイズ・寸法を確認
   - Read ツールで画像を直接表示し、Japan Topo配色・既知の格子状バグ（解決済みBUGあり）再発の有無を目視確認

5. **lint確認**
   - `make lint` を実行し、新規Cコードに対するコンパイラ警告がゼロであることを確認（機械的チェック警告ゼロの原則）

## 検証方法

- `make test_terrain_image` がビルド警告ゼロで成功すること
- 実行後、`$DATA_DIR/results/` にファイルの追加・変更が発生していないこと（`ls -la` で確認）
- 生成PNGを実際に開いて目視確認（3×3分の範囲が結合されていること、富士山の山頂が写っていること、配色が仕様通りJapan Topoスキームであること）

## 実行モデル

全ステップSonnet直接対応（既定）。アーキテクチャ判断は完了済みのため委譲対象タスクなし。
