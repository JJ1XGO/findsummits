# ADR-010: C++/OpenCV への全面移行

| 項目 | 内容 |
|---|---|
| 状態 | 採用・未実装 |
| 決定日 | 2026-05-17 |
| 調査資料 | [`research/cpp-opencv-migration-research.md`](research/cpp-opencv-migration-research.md) |

---

## Context

FR-016（アクティベーションゾーン計算）の実装方法を検討する中で、より広範な技術判断が必要であることが明らかになった。

### FR-016 が要求する処理

FR-016 はピーク位置を起点とした Flood Fill によりアクティベーションゾーン（標高差 25m 以内の連続エリア）を抽出し、その外周輪郭を GeoJSON Polygon として出力する。同じ Flood Fill を `max(col_elev, peak_elev - delete_zone_max_drop)` 閾値で実行して delete判定ゾーンポリゴンも生成する（[ADR-011](ADR-011-delete-zone-polygon.md)）。これらはいずれも**画像処理の典型タスク**（領域塗りつぶし・輪郭抽出）であり、自前実装すると以下のコストが発生する:

- BFS による Flood Fill（4 連結／8 連結の選択、訪問配列管理）
- 境界追跡アルゴリズム（Moore-neighbor / Suzuki-Abe 等）の実装とエッジケース対応
- 穴あき領域や退化ケース（1 ピクセル領域等）への対応

これらは OpenCV の `cv::floodFill`・`cv::findContours` で枯れた実装が提供されている。

なお、FR-009 の point-in-polygon 突合精度を確保するため、ポリゴンの**形状を変える簡略化（Douglas-Peucker 等）は採用しない**。`cv::findContours` の `CHAIN_APPROX_SIMPLE` モード（直線上にある冗長な中間頂点のみを削除する lossless 圧縮）で出力する。

### OpenCV 適用範囲は FR-016 だけにとどまらない

OpenCV を導入する場合、FR-016 単独ではなく以下にも統一的に適用するのが自然である:

| 現状処理 | OpenCV 代替 | 該当機能 |
|---|---|---|
| libpng で標高タイル PNG をデコード | `cv::imread(path, IMREAD_UNCHANGED)` | FR-003 |
| RGB→標高変換 `(R*65536+G*256+B)/100` | cv::Mat の split + 行列演算 | FR-003 |
| 標高地形図 PNG（色分け・縮小・出力） | `cv::applyColorMap` / LUT + `cv::resize` + `cv::imwrite` | FR-015 |
| Flood Fill・輪郭抽出（lossless） | `cv::floodFill` / `cv::findContours`（`CHAIN_APPROX_SIMPLE`） | FR-016 |

### C++ 化の必然性

OpenCV 4.x は **C API が deprecated 後に削除されており**、実用上は C++ API のみが提供されている。C から OpenCV を呼ぶには `extern "C"` ラッパーを多数書く必要があり、その負担を払うくらいなら src/ 全体を C++ に移行したほうが整合性が取れる。

加えて、現行の C 実装には以下の構造的負担がある:

- `malloc` / `free` / `realloc` の手動管理が随所に存在（過去に `load_mesh_tile()` での書き込み後リセットバグ等が発生）
- 動的配列を `realloc` で都度拡張するボイラープレート
- メモリ解放漏れの静的検証が困難

C++ の RAII（コンストラクタ／デストラクタによる自動メモリ解放）・`std::vector`・`cv::Mat` を活用すれば、これらが構造的に解消される。

### 現行 ADR との関係

ADR-001（C + Python ハイブリッドアーキテクチャ）は「C エンジン + Python スクリプト」という分業を定めている。本 ADR は **C エンジン部分を C++ に置き換える**ものであり、ハイブリッド原則（性能要求は C/C++、出力フォーマットは Python）は維持される。ADR-001 の状態を「ADR-010 により部分置換予定」に更新する。

ADR-002（DEM 階層フォールバック）・ADR-004（L14 max pooling 広域再解析）等の他の ADR は、いずれも C エンジン内部の実装方針を定めるもので、本 ADR とは独立。C++ 移行後も同じ方針を継承する。

## Decision

1. **src/ 全体を C → C++ に移行する**（mesh, elevation, unionfind, analyze, mesh_analyze, main およびテスト群）
2. **画像処理関連は OpenCV を採用する**:
   - PNG デコード（`cv::imread`）
   - 色変換・LUT 適用（`cv::applyColorMap` / `cv::LUT`）
   - リサイズ（`cv::resize`）
   - Flood Fill（`cv::floodFill`）
   - 輪郭抽出（`cv::findContours`、lossless 出力のため `CHAIN_APPROX_SIMPLE` モードを使用。形状を変える簡略化は採用しない）
3. **ADR-001 のハイブリッド原則を維持**: 性能要求のある処理は C++、出力フォーマット要求は Python の境界は変えない。per-mesh CSV と per-mesh GeoJSON が境界ファイルとなる
4. **段階的移行を採用**: 各 Phase 完了時点で既存テスト全通過＋実メッシュでの動作同値性を検証してから次へ進む

### 段階実行プラン

