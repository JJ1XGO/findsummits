# 計画: 開発ステージ・ADR の用語集登録と ADR 命名体系の見直し

## Context

開発ドキュメント（CLAUDE.md・URD・SRS・ADR）で `URD`・`SRS`・`HLD`・`LLD`・`ADR` など開発プロセス用語が多用されているが、用語集（`docs/00_GLOSSARY.md`）には未登録のため、ドキュメントを読み始めた人がこれらの略語を解読できない。

また、既存の `UR-XXX`（URD）・`FR-XXX`（SRS）・`NFR-XXX`（SRS）はステージコード化されているのに対し、`ADR-NNN` だけは無印で、参照されても「どのステージで決まった判断か」が番号だけでは想起できない。ADR 番号にステージ識別子を追加することで、参照時に位置づけが瞬時に分かるようにする。

ステージ識別子は **URD / SRS の2種類のみ** とする。理由: HLD/LLD/COD 等の下流ステージ文書が本プロジェクトではまだ作成されておらず、現時点の ADR はすべて URD（スコープ・要件）または SRS（機能・出力仕様）レベルの判断に分類できるため。HLD/LLD 文書を作成する段階で必要なら拡張する。

連番は既存の `ADR-SRS-001`〜`ADR-URD-014` をそのまま維持し、間に `URD` または `SRS` を挿入するだけ（例: `ADR-SRS-001` → `ADR-SRS-001`、`ADR-URD-005` → `ADR-URD-005`）。新規 ADR は次の連番（`ADR-015`）から、ステージ判定に従って `ADR-URD-015` または `ADR-SRS-015` を割り振る。

---

## 変更内容

### 1. 用語集に「開発プロセス用語」セクション追加

ファイル: `docs/00_GLOSSARY.md`（末尾に追加）

**登録する用語**:

開発ステージ（ウォーターフォール各段階）:

| 用語 | 正式名称 | 説明 |
|---|---|---|
| URD | User Requirements Document | ユーザー要件定義書。利用者視点での「何ができるべきか」を記述（`docs/01_URD.md`）。識別子: `UR-XXX` |
| SRS | Software Requirements Specification | ソフトウェア要件仕様書。システム視点での機能・非機能要件を記述（`docs/02_SRS.md`）。識別子: 機能要件 `FR-XXX` / 非機能要件 `NFR-XXX` |
| HLD | High-Level Design | 概要設計。アーキテクチャ・主要モジュール構成（`docs/03_HLD.md`、未作成） |
| LLD | Low-Level Design | 詳細設計。モジュール内部のアルゴリズム・データ構造（`docs/04_LLD.md`、未作成） |
| COD | Coding | 実装。`src/*.c`・`scripts/*.py` |
| UT | Unit Test | 単体テスト（`docs/05_UT.md`、未作成） |
| IT | Integration Test | 結合テスト（`docs/06_IT.md`、未作成） |
| ST | System Test | システムテスト（`docs/07_ST.md`、未作成） |
| OPS | Operations | 運用（`docs/08_OPS.md`、未作成） |

設計判断記録:

| 用語 | 正式名称 | 説明 |
|---|---|---|
| ADR | Architecture Decision Record | アーキテクチャ決定記録。アーキテクチャ上の重要な判断（実装方針・技術選択・スコープ決定）の Context / Decision / Alternatives / Consequences を記録する文書。`docs/decisions/` 配下に格納 |

ADR 命名規約サブセクション（用語表の直後に追記）:

```markdown
#### ADR 命名規約

`ADR-{STAGE}-NNN-kebab-case-description.md`

- `{STAGE}`: 判断が発生したステージ。**URD / SRS の2種類のみ**
  - 「上流ステージから見て最初に該当するステージ」を採用するルール
  - 現時点では HLD/LLD 文書が未作成のため、それらに相当する判断も SRS に分類する
- `NNN`: 3桁連番。ステージ種別を跨いだ全体通し番号（ステージごとには分けない）

例: `ADR-URD-005-northern-territories-exclusion.md`, `ADR-SRS-001-hybrid-c-python-architecture.md`
```

### 2. 既存 14 個の ADR をリネーム

`git mv` で履歴を保持してリネーム（連番は維持、間に URD または SRS を挿入）:

| 旧ファイル名 | 新ファイル名 | 判断種別 |
|---|---|---|
| ADR-SRS-001-hybrid-c-python-architecture.md | ADR-SRS-001-hybrid-c-python-architecture.md | アーキテクチャ構成 |
| ADR-SRS-002-dem-hierarchy-fallback.md | ADR-SRS-002-dem-hierarchy-fallback.md | データ取得設計 |
| ADR-SRS-003-3x3-mesh-analysis.md | ADR-SRS-003-3x3-mesh-analysis.md | 解析範囲設計 |
| ADR-SRS-004-level14-max-pooling-isolated-peaks.md | ADR-SRS-004-level14-max-pooling-isolated-peaks.md | アルゴリズム |
| ADR-URD-005-northern-territories-exclusion.md | ADR-URD-005-northern-territories-exclusion.md | スコープ判断 |
| ADR-SRS-006-viewer-background-tile-selection.md | ADR-SRS-006-viewer-background-tile-selection.md | 機能仕様 |
| ADR-URD-007-peak-match-status-terminology.md | ADR-URD-007-peak-match-status-terminology.md | 用語定義 |
| ADR-SRS-008-dominant-peak-identification.md | ADR-SRS-008-dominant-peak-identification.md | FR-009 仕様 |
| ADR-URD-009-takeshima-exclusion.md | ADR-URD-009-takeshima-exclusion.md | スコープ判断 |
| ADR-SRS-010-cpp-opencv-migration.md | ADR-SRS-010-cpp-opencv-migration.md | 実装言語選択 |
| ADR-SRS-011-delete-zone-polygon.md | ADR-SRS-011-delete-zone-polygon.md | FR-016 仕様 |
| ADR-SRS-012-terrain-image-downscaling-method.md | ADR-SRS-012-terrain-image-downscaling-method.md | 出力処理 |
| ADR-SRS-013-merged-geojson-as-central-data.md | ADR-SRS-013-merged-geojson-as-central-data.md | 出力仕様 |
| ADR-URD-014-gsi-tile-attribution-policy.md | ADR-URD-014-gsi-tile-attribution-policy.md | UR-011 新設 |

