# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## プロジェクト概要

国土地理院の標高タイル（レベル15）を使ってSOTA日本支部の対象サミットを最新化する為の支援ツール。
最新化するに当たって必要なO/Pは、下記3点。
・SOTA日本支部に更新申請をするに当たって申請用ファイル（excel。新規・変更・削除）があるので、それに値を入れたexcelファイル。最重要。
・対象サミットの一覧（座標と標高、そのサミットのKeyコル座標と標高。対象サミットとそのKeyコルの標高がわかればプロミネンスがわかるが一応データとしてはプロミネンスも欲しい。csvで可）。申請書の内容を証明するためのエビデンスとして。
・国土地理院地図上で目視確認するためのGEOJSONファイル。申請書の内容を証明するためのエビデンスとして。
当然、現行の対象サミットに変更・削除があるケースが考えられるので、それらを考慮して申請先の人が見てわかる形のO/Pにする必要がある。

解析に使う基本データはdem5aをベースとし、dem5aがない/もしくは有効値がない座標はdem5b(の該当座標)から、dem5bがない/もしくは有効値がない座標はdem5c(の該当座標)から、dem5cがない/もしくは有効値がない座標はdem10b(の該当座標)から標高値を取得する。

## 仕様優先原則（重要）

**現在のコードは仕様が曖昧な状態で Claude が意図を推測して書いたものであり、正式な仕様ではない。**

- **URD/SRS/HLD/LLD が目標状態**。コードはその暫定的な副産物に過ぎない
- **コードと URD/SRS の乖離は意図的かつ正常**。コードを正にしてはならない
- **仕様を決めてからコードを書く**。SRS/HLD/LLD レビュー中は実装に手を入れない
- この原則は HLD/LLD フェーズでも同様に適用する

Claude へ: SRS/HLD/LLD の記述を確認・レビューするときに「でも実装ではこうなっている」という視点でコードを参照してはならない。仕様書の内容が正であり、コードは後で仕様に合わせて書き直す。

## ドキュメント・実装整合性原則

仕様書・設計書（URD/SRS/HLD/LLD/ADR）の内容とプログラムの実装は常に整合させること。

- 仕様変更時はドキュメントとコードを同期して更新する（片方だけの修正禁止）
- 実装が仕様と乖離した場合は、仕様優先原則に基づき仕様を正としてコードを修正する
- 乖離が意図的で許容される場合は ADR として記録する

（補足: 本原則は元々 UR-009 として URD に記載されていたが、ユーザー要件ではなく開発プロセス品質目標であるため CLAUDE.md に移動した。ISSUE-029）

## ビルド・テスト

```bash
make                    # build/findsummits をビルド
make test_mesh_analyze  # build/test_mesh_analyze をビルド
make test_analyze       # build/test_analyze をビルド
make clean              # build/ ディレクトリごと削除

./build/findsummits 4929                        # 1次メッシュコード指定で実行
./build/findsummits params/mesh_list_japan.txt  # メッシュリストファイル指定で実行
./build/test_mesh_analyze 4929                  # テスト（イメージ出力なし）
./build/test_mesh_analyze 4929 --save-image     # テスト（標高地形図 PNG も出力）
```

依存: `libpng`, `libm`, `pthread`（GCC / C99）

## Python スクリプトの実行

**Python スクリプトは必ず `venv/bin/python3` で呼び出すこと（`python3` は不可）。**
`python3` はコンテナのシステム Python であり、パッケージが入っていない。

venv が存在しない場合は先に `make venv` を実行する:

```bash
make venv                                                          # 初回セットアップ・requirements.txt 変更時
venv/bin/python3 scripts/prefetch_tiles.py ...                    # タイル取得
venv/bin/python3 scripts/merge.py ...                             # CSV 統合・出力
venv/bin/python3 scripts/preprocess_pref_boundaries.py ...        # 都道府県境界前処理
venv/bin/python3 mgmt/tracker/track.py issue list                          # 課題管理
```

venv は `/workspace/venv/`（ホストマウント下）に作られるためコンテナリビルド後も消えない。

## アーキテクチャ

### データフロー

