# FR-009 中心成果物リネーム + 仮サミットコード採番の設定可能化

## Context

FR-009 のレビューで以下2論点が確定した。いずれも **SRS/ADR への反映のみ**（仕様優先原則によりコードは後追い・本作業では触らない）。

1. **中心成果物 `merged.geojson` の和名が不適切**
   - 現状の和名「統合ピーク候補」が中間ファイル `merged_peak.geojson`（和名「統合ピーク候補 GeoJSON」）と重複。
   - さらに FR-009 は「ピーク中心 → サミット中心へ切り替わる節目」（SRS L830）なのに「ピーク候補」は内容と矛盾。
   - → ファイル名・和名を整理する。

2. **仮サミットコードの連番上限 `A99` が固定**
   - 初年度は新規ピーク候補が1エリアで100件を超える可能性があり、全件採番して様子を見たい。次年度以降は `A99` で十分な見込み。
   - → 上限を設定可能項目にし、運用年度で切り替えられるようにする。あわせて連番を 00 始まりに変更。

## 確定した変更内容

### 変更A: `merged.geojson` → `merged_summit.geojson` リネーム
- ファイル名: `merged.geojson` → **`merged_summit.geojson`**（`merged_summit.xlsx` とペア）
- 和名: 「統合ピーク候補」→ **「突合済み統合 GeoJSON」**
  - 「突合済み」で SOTA 突合**後**の最終成果物であることを示し、中間ファイル「統合ピーク候補 GeoJSON」（突合**前**）と区別する。
- 中身は変更なし（ピーク/コル/サミット Point・ゾーン Polygon・接続線・metadata を含む中心成果物）。

### 変更B: 仮サミットコード採番の設定可能化 + 00始まり化
- **2.2.1 設定可能項目に1行追加**:

  | 項目名 | 意味 | デフォルト値 | 許容範囲 |
  |---|---|---|---|
  | 仮サミットコード連番上限 | 同一エリアに採番する仮コードの最大 prefix 文字。この文字の `99` を超えたら異常終了 | A | A〜Z |

- **連番を 00 始まりに変更**: `A00`〜`A99`（1 prefix あたり**100件**）。`A99` の次（`B00`）が必要になったら異常終了。
- デフォルト `A` で現状同等（100件超で異常検知）。初年度だけ `Z` 等に設定すれば `A00`〜`Z99`（最大 2,574件）まで許容。次年度に `A` へ戻せば異常検知が復活。
- 表記規則の更新: `JAx/XX-A01` → `JAx/XX-A00`、`ZZ/ZZ-A01` → `ZZ/ZZ-A00`。

## 作業ステップ

### 1. issue 登録（2件・決着済みなので登録と同時に決定を記録）
```
venv/bin/python3 mgmt/tracker/track.py issue add ...  # 論点1: 中心成果物GeoJSONのリネーム・和名整理
venv/bin/python3 mgmt/tracker/track.py issue add ...  # 論点2: 仮コード連番上限の設定可能化・00始まり化
```
※ ADR/SRS 反映完了後に `issue close`（対応完了）。前回 ISSUE-111/112/113 と同じ運用。

### 2. ADR 作成（1論点1ADR）
- **`docs/decisions/ADR-SRS-030-rename-central-geojson-merged-summit.md`**（変更A）
  - 中心成果物を `merged_summit.geojson` にリネーム・和名「突合済み統合 GeoJSON」。
  - ADR-SRS-013（中心成果物の定義元）・ADR-SRS-029（GeoJSON命名のCSV整合）の延長として位置付け、両ADRから/への相互参照。
  - Alternatives: ①和名のみ変更しファイル名据え置き（XLSXとペアにならず据え置きの利点小）/ ②`merged.geojson` 維持（中間ファイルと和名衝突が残る）→ 却下理由を記載。
- **`docs/decisions/ADR-SRS-031-provisional-code-limit-configurable.md`**（変更B）
  - 固定上限（異常検知目的）→ 設定可能（運用年度で調整）への思想転換と、00始まり化の理由を記録。
  - Alternatives: ①A99固定維持（初年度の100件超に対応できない）/ ②無制限自動拡張（異常検知が完全に失われる）→ 却下理由を記載。

### 3. SRS 本文修正（`docs/20_SRS.md`）
- **変更A**（`merged.geojson` 18箇所 + 和名）: 主な箇所 L160-161, 224-225, 846, 909, 913, 929/934, 945, 1048, 1080, 1092, 1109, 1119 等。和名「統合ピーク候補（`merged.geojson`）」→「突合済み統合 GeoJSON（`merged_summit.geojson`）」。
- **変更B**: 2.2.1 表に1行追加（L119-128 の表）。FR-009 採番規定 L866, L869（「01 からリセット」→「00 からリセット」）, L870, L871, L879（連番上限を設定項目参照に書き換え）, L880。`ZZ/ZZ-A01` 系 L283, L838, L870。`summit_code` 例 L968, L1204。NFR-003 L1288。すべて `A01`→`A00`・上限記述を設定項目リンクへ。

### 4. 関連 ADR の整合更新（`merged_summit.geojson` へ）
- 本文中で `merged.geojson` を参照する ADR を最新名に更新（ADR は自己完結で参照名を最新に揃える）:
  - `ADR-SRS-013`（中心成果物定義元・リネーム経緯を追記）, `ADR-SRS-026`, `ADR-SRS-011`, `ADR-URD-016`, `ADR-URD-014`。

### 5. mockup 整合（任意・推奨）
- `docs/mockup/viewer_mockup.html` の例示 `JA/YN-A01` → `JA/YN-A00`（00始まりへ追従）。

### 6. コミット
- `docs/` 配下を個別 `git add` で Conventional Commits（本文日本語）。push は指示があるまで行わない。

## 検証

```bash
# 変更A: 旧名の残存ゼロ（merged_peak/summit/viewer を除く）
grep -rn "merged\.geojson" docs/ | grep -v "merged_peak\|merged_summit\|merged_viewer"   # → 0件

# 変更B: 旧表記 A01 / 旧上限記述の残存ゼロ
grep -rn "A01\|01 からリセット\|A99 まで" docs/   # → 想定残存のみ（過去ADRの決定記録等）を目視確認

# 和名の重複解消（「統合ピーク候補」は merged_peak のみに限定されること）
grep -rn "統合ピーク候補" docs/20_SRS.md

# Markdown 内部リンク切れがないこと（2.2.1 へのアンカー追加分含む）
```

- 上記 grep が想定どおりであること、SRS の入出力テーブル・処理フロー俯瞰図の双方で新名に揃っていることを目視確認する。
