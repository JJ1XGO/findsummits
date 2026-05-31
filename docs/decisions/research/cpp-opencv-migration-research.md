# 調査資料: C++/OpenCV 全面移行の検討

ADR-SRS-010 の判断根拠として、技術的な検証ポイント・比較・想定リスクをまとめる。
本資料は意思決定のための生の検討資料であり、整形・章立てを優先しない。

---

## 1. OpenCV vs 自前実装の比較（FR-016 を例に）

### Flood Fill

| 項目 | 自前実装（C） | OpenCV (`cv::floodFill`) |
|---|---|---|
| 実装規模 | BFS キュー + visited 配列で ~80 行 | 1 行（API 呼び出し） |
| 連結性 | 4/8 連結を自分で実装 | 引数で `4 \| FLOODFILL_FIXED_RANGE` 指定 |
| 閾値モード | 自前で「seed からの絶対差」を実装 | `FLOODFILL_FIXED_RANGE` フラグで切替 |
| マスク出力 | 別配列を返す設計が必要 | `FLOODFILL_MASK_ONLY` + `(255 << 8)` で mask に書き込み |
| バウンディング矩形 | 自分で min/max 追跡 | API が `cv::Rect*` で返却 |

cv::floodFill の典型コード:

```cpp
cv::Mat mask(H + 2, W + 2, CV_8UC1, cv::Scalar(0));  // OpenCV 要求: 入力より +2px
cv::Rect bbox;
cv::floodFill(elev_mat, mask, seed_point,
              cv::Scalar(0),       // newVal（mask-only モードでは未使用）
              &bbox,
              cv::Scalar(25.0),    // loDiff: seed から下方向に許容する差
              cv::Scalar(0.0),     // upDiff: seed が局所最大なので 0
              4 | cv::FLOODFILL_FIXED_RANGE | cv::FLOODFILL_MASK_ONLY | (255 << 8));
```

### 輪郭抽出

| 項目 | 自前実装 | OpenCV (`cv::findContours`) |
|---|---|---|
| アルゴリズム | Moore-neighbor tracing または Suzuki-Abe を自前実装 | Suzuki-Abe 実装済み |
| 実装規模 | ~150 行（エッジケース込み） | 1 行 |
| 階層対応（穴あり） | 別途実装必要 | `RETR_TREE` モードで自動取得 |
| 戻り値 | std::vector<Point> 相当を自前 | `std::vector<std::vector<cv::Point>>` |

### 輪郭の頂点表現（lossless 方針）

FR-009 の point-in-polygon 突合精度を保つため、**ポリゴンの形状を変える簡略化（`cv::approxPolyDP` 等の Douglas-Peucker、等間隔抽出）は採用しない**。`cv::findContours` のモード選択で lossless に出力する:

| `cv::findContours` モード | 挙動 | 形状保持 | 採用 |
|---|---|---|---|
| `CHAIN_APPROX_NONE` | ピクセル境界の全頂点を返す | 完全保持 | 候補 |
| `CHAIN_APPROX_SIMPLE` | 直線上の冗長な中間点を削除（端点のみ残す） | 完全保持（数学的に同一） | **採用** |
| `CHAIN_APPROX_TC89_*` | Teh-Chin の折れ線近似 | わずかに lossy | 不採用 |
| `cv::approxPolyDP`（追加適用） | Douglas-Peucker による簡略化 | lossy | 不採用 |

`CHAIN_APPROX_SIMPLE` は「水平・垂直の長い直線セグメント上で中間頂点を省略する」だけで、ポリゴンの実形状は `NONE` と数学的に同一。ファイルサイズも `NONE` より自然に圧縮される。

### ファイルサイズ見積（lossless 採用時）

- 大きな AZ（例: 1km × 1km ≈ 400×400 px）の外周頂点数 ≈ 1600 頂点（CHAIN_APPROX_NONE 想定）
- `CHAIN_APPROX_SIMPLE` で水平/垂直直線が圧縮され、実質頂点数は数百レベルまで縮減（地形依存）
- 9 メッシュで数十〜数百ピーク → per-mesh GeoJSON 数 MB
- 全国（メッシュ数百件）統合で `merged_activation.geojson` が数百 MB 級になる可能性
- ファイルサイズが viewer 表示等で問題化した場合は、後段の merge.py 等で別途圧縮（gzip）や表示用と突合用の分離を検討。**突合精度を保つ方が優先**という方針は維持