```
（事前準備）
prefetch_tiles.py でタイルを $DATA_DIR/tiles/ へ取得済み
  ↓
入力: 1次メッシュコード（例: 4929）またはメッシュリストファイル
  ↓
mesh_analyze() [mesh_analyze.c]
  ├─ 中心メッシュ + 隣接メッシュ（最大3×3）の結合範囲を計算 [mesh.c]
  ├─ ローカルキャッシュ済みタイルを読み込み [elevation.c]
  │   （ローカルキャッシュ未取得時はエラー終了）
  └─ 全タイルを1枚の大画像に結合
  ↓
標高地形図 PNG 出力 [mesh_analyze.c]
  └─ $DATA_DIR/images/<meshcode>_terrain.png（長辺6000px縮小）
  ↓
Union-Find アルゴリズムでピーク・コルを検出 [unionfind.c]
  ↓
比高 >= 130m でフィルタ（最終 150m 判定は merge.py で実施）
  ↓
出力: $DATA_DIR/results/csv/<meshcode>.csv
```

### 主要モジュール

| モジュール | 役割 |
|---|---|
| `mesh.c/h` | 1次メッシュコード ↔ タイル座標変換（ズーム15 Web Mercator）、隣接メッシュ計算・MeshSet |
| `elevation.c/h` | PNG タイルデコード（RGB→標高）、8方向オーバーラップ対応、ローカルキャッシュ参照のみ |
| `unionfind.c/h` | Union-Find（経路圧縮・rank による union）でピークグループ管理 |
| `analyze.c/h` | タイル単体のピーク候補検出・比高計算、`col_margin_px` 算出 |
| `mesh_analyze.c/h` | メッシュ全体のオーケストレーション・標高地形図 PNG 出力・CSV 出力 |
| `scripts/prefetch_tiles.py` | タイル事前取得（If-Modified-Since 条件付き GET・並列4・429/503 backoff） |
| `scripts/preprocess_pref_boundaries.py` | N03 行政区域 GeoJSON を都道府県/振興局レベルに dissolve して軽量化（初回のみ実行） |

### 重要な実装詳細

- **標高デコード**: `elev = (R*65536 + G*256 + B) / 100.0` 、無効値 `(R=128,G=0,B=0)` → `-9999.0f`
- **境界処理**: メッシュ外周に 0m（海面）の 1px ボーダーを追加して、メッシュ端を海岸線とみなす
- **タイルサイズ**: 実画像は 256×256 px だが、隣接タイルとの境界を正確に処理するため 257×257 px（1px オーバーラップ）で管理
- **DEM フォールバック**: DEM5 が存在しないタイルは DEM10 で代替
- **無効標高のセンチネル**: `-9999.0f`
- **標高地形図 PNG**: `findsummits` 実行時に解析範囲を人間が視認しやすい配色で標高を色分けした PNG として出力。長辺 6000px に縮小して保存

### アーキテクチャ方針（ハイブリッド構成）

- **C エンジン** (`src/`): 標高デコード・Union-Find によるピーク/コル検出・標高地形図 PNG 出力・per-mesh CSV 出力（タイル取得は行わない）
- **Python スクリプト** (`scripts/`): タイル事前取得（prefetch_tiles.py）、N03 行政区域前処理（preprocess_pref_boundaries.py・初回のみ）、複数 CSV の統合、SOTA リスト突合、XLSX/GeoJSON 生成

C に XLSX/GeoJSON ライブラリを持ち込むコストが高く、`findsummits4sotaja`（Python）に出力生成コードが既存するため、この分担を採用。性能が必要な計算は C、申請用出力は Python。

### ディレクトリ構成