| Phase | 内容 | 規模 | 主な検証ポイント |
|---|---|---|---|
| Phase 1 | ビルド基盤の C++ 化（Makefile を g++ + pkg-config 化）<br>`elevation.c` を C++ + `cv::imread` 化 | 中 | **タイル PNG デコード後の標高 float 値が現行 libpng 実装と完全一致**すること |
| Phase 2 | `mesh.c`, `unionfind.c`, `analyze.c`, `mesh_analyze.c`, `main.c` および tests/ を C++ 翻訳<br>（malloc → std::vector、構造体 → class への機械的変換主体） | 大 | 既存テスト（`test_mesh_analyze`, `test_analyze`）が全通過 + 既知メッシュの per-mesh CSV が現行と完全一致 |
| Phase 3 | FR-015 標高地形図を `cv::applyColorMap` + `cv::resize` + `cv::imwrite` で書き直し | 小 | 既存出力との視覚比較（同等の可読性であれば可） |
| Phase 4 | FR-016 を `cv::floodFill` + `cv::findContours` で新規実装 | 中 | per-mesh `<meshcode>_activation.geojson` を実メッシュで出力し、地理院地図上で目視確認 |

Phase 1〜2 は既存機能の動作維持が目的であり、新機能追加は伴わない。Phase 3 は内部実装の置換のみで仕様変更なし。Phase 4 で初めて FR-016 として新機能を追加する。

### C++ 利用の方針（複雑機能の意図的回避）

過剰な C++ 機能を導入してメンテナンスコストを高めないよう、以下を方針とする:

- 採用する: RAII、`std::vector`、`std::string`、参照、コンストラクタ／デストラクタ、`const` 厳格化、`cv::Mat`
- 採用しない: テンプレートメタプログラミング、独自例外階層、複雑な継承、ムーブセマンティクス最適化、`std::variant` 等の高度な型機能
- C コードからの翻訳時は「C の構造を C++ で書き直す」程度の翻訳に留め、過剰な抽象化はしない

## Alternatives

| 案 | 却下理由 |
|---|---|
| C のまま自前実装で FR-016 を完成させる | 境界追跡・輪郭抽出のエッジケース実装コストが高く、バグリスクも大。FR-014 や将来の画像処理拡張のたびに同様の自作が必要になる |
| FR-016 のみを C++ モジュール化（折衷案） | C から呼ぶための `extern "C"` ラッパーが煩雑。OpenCV の戻り値型（`std::vector<std::vector<cv::Point>>` 等）を C 側で扱うのが現実的でない。結局フル C++ 化したくなる |
| Python 単体実装に回帰（findsummits4sotaja 方式） | 大規模メッシュでのメモリ・速度要件を満たせない懸念から ADR-001 で却下済み。本判断でもその前提は維持 |
| OpenCV を採用せず別の C++ 画像処理ライブラリ（CImg, GIL 等）を採用 | 採用例・コミュニティ規模・ドキュメント量で OpenCV が圧倒的。Terrain-RGB の用途で他ライブラリを選ぶ理由がない |

## Consequences

### Positive

- **メモリ管理の構造的安全化**: malloc/free 漏れが物理的に発生しない（RAII による自動解放）
- **画像処理のバグリスク低減**: OpenCV は数十年の運用実績があり、自前実装より圧倒的に堅牢
- **コード量の削減**: Flood Fill・輪郭追跡・PNG デコード・色変換・リサイズが OpenCV API 呼び出しに置き換わる
- **将来の拡張性**: FR-014（L14 max pooling 広域再解析）や、将来検討される画像処理機能（地形分類・等高線抽出・自動カラーマップ調整等）の追加コストが下がる
- **テスト可能性向上**: cv::Mat はファイル入出力・ピクセル比較が容易で、回帰テストの整備が現行より楽になる

### Negative

- **移行期間中の FR-016 着手遅延**: Phase 1〜2 で実装 1〜2 週間 + 検証 1 週間 = 計 2〜3 週間程度を見込む必要があり、その間 FR-016 は着手できない
- **OpenCV 依存追加**: コンテナイメージサイズが ~100 MB 増加。`libopencv-dev` のインストールが必要
- **Phase 1 の同値性検証リスク**: cv::imread と libpng の PNG デコード結果が microscopic に異なると、既存解析結果との完全一致が崩れる可能性がある。検証手順は調査資料で詳述
- **ビルド時間の増加**: C++ コンパイル + OpenCV ヘッダの取り込みで現行より遅くなる（数倍程度の想定）
- **チーム規模が小さいプロジェクトでの C++ 採用は学習コスト**: ただし本プロジェクトは個人開発であり、ユーザーが C++ 採用を許容している前提

### Migration Plan（実装時）

Phase 1〜4 の詳細手順・検証手順は採用後に各 ISSUE として登録し、そこで管理する。本 ADR では Phase 構成と検証ポイントのみ確定させる。

### 既存ドキュメントへの波及

- **ADR-001**: 状態を「採用・実装済み（ADR-010 により C 部分が C++ に置換予定）」に更新
- **SRS（02_SRS.md）のアーキテクチャ概要**: 「C エンジン」表記を「C++ エンジン」に変更（Phase 1 着手時に実施）
- **CLAUDE.md**: 「依存: libpng, libm, pthread（GCC / C99）」を「依存: OpenCV, libm, pthread（g++ / C++17）」に更新（Phase 1 着手時に実施）

### 既存 ISSUE への影響

- **ISSUE-013**（隣接メッシュ拡張ロジック保留）: Phase 2 で対応方針を再検討
- **ISSUE-021〜025 等の SRS 改善**: Phase 進行に依存しない（並行進行可能）

## 関連ドキュメント

- [ADR-001: C + Python ハイブリッドアーキテクチャ](ADR-001-hybrid-c-python-architecture.md)（部分置換）
- [SRS FR-015: 標高地形図出力](../02_SRS.md#fr-015-標高地形図出力)
- [SRS FR-016: アクティベーションゾーン計算](../02_SRS.md#fr-016-アクティベーションゾーン計算)
- [調査資料: cpp-opencv-migration-research.md](research/cpp-opencv-migration-research.md)