### PNG デコード

| 項目 | 現行（libpng） | OpenCV (`cv::imread`) |
|---|---|---|
| 実装規模 | `elevation.c` の PNG デコードロジック ~100 行 | 1 行 |
| エラー処理 | libpng のセットアップ・エラージャンプ等 | 戻り値 `cv::Mat::empty()` チェック |
| カラーモード制御 | `png_set_*` 呼び出し群 | `IMREAD_UNCHANGED` フラグ |

---

## 2. メモリ見積

### 現行（C 実装）の 9 メッシュ最大時メモリ（lessons.md 実測ベース）

| 配列 | サイズ |
|---|---|
| 結合画像 elevation float[H*W] | ~16 GB |
| Union-Find parent[H*W] uint32_t | ~16 GB |
| Union-Find rank[H*W] int8_t | ~4 GB |
| analyze processed[H*W] int8_t | ~4 GB |
| 他（peaks 配列、indices 等） | 数 GB |
| **合計（実測ピーク）** | **約 56.9 GB / 62.72 GB マシン** |

### OpenCV 化後の追加メモリ

| 追加配列 | サイズ |
|---|---|
| FR-016 floodFill mask cv::Mat (CV_8UC1, H+2 x W+2) | ~4 GB |
| 各 FR-016 一時バッファ（contour 等） | 数 MB（領域サイズ依存・小さい） |
| **合計増加** | **約 4 GB → 60.9 GB / 62.72 GB マシン** |

mask は全ピーク間で使い回し（`cv::Mat::setTo(0)` または `mask = 0`）するため、ピーク数に対して線形には増えない。

### リスク

- 9 メッシュ中心メッシュ（5339）でスワップ使用しつつ完走している現状において、+4 GB は実用上ぎりぎり
- 必要なら mask を bbox 限定で確保するなどの最適化余地があるが、まずは素直な実装で計測する

---

## 3. cv::imread の Terrain-RGB デコード同値性検証

Phase 1 の最重要検証ポイント。標高値は `elev = (R * 65536 + G * 256 + B) / 100.0` で計算されるため、PNG デコード結果が 1 ビットでも異なると標高 float が変わる。

### 検証手順（Phase 1 着手時に実施）

1. 既知タイル（例: `$DATA_DIR/tiles/15/29097/12863.png`）を libpng と cv::imread の両方でデコード
2. 全 256×256 ピクセルの (R, G, B) を比較
3. 差分があれば、PNG の保存形式（インターレース・カラータイプ・ビット深度等）と OpenCV のデフォルト挙動を確認
4. `IMREAD_UNCHANGED` フラグで raw RGB が取れることを確認
5. cv::Mat はデフォルトで BGR 順なので、Terrain-RGB → 標高変換時に B/R のチャンネル順に注意

### 想定される非互換ケース

- OpenCV はガンマ補正・色プロファイル変換を行わない `IMREAD_UNCHANGED` で読めば原理上完全一致するはず
- ただし PNG パレット形式（GSI はそうしないと思うが）の場合は展開挙動が異なる可能性
- 念のため、ランダムサンプル 100 枚で全ピクセル一致を確認すれば十分

### 検証失敗時のフォールバック

万一同値性が崩れる場合は、libpng を残して cv::imread の使用を見送る選択肢もある（OpenCV は他の機能で使用継続）。ただしこのケースは現実的にはほぼ起きない見込み。

---

## 4. C++ 利用方針と踏まない罠

C++ には初学者が踏みがちな複雑性が多数あるが、本プロジェクトでは以下を方針として複雑機能を意図的に回避する。

### 採用する機能（必要十分）

- **RAII**: コンストラクタ／デストラクタで `new`/`delete`、`fopen`/`fclose` を自動化
- **`std::vector<T>`**: malloc/realloc の代替
- **`std::string`**: char* + strdup の代替
- **参照（`&`）**: ポインタの代替（null になりえない引数）
- **`const` 厳格化**: 読み取り専用引数の明示
- **`cv::Mat`**: 画像データ管理（リファレンスカウント付きで効率的）
- **`auto` キーワード**: 型推論で冗長な記述削減（ループのイテレータ等に限定）