URD 分類: 005, 007, 009, 014（スコープ・要件・用語）
SRS 分類: 001, 002, 003, 004, 006, 008, 010, 011, 012, 013（機能・出力・実装方針）

### 3. 全参照の更新

ADR を参照しているすべてのファイルで `ADR-NNN` → `ADR-{URD|SRS}-NNN` に置換。

更新対象（事前 grep で網羅検索）:

- `CLAUDE.md`（ADR 命名規則セクションを含む）
- `docs/00_GLOSSARY.md`（既存の ADR-SRS-008・ADR-SRS-011 参照リンク）
- `docs/01_URD.md`
- `docs/02_SRS.md`
- `docs/decisions/*.md`（ADR 相互参照）
- `docs/decisions/research/*.md`
- `mgmt/plan.md`・`mgmt/lessons.md`
- `mgmt/tracker/data/*.json`（issue/bug の本文中の ADR 参照）
- `mgmt/tracker/CLAUDE.md`

検索コマンド: `grep -rEn "ADR-0[0-9]{2}" --include="*.md" --include="*.json"`（リネーム前に実行して網羅性を確認）

### 4. CLAUDE.md の ADR 管理ルール更新

`/workspace/CLAUDE.md` の「ADR 管理ルール」セクション:

**変更前**:
```
**命名規則**: `docs/decisions/ADR-NNN-kebab-case-description.md`（NNN は3桁連番）
```

**変更後**:
```
**命名規則**: `docs/decisions/ADR-{STAGE}-NNN-kebab-case-description.md`

- `{STAGE}` は URD または SRS（HLD/LLD 文書未作成のため当面この2種類）
- 「上流ステージから見て最初に該当するステージ」を採用
- `NNN` は3桁連番。ステージ種別を跨いだ全体通し番号
- 詳細は `docs/00_GLOSSARY.md` の「ADR 命名規約」を参照
```

---

## 実行順序

1. **事前 grep**: `grep -rEn "ADR-0[0-9]{2}" /workspace --include="*.md" --include="*.json"` で全参照箇所をリストアップし、置換漏れがないことを確認できる状態にする
2. **用語集に開発プロセス用語セクションを追加**（`docs/00_GLOSSARY.md`）
3. **CLAUDE.md の ADR 命名規則を更新**
4. **既存 14 ADR を `git mv` でリネーム**（履歴保持）
5. **全参照を一括置換**（Edit ツールで個別ファイルごとに実施。`sed -i` は使わない）
6. **再 grep で残存参照をゼロ確認**
7. **コミット**: 1コミットで一括（用語集追加 + リネーム + 参照更新）
   - メッセージ案: `refactor(docs): ADR命名にステージ識別子追加・用語集に開発プロセス用語登録`

---

## 検証

1. `grep -rEn "ADR-0[0-9]{2}-" /workspace --include="*.md" --include="*.json"`
   → リネーム後のファイル名（`ADR-{URD|SRS}-NNN-...`）以外にマッチがないこと
2. `ls /workspace/docs/decisions/ADR-*.md`
   → すべて `ADR-URD-` または `ADR-SRS-` プレフィックスで始まること（14個）
3. `git log --follow docs/decisions/ADR-SRS-001-hybrid-c-python-architecture.md`
   → 旧 `ADR-SRS-001-...` 時代の履歴が追跡できること
4. 用語集を Read で開き、開発プロセス用語セクションがフォーマット崩れなく追加されていること
5. ADR 内の相互参照リンクが新ファイル名を指していること（手動で 2〜3 個サンプリング）

---

## 影響範囲（参考）

- 既存14個のADRファイル名（破壊的、ただし `git mv` で履歴保持）
- ADRを参照している全文書（CLAUDE.md / URD / SRS / GLOSSARY / 他ADR / mgmt配下）
- 外部リンク: GitHub上のADR直リンクを共有している場合は失効する（共有先があれば手動更新）

## 残課題（本計画スコープ外）

- HLD/LLD 文書を作成するタイミングで、ADR の `{STAGE}` を `HLD` / `LLD` まで拡張するかを再検討
- 既存 ADR の SRS 分類のうち、HLD/LLD 寄りのもの（例: ADR-SRS-001 ハイブリッドC+Python、ADR-SRS-003 3×3メッシュ）を HLD/LLD 文書作成時に再分類するかを判断