```
src/          # ソースファイル（main.c, *.c, *.h）
scripts/      # 本番パイプライン用スクリプト
  prefetch_tiles.py             # タイル事前取得（params/fetch_config.ini を参照）
  preprocess_pref_boundaries.py # N03行政区域前処理（初回のみ: $DATA_DIR/ref/N03-2026_regions.geojson 生成）
  merge.py                      # 複数CSV統合・SOTA突合・都道府県ベース仮コード割り振り
  run_all.sh                    # 全メッシュ一括解析ラッパー
analysis/     # 検証・解析用スクリプト（本番パイプライン外）
  analyze_keycol_distance.py    # Keyコル距離分析
params/       # パラメータファイル
  mesh_list_japan.txt           # 解析対象メッシュコードリスト
  config.ini.example            # DATA_DIR 設定テンプレート（コミット済み）
  config.ini                    # 実設定（gitignore）: DATA_DIR を記入
  fetch_config.ini.example      # UA・並列数設定のテンプレート（コミット済み）
  fetch_config.ini              # 実設定（gitignore・メールアドレス記入）
docs/         # 設計ドキュメント（git管理・devel/main 両ブランチ）
  00_GLOSSARY.md                           # 用語集
  01_environment.md                        # 環境定義
  10_URD.md                                # ユーザー要件定義書
  20_SRS.md                                # ソフトウェア要件仕様書
  decisions/                               # アーキテクチャ決定記録（ADR）
    research/                              # ADR 決定前の設計調査資料
ref/          # 参照データ（git管理）
  summitslist.csv                          # SOTAの山岳リスト（全サミット）
  SOTA-Summit-list-revision-request.xlsx   # SOTA日本支部への申請書テンプレート
  SOURCES.md                               # 参照資料の出典一覧
tests/        # テスト用プログラム（test_*.c）
mgmt/          # 管理ドキュメント（lessons.md, plan.md, tracker/）※ devel ブランチのみ・main には含めない
.claude-container  # 実設定（gitignore）: EXTRA_MOUNT でホストの /mnt/findsummits をコンテナ内にマウント

# 以下のパスは params/config.ini の DATA_DIR で設定する
# - claude-container 使用時: DATA_DIR = /data（コンテナ内パス）
# - 非コンテナ時: DATA_DIR = /path/to/your/data（ホストのデータパス）
$DATA_DIR/images/       # 標高地形図 PNG（findsummits が自動出力: <meshcode>_terrain.png）
$DATA_DIR/results/      # 最終O/Pのxlsx,geojson,csv
$DATA_DIR/results/csv/  # 一時csv（findsummits が出力するper-mesh CSV）
$DATA_DIR/tiles/        # ダウンロード済みタイルのローカルキャッシュ
  └─ {z}/     # タイルのURLの命名規則と同様
     └─ {x}   # タイルのURLの命名規則と同様
         └─ {y}
$DATA_DIR/logs/         # findsummits・prefetch_tiles のログ
```

## 開発ドキュメント管理

docs/ 配下を編集するときは採番・フォーマット・ADR ルールを `docs/CLAUDE.md` で確認すること。

## 欠陥管理ルール（必須）

**バグ・欠陥を発見しても、すぐに修正を始めてはならない。必ず以下のフローを守ること。**

1. テスト結果を確認し、発見した欠陥を**すべて洗い出してから**まとめて一覧提示する（1件見つけるたびに都度登録しない）
2. ユーザーに確認を取ってから `venv/bin/python3 mgmt/tracker/track.py bug add` で登録する
3. 原因がわかっている場合は `--resolution` に対応方針まで記入してから登録する
4. 登録完了後、修正作業の承認を得てから着手する
5. 修正完了後は `bug close` コマンドでステータスを「対応完了」にする（解決済にしない）
6. ユーザーが確認完了後、`bug verify` コマンドでステータスを「解決済」にする

詳細な運用手順・コマンド一覧は `mgmt/tracker/CLAUDE.md` を参照。

## 課題管理ルール

**課題管理（`mgmt/tracker/` issue）はプロジェクトの仕様・設計・調査・新機能に特化する。**
文書（URD / SRS / HLD / LLD / ADR / GLOSSARY 等）と紐づく議論を伴うものだけを `issue` に登録する。
それ以外の作業リスト（実装タスク・ファイル名追従・ログ整備・運用作業等）は `mgmt/todo.md` で管理する（後述「ToDo リスト運用ルール」）。
バグ（欠陥）は `track.py bug`、課題は `track.py issue` と使い分けること。

**判定基準: 文書・仕様の議論を伴うか？**
- Yes → `issue`（例: SRS の FR 追加、ADR 作成、SRS と実装の乖離調査）
- No  → `todo.md`（例: 関数名のリネーム、ログ書式の統一、設定ファイルの追従）

1. 新規の課題が発生したら `venv/bin/python3 mgmt/tracker/track.py issue add` で登録する
2. 作業開始時は `issue update --status 対応中` でステータスを更新する
3. 実装完了後は `issue close` コマンドでステータスを「対応完了」にする
4. ユーザーが確認完了後、`issue verify` コマンドでステータスを「解決済」にする
5. `stage` フィールドは次ステージ移行の判断材料として活用する

詳細な運用手順・コマンド一覧は `mgmt/tracker/CLAUDE.md` を参照。

トラッカーで担当者（`--actor`）にモデル名を記入する場合はバージョン番号なしで「Sonnet」「Opus」「Fable」とだけ書く（バージョンアップ追従の手間を避けるため）。

## ToDo リスト運用ルール

**作業リスト（仕様議論を伴わない実装タスク等）は `mgmt/todo.md` で管理する。**

- **用途**: 文書・仕様の議論を伴わない作業の保管庫（実装タスク・ファイル名追従・ログ整備・運用作業・チェックリスト等）
- **粒度**: 数行で書ける作業単位。実装ステップは箇条書きでチェックボックス化可
- **管理方法**: 完了したものは消す（履歴は git で追える）。長期保留中のものは「保留」セクションへ
- **新規発生時**: 「文書・仕様の議論を伴うか？」で判定し、No なら todo.md へ追記（issue 登録不要）