### 採用しない機能（複雑性源）

- **テンプレートメタプログラミング**: SFINAE, concepts, 等
- **複雑な継承階層**: 仮想関数を 1 階層だけ使う程度に留める
- **独自例外階層**: OpenCV の例外をそのまま受ければよい
- **ムーブセマンティクスの最適化**: コンパイラ任せで十分
- **`std::variant` / `std::optional` の多用**: ポインタ + null チェックで足りるケースは複雑化させない
- **演算子オーバーロード**: 標準型と OpenCV 型の既存オーバーロードのみ使用
- **ヘッダオンリーライブラリの追加**: pkg-config で扱えるシステムライブラリに限定

### 翻訳ガイドライン

C → C++ 翻訳は「機械的変換」を基本とする:

| C パターン | C++ 翻訳 |
|---|---|
| `T* arr = malloc(N*sizeof(T))` + `free(arr)` | `std::vector<T> arr(N)` |
| `struct S { ... }` + 関連関数群 | `class S { ... }`（メソッド化はしない、関連関数 namespace 化も可） |
| `static void func(...)` | `static` 関数のまま（クラス化しない） |
| ポインタ引数 | 必須かつ null にならないものは参照 `T&`、それ以外はポインタ維持 |
| `printf` で構造化エラー | OpenCV エラーは cv::Exception、ロジックエラーは `std::runtime_error` |

過剰な「C++ らしさ」の追求は避け、現行 C コードの構造を保持したまま安全性のみ底上げする。

---

## 5. 段階移行の検証手順詳細

### Phase 1 検証

1. `make` で C++ ビルドが通る
2. `test_mesh_analyze 4929` が以前と同じ結果を出力
3. 既知タイルでの PNG デコード結果が libpng と完全一致（前述）
4. 9 メッシュ実行でメモリ消費が ±10% 以内に収まる

### Phase 2 検証

1. 既存テスト（`test_mesh_analyze`, `test_analyze`）が全通過
2. 既知メッシュ（例: 4929, 5339）の per-mesh CSV が**バイト単位で**現行実装と一致
3. 標高地形図 PNG が現行実装と一致（cv::imread/imwrite で再生成しなければ）

### Phase 3 検証

1. 標高地形図 PNG が新カラーマップ実装で生成される
2. 可読性が現行と同等以上（目視確認）
3. 長辺 6000px の縮小が正しく動作

### Phase 4 検証

1. per-mesh `<meshcode>_activation.geojson` が生成される
2. 既知のピークについて、地理院地図にアクティベーションゾーンを重ねて目視確認
3. SOTA ルール（25m 以内連続）に従ったエリアになっている
4. `area_truncated` フラグが解析範囲端でのケースで正しく付与される
5. コル等高線ポリゴンが `feature_type="key_col_boundary"` で出力される
6. `is_tile_top=1` のピークではコル等高線が出力されない

---

## 6. ビルド基盤の変更概要（Phase 1 詳細は別途）

### Makefile

- `CC=gcc` → `CXX=g++`
- `CFLAGS` → `CXXFLAGS`（`-std=c++17`）
- `LDLIBS` に `pkg-config --libs opencv4` を組み込み
- `-lpng` を削除
- ターゲット: `findsummits`, `test_mesh_analyze`, `test_analyze` のソース拡張子を `.c` → `.cpp` に変更

### コンテナ環境

- `apt install libopencv-dev`（または `python3-opencv` を含むパッケージ）を Dockerfile に追加
- イメージサイズは ~100 MB 増加（実測値は別途確認）

### CI / 環境ドキュメント

- `docs/environment.md` の依存ライブラリリストを更新
- `CLAUDE.md` のビルド説明を更新

---

## 7. 採用判断のためのチェックリスト

ユーザーが ADR-SRS-010 を採用する判断材料:

- [ ] 移行期間（2〜3 週間）に FR-016 着手が遅延することを許容できるか
- [ ] OpenCV 依存（コンテナイメージ +100 MB）を許容できるか
- [ ] Phase 1 の標高値同値性検証で失敗するリスクを許容できるか（実用上ほぼ起きないと想定）
- [ ] C++ 採用後の長期保守を許容できるか
- [ ] 段階移行（Phase 1〜4）の進め方に同意できるか
