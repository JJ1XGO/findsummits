# フラグ命名の肯定形統一: key_col_unresolved → key_col_resolved + area_truncated → area_complete

## Context

直前のコミット `633e54b` で `is_tile_top` → `key_col_unresolved` にリネームしたが、
ユーザーから「`key_col_resolved` の方が良いのでは」という提案を受けた。

検討の結果、`key_col_unresolved` 単独だと否定形になり読みにくい一方、
肯定形に揃えるなら並列フラグ `area_truncated` も同時に `area_complete` に
変えないと真偽値方向が揃わないと判断（ユーザー合意）。

**最終方針**: 両フラグとも「`true = 正常状態`」で意味方向を統一する。

| 旧名 | 新名 | true の意味 |
|---|---|---|
| `key_col_unresolved` | `key_col_resolved` | コル確定済み（正常） |
| `area_truncated` | `area_complete` | ポリゴン完全（正常） |

## 対象ファイルと変更内容

### 1. `docs/02_SRS.md`（30+ 箇所）

#### `key_col_unresolved` → `key_col_resolved`（真偽値反転）

- **FR-006**: 「per-mesh CSV に `key_col_unresolved=true` を付与する」→「`key_col_resolved=false` を付与する」（解析範囲外時）
- **FR-007 出力カラム表**: カラム名 + 説明文を反転（`true` = 確定済み）
- **FR-008**:
  - 重複排除ロジック `(key_col_unresolved=false, コル標高最高)` → `(key_col_resolved=true, コル標高最高)`
  - 説明文: 通常で `key_col_unresolved=true`/`false` → `key_col_resolved=false`/`true`
  - stability 判定: `key_col_unresolved=true` が 1 件でも含まれる → `key_col_resolved=false` が 1 件でも含まれる
- **FR-009**: stability 値 `confirmed`: 「`key_col_unresolved=false`」→「`key_col_resolved=true`」、`unstable`: 「`key_col_unresolved=true` あり」→「`key_col_resolved=false` あり」
- **FR-013 GeoJSON フィーチャ**:
  - ピーク Point: プロパティ `key_col_unresolved`（true/false）→ `key_col_resolved`（true/false）
  - コル Point: 「`key_col_unresolved=true` の場合は含めない」→「`key_col_resolved=false` の場合は含めない」
  - LineString prominence_range: 同様に反転
- **FR-014**: トリガー条件 `key_col_unresolved=true` → `key_col_resolved=false`（全箇所）
- **FR-016**: `key_col_unresolved=true` のピーク → `key_col_resolved=false` のピーク
- **merged.csv カラム説明**: `key_col_unresolved`（true/false）→ `key_col_resolved`（true/false）

#### `area_truncated` → `area_complete`（真偽値反転）

- **FR-016**: 「`area_truncated=true` フラグを付与」→「`area_complete=false` を設定」、「`area_truncated=false` のポリゴンに置き換わる」→「`area_complete=true` のポリゴンに置き換わる」
- **FR-018**: `area_truncated=false`（完全） → `area_complete=true`（完全）、優先採用ロジック全箇所反転
- **FR-014**: トリガー条件 `area_truncated=true` → `area_complete=false`（全箇所）
- **FR-009 入力**: 「`key_col_unresolved` および `area_truncated` フラグが解消」→「`key_col_resolved` および `area_complete` が全 true である」（文意の正方向化）
- **FR-013 アクティベーションゾーン Polygon プロパティ**: `area_truncated` → `area_complete`
- **6.x 中間ファイル仕様**（line 708/770）: アクティベーションゾーンプロパティの記述 2 箇所反転
- **目次・FR-014 説明文** 等での出現箇所

#### ヘッダー
- 最終更新日: `2026-05-19`（既に更新済みのため変更不要）

### 2. `docs/decisions/ADR-004-level14-max-pooling-isolated-peaks.md`（多数）

- `key_col_unresolved=true` / `=false` → `key_col_resolved=false` / `=true`
- `area_truncated=true` / `=false` → `area_complete=false` / `=true`
- 単独で出現する `key_col_unresolved` / `area_truncated` → `key_col_resolved` / `area_complete`
- Alternatives 表内の旧名表記も反転（読み手の混乱を避けるため新名で記述）
- 注意: Consequences 2 の「`key_col_unresolved` の判定ロジックは変更なし」は新名で記述しつつ「判定方向は反転」を補足

