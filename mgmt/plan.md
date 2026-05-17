# Plan: ADR-010 ドラフト作成（C++/OpenCV 全面移行）

## Context

FR-016（アクティベーションゾーン計算）の実装を検討する中で、より広範な技術判断が必要であることが明らかになった。

**経緯**:
- FR-016 はピクセル群 → ポリゴン化（Flood Fill + 輪郭抽出）が必要で、これは画像処理の典型タスク
- ユーザー希望: 自前ロジック実装ではなく OpenCV を活用し、コードの信頼性と将来性を確保したい
- 適用範囲は FR-016 単独ではなく、標高タイル読込（libpng→cv::imread）・標高地形図出力（FR-015）も含めて統一したい
- OpenCV は実質 C++ 必須（C API は deprecated → 削除済み）のため、src/ 全体を C++ に移行する必要がある

**現行 ADR との関係**:
- ADR-001（C + Python ハイブリッド）の C エンジン部分が C++ に変わる
- ADR-001 を Superseded by ADR-010 として状態更新する必要あり
- ハイブリッド原則（性能要求は C++、出力フォーマットは Python）は維持

**今フェーズの目的**: 実装ではなく、**ADR-010 ドラフトを作成して意思決定を文書化**する。実装は ADR 採用後、別 ISSUE で段階的に進める。

## Decision Summary

**採用**: src/ 全体を C → C++ に移行し、画像処理関連は OpenCV を活用する。

**段階実行**:
| Phase | 内容 | 規模 | 検証ポイント |
|---|---|---|---|
| Phase 1 | ビルド基盤 + `elevation.c` → C++/cv::imread 化 | 中 | **標高値の libpng と完全一致** |
| Phase 2 | `mesh.c`/`unionfind.c`/`analyze.c`/`mesh_analyze.c`/`main.c` を C++ 翻訳 | 大 | 既存テスト全通過 + 実メッシュで CSV 同値 |
| Phase 3 | FR-015 標高地形図を OpenCV (applyColorMap + resize) で書き直し | 小 | 既存出力と視覚比較 |
| Phase 4 | FR-016 を `cv::floodFill` + `cv::findContours` で新規実装 | 中 | per-mesh GeoJSON 出力検証 |

## 今回の作業: ADR-010 ドラフト作成

### 作成ファイル

- **`docs/decisions/ADR-010-cpp-opencv-migration.md`** （新規）
- **`docs/decisions/research/cpp-opencv-migration-research.md`** （新規・検討資料）

### ADR 構造（既存 ADR-001 フォーマットに準拠）

ヘッダーテーブル（状態=検討中・決定日=2026-05-17）の後、以下のセクションを含む:

- **Context**: FR-016 経緯 / 自前実装 vs OpenCV / C API 廃止 / C++ 化必須の技術的経緯
- **Decision**: 全面 C++ 化 / OpenCV 採用 / ADR-001 ハイブリッド原則維持 / 段階実行
- **Alternatives**: C のまま自前実装 / FR-016 のみ C++ 化 / Python 回帰
- **Consequences**: Positive・Negative・Migration Plan（Phase 1〜4）
- **関連ドキュメント**: ADR-001（Superseded by）/ FR-015 / FR-016
- **検討資料**: research/cpp-opencv-migration-research.md

### 検討資料の内容

- OpenCV vs 自前実装の比較表
- メモリ見積（4 GB mask 追加の影響、56.9→60.9 GB）
- cv::imread の Terrain-RGB 完全デコード確認方法（標高値同値性検証手順）
- C++ 移行で踏まない範囲（テンプレート・ABI 等を使わない方針）
- 段階移行の検証手順詳細

### ADR-001 の更新

- ステータスを「採用・実装済み」→「**採用・実装済み（ADR-010 により部分置換予定）**」に変更
- 末尾に「ADR-010 で C 部分が C++ に移行される」旨を追記

### URD/SRS への影響

- 今回は ADR ドラフトのみ
- SRS の「C エンジン」表記の修正は Phase 1 着手時に別タスクとして実施

### 関連 ISSUE の登録（ADR 採用後）

ADR-010 が採用された後、以下を `mgmt/tracker/track.py issue add` で登録:

| 種別 | 優先度 | タイトル |
|---|---|---|
| COD | 高 | Phase 1: ビルド基盤 + elevation.c の C++/OpenCV 化（標高値同値性検証含む） |
| COD | 高 | Phase 2: 残り src/ の C++ 翻訳 |
| COD | 中 | Phase 3: FR-015 の OpenCV 化 |
| COD | 高 | Phase 4: FR-016 を C++/OpenCV で実装 |

既存 ISSUE への影響:
- ISSUE-013（隣接メッシュ拡張ロジック保留）は Phase 2 で対応方針再検討
- ISSUE-021〜025 等の SRS 改善は Phase 進行に依存しない

## 作業手順

1. `docs/decisions/ADR-010-cpp-opencv-migration.md` を新規作成
2. `docs/decisions/research/cpp-opencv-migration-research.md` を作成
3. `docs/decisions/ADR-001-hybrid-c-python-architecture.md` の状態欄を更新
4. ユーザーレビュー → 採用判断
5. （採用後）`mgmt/tracker/track.py issue add` で Phase 1〜4 を登録

## 検証

ADR ドラフトの品質基準:
- 既存 ADR フォーマット（ヘッダーテーブル・Context・Decision・Alternatives・Consequences）に準拠
- 「なぜ今このタイミングで判断するのか」の経緯が明示されている
- 段階移行の各 Phase が独立検証可能であること
- リスク（標高値同値性・メモリ・依存追加）が漏れなく記載されている
- ADR-001 との関係（部分置換）が明確化されている

## 関連ファイル

- 作成: `docs/decisions/ADR-010-cpp-opencv-migration.md`
- 作成: `docs/decisions/research/cpp-opencv-migration-research.md`
- 更新: `docs/decisions/ADR-001-hybrid-c-python-architecture.md`
- 参照のみ: `docs/02_SRS.md`（FR-015, FR-016, アーキテクチャ概要セクション）
- 参照のみ: `src/elevation.c` `src/mesh_analyze.c` `src/analyze.c`（現状把握）

## このプランの範囲外（次回以降）

- 実装（Phase 1〜4）は別 ISSUE
- SRS / HLD の C++ 表記への更新
- Makefile の g++/OpenCV 化
- コンテナイメージへの OpenCV インストール手順