handover との関係:
- todo.md と issue は **原本**（永続的な作業リスト）
- handover はセッション終了時点の **スナップショット**。todo.md と issue の未対応分を抜粋して載せる

Issue から todo.md への降格判定:
- 文書・仕様の議論が不要 / 単独のファイル修正で完結 / 純粋な実装タスクのみ → todo.md へ移行可
- 移行時は `issue close` の理由欄に「todo.md に移行」と記載し、todo.md 側に転記する

## handover 実行時のルール

`/handover` を実行するとき（または手動で handover ドキュメントを作成するとき）は、
handover ファイルの本文を書き終えた後、**必ず以下の手順を実行する**。

1. Excel レポートの条件付き更新:
   ```bash
   venv/bin/python3 mgmt/tracker/track.py bug export --if-changed
   venv/bin/python3 mgmt/tracker/track.py issue export --if-changed
   ```
   `mgmt/tracker/data/` 配下の JSON が xlsx より新しい場合のみ再生成。変更がなければスキップ。

2. handover ファイルの末尾に「未対応バグ・課題サマリー」セクションを追記する:
   ```
   ## 未対応バグ・課題サマリー

   ### 未対応バグ（N件: 高=x / 中=y / 低=z）
   - BUG-XXX [高] タイトル
   - ...

   ### 未対応課題（N件: 高=x / 中=y / 低=z）
   - ISSUE-XXX [高/機能追加] タイトル
   - ...

   ※ 詳細は `mgmt/tracker/reports/{bugs,issues}_export.xlsx` または
     `venv/bin/python3 mgmt/tracker/track.py {bug,issue} show <ID>` で確認
   ```
   件数・集計値は `track.py bug list --open` / `track.py issue list --open` の結果から取得すること。

3. コミット漏れ確認・コミット:
   - `git status` で未コミットの変更を確認する
   - 変更がなければスキップ
   - 変更がある場合:
     - `params/config.ini` / `.claude-container` など機密・gitignore 対象が含まれていないことを確認
     - 変更内容から Conventional Commits 形式・本文日本語のメッセージを作成
     - ファイルを**個別指定**で `git add <files>` してコミット（`git add .` / `git add -A` は使わない）
     - handover ファイル・xlsx 更新分もこのコミットに含める
     - コミット後に `git status` でクリーンになったことを確認する

## ドキュメント更新時のルール

ユーザーはドキュメントを GitHub 上で確認しているため、ローカル編集だけでは確認できない。
**ドキュメントの更新が完了した直後（その作業ターン内）に必ず commit する**こと。push は別途指示があるまで不要。

対象ドキュメント:
- `docs/` 配下のすべてのファイル（URD/SRS/HLD/LLD/UT/IT/ST/OPS/GLOSSARY/environment）
- `docs/decisions/` 配下の ADR と research 資料
- `ref/SOURCES.md` などの参照資料
- `mgmt/plan.md`・`mgmt/lessons.md`（devel ブランチ運用ファイル）

手順:
1. 更新作業が一段落したら `git status` で対象を確認
2. **個別ファイル指定**で `git add <files>`（`git add .` / `-A` は禁止）
3. Conventional Commits 形式・本文日本語でコミット
4. push は別途指示があるまで行わない（ユーザーが任意のタイミングで push する）

例外: 同一作業内でコードと一緒に更新したドキュメントは、コードのコミットに含めて構わない（ドキュメント単独でのコミット分割は不要）。

## ブランチ運用ルール

- `devel → main` の直接マージ禁止
- リリース時は必ず `release/*` ブランチを介す:
  1. `git checkout -b release/vX.X`
  2. `git rm -r mgmt/`
  3. `git commit -m "chore: リリース用にmgmt/除外"`
  4. `git checkout main && git merge release/vX.X`

## その他
他のプロジェクトの参考コードは以下の場所にあります：
@../findsummits4sotaja/ # 以前、pythonで開発した時のプロジェクト。九州・四国を解析してSOTA日本支部に申請した時のもの。

### SOTA関連資料
- SOTAの山岳リスト(JAで始まるものが日本支部のサミット)
  https://www.sotadata.org.uk/summitslist.csv
- SOTA日本支部への山岳リスト更新申請書
  https://www.kawauchi.homeip.mydns.jp/sotajp/wp-content/uploads/2024/03/SOTA-Summit-list-revision-request.xlsx