### 3. `docs/decisions/ADR-008-dominant-peak-identification.md`

- Context 2: 「`key_col_unresolved=true` が残るピーク」→「`key_col_resolved=false` が残るピーク」
- Alternatives フォールバック節: 同様に反転
- 最終更新日は据え置き（`2026-05-19`）

### 4. `docs/00_GLOSSARY.md`

- 既存 `key_col_unresolved` 行を `key_col_resolved` に書き換え
  - 説明文を真偽値方向に合わせて書き直す:
    「コルが確定済みの場合 `true`、3×3 メッシュ解析範囲外でコルが未発見の場合 `false`。旧称 `is_tile_top` → `key_col_unresolved`（命名整理の経緯あり）」
- 新規 `area_complete` 行を追加
  - 説明文: 「アクティベーションゾーンポリゴンが解析範囲内で完結している場合 `true`、解析範囲外で途切れた場合 `false`。旧称 `area_truncated`」

### 5. `docs/mockup/viewer_mockup.html`（8 箇所）

- `key_col_unresolved: false` → `key_col_resolved: true`（4 箇所）
- `area_truncated: false` → `area_complete: true`(4 箇所)

### 6. `mgmt/lessons.md`

- 既存の「実装由来のフラグ名は仕様読者を混乱させる」教訓を更新
  - 経緯を 1 文補足: 「`key_col_unresolved` で `area_truncated` と並列にしたが、真偽値方向の統一を優先して `key_col_resolved` + `area_complete`（共に `true=正常`）に再リネーム。フラグ名は単独の読みやすさだけでなく、並列フラグ群との真偽値方向の統一も考慮する」

### 7. `mgmt/tracker/data/issues.json`

- **ISSUE-014** の `notes` に経緯追記:
  - 「2026-05-19 追加: `key_col_unresolved` を `key_col_resolved` に再リネーム + 並列フラグ `area_truncated` も `area_complete` に変更（共に `true=正常` で方向統一）」

### 8. `mgmt/tracker/reports/issues_export.xlsx`

- `track.py issue export --if-changed` で再生成

## 実装側（src/, scripts/）への影響

仕様優先原則のため、実装コードへの追従は本タスクのスコープ外。
直前のコミット `633e54b` のスコープに含めなかったのと同じ扱い。
将来の実装 ISSUE で対応する。

## 検証

1. `grep -rn "key_col_unresolved" docs/ mgmt/lessons.md` で残存ゼロ（research/ 配下は除外）
2. `grep -rn "area_truncated" docs/ mgmt/lessons.md` で残存ゼロ（research/ 配下は除外）
3. `grep -rn "key_col_resolved" docs/ | wc -l` と `grep -rn "area_complete" docs/ | wc -l` が新名で増えていることを確認
4. SRS の論理整合確認:
   - stability `confirmed` 条件と `unstable` 条件が排他で漏れがないか
   - FR-008 重複排除ロジックと FR-009 stability 判定が同じ方向で書かれているか
   - FR-014 トリガー条件と FR-016 ポリゴン生成判定が整合しているか
5. `track.py issue show ISSUE-014` で notes に新経緯が記録されているか
6. `mgmt/tracker/reports/issues_export.xlsx` の再生成

## 実行手順

1. SRS（`docs/02_SRS.md`）: `key_col_unresolved` 真偽値反転 → `area_truncated` 真偽値反転（Edit 多数）
2. ADR-004: 同様にリネーム + 真偽値反転
3. ADR-008: 同様
4. GLOSSARY: 既存項目を書き換え + `area_complete` 追加
5. モックアップ HTML: 8 箇所反転
6. lessons.md: 既存教訓に経緯補足
7. `track.py issue update ISSUE-014 --notes "..."` で経緯追記
8. `track.py issue export --if-changed`
9. `git add <files>` → Conventional Commits（本文日本語）で commit

## 関連ファイル

- `docs/02_SRS.md`
- `docs/decisions/ADR-004-level14-max-pooling-isolated-peaks.md`
- `docs/decisions/ADR-008-dominant-peak-identification.md`
- `docs/00_GLOSSARY.md`
- `docs/mockup/viewer_mockup.html`
- `mgmt/lessons.md`
- `mgmt/tracker/data/issues.json`
- `mgmt/tracker/reports/issues_export.xlsx`
