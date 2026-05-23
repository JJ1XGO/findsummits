# Plan: SRS 3.2 主要コンポーネント構成の抽象化（3.3・ADR-010 整合含む）

## Context

SRS `02_SRS.md` の「3.2 主要コンポーネント構成」が実装名（`findsummits` / `prefetch_tiles.py` / `merge.py` / `merged_viewer.html`）と実装言語（C / Python / JavaScript）を直書きしており、SRS の文書性質（What を書く層）から逸脱している。同様に「3.3 フェーズ分割の俯瞰」も `findsummits (C)` `merge.py (Python)` と引きずられている。

加えて ADR-010（採用済み・未実装）により C エンジンを C++ に移行する方針が決まっているが、3.2 表は現行 C 実装の名称・言語に固定されているため、C++ 移行時に陳腐化リスクがある。

仕様優先原則に従い、SRS は論理コンポーネント名と責務・主要 I/O で記述する。実装ファイル名・言語選定は ADR / HLD / LLD の管轄とし、SRS からは切り離す。

なお SRS 4 章以降（FR-002〜FR-013）は既に論理責務レベルで書かれており、3.2/3.3 のみが乖離しているため、本改修で SRS 全体の抽象度を一貫させる。

## 改修方針

### 1. SRS 3.2 主要コンポーネント構成（02_SRS.md:109-122）

**表スキーマ変更**:
- 現状: `コンポーネント | 言語 | 責務`
- 改修後: `コンポーネント | 責務 | 主要入力 | 主要出力`
- 「言語」列は削除（実装言語の決定は ADR-010 に委ねる旨を本節冒頭で注記）

**論理コンポーネント命名案**（実装名との対応関係はコメントで残さない／HLD/LLD で結びつける想定）:

| 論理コンポーネント | 責務 | 主要入力 | 主要出力 |
|---|---|---|---|
| タイル取得コンポーネント | DEM タイルを国土地理院から取得・キャッシュ | メッシュコード／取得設定 | キャッシュ済み PNG タイル |
| 行政区域前処理コンポーネント | 都道府県・振興局境界 GeoJSON を解析用形式に変換 | N03 行政区域 GeoJSON | 軽量化された境界 GeoJSON |
| 地形解析エンジン | DEM からピーク／コル／プロミネンス／AZ・delete判定ゾーンを検出 | キャッシュ済み PNG タイル | per-mesh CSV、per-mesh activation GeoJSON、標高地形図 PNG |
| 統合・突合コンポーネント | per-mesh 成果物を統合し SOTA リストと突合、不備フラグ判定 | per-mesh CSV／GeoJSON、SOTA リスト、行政区域 GeoJSON | merged.csv、merged_activation.geojson、exit code |
| 可視化生成コンポーネント | 統合結果から GeoJSON と HTML ビューアを生成 | merged.csv、merged_activation.geojson | merged.geojson、merged_viewer.html |
| 申請書生成 UI | HTML ビューア内でユーザー操作に応じ XLSX を生成 | merged_viewer.html、ユーザー操作 | 申請用 XLSX |

### 2. SRS 3.3 フェーズ分割の俯瞰（02_SRS.md:124-149）

- `findsummits (C)` → 「地形解析エンジン」
- `merge.py (Python)` → 「統合・突合コンポーネント」
- `prefetch_tiles.py` → 「タイル取得コンポーネント」
- `preprocess_pref_boundaries.py` → 「行政区域前処理コンポーネント」
- フロー図テキスト内の実装名引用箇所を全て論理コンポーネント名で置換
- もし図中に言語名（C/Python）が出ていれば削除

### 3. ADR-010 既存ドキュメントへの波及（ADR-010 行119-123）

- 現状: 「SRS 3.2 アーキテクチャ概要『C エンジン』を『C++ エンジン』に変更（Phase 1 着手時に実施）」
- 改修後: 「SRS 3.2/3.3 は論理コンポーネント名で記述するため、本 ADR の言語変更による影響を受けない（実装言語の選定は本 ADR で完結し、HLD/LLD で具体ファイル名・ビルド構成を扱う）」

## 触る箇所（critical files）

- `/workspace/docs/02_SRS.md` — 3.2（行109-122）と 3.3（行124-149）
- `/workspace/docs/decisions/ADR-010-cpp-opencv-migration.md` — 波及セクション（行119-123 付近）

## 触らないこと

- SRS 4 章以降（FR-002〜FR-013）: 既に論理責務レベルで書かれており整合済み
- SRS 3.1（システムコンテキスト）: 抽象度問題なし
- ADR-010 の Decision / Alternatives / Consequences 本体: 言語選定の意思決定は変更しない
- 実装コード（src/, scripts/）: 仕様優先原則、SRS フェーズではコード変更しない
- ADR-011 / GLOSSARY / 他 ADR: 今回のスコープ外

## 検証手順

1. `02_SRS.md` の 3.2 表に実装名（`findsummits` / `merge.py` 等）と言語列が残っていないことを目視確認
2. `02_SRS.md` の 3.3 フェーズ俯瞰に実装名と言語が残っていないことを目視確認
3. `grep -nE "findsummits|merge\.py|prefetch_tiles|preprocess_pref|merged_viewer\.html|\(C\)|\(Python\)" docs/02_SRS.md` で意図しない実装名引用が残っていないか確認
4. ADR-010 波及セクションが「SRS は論理コンポーネント名のため影響なし」に書き換わっていることを確認
5. SRS 4 章以降の FR から 3.2 へのテキスト参照（リンク）があれば、論理コンポーネント名で一貫しているか確認
6. 更新後、CLAUDE.md ルールに従い `docs/` 配下の変更を Conventional Commits 形式・本文日本語で個別ファイル指定コミット

## 後続作業

- 本補正コミット後、ISSUE-040〜042（C++ 移行 Phase 1〜2 実装着手）の際に、3.2 を更新する必要がなくなる（論理コンポーネント名のため実装言語変更の影響を受けない）
