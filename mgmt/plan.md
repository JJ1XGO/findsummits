# 計画: ISSUE-151〜154 SRS反映

## Context

spec-panelレビュー（FR-009 matched行へのdelete_zone追記・FR-019 is_island削除）で発見した
4件の仕様未定義箇所について、設計決定が出揃ったため SRS および ADR に反映する。

決定内容:

- ISSUE-151: matchedのdelete_zoneをビューアで表示する
- ISSUE-152: 案A — 海面確定規則でもcol_lat/col_lon=0.0 sentinelに統一
- ISSUE-153: 表示文言「未定義（陸地最高峰）」を統一ラベルとして維持（理由を明記）
- ISSUE-154: 案① — matchedピークをdeleteサミットの主ピーク候補から除外（将来的に案②へ移行可能性あり）

## 変更ファイル一覧

| ファイル | 変更内容 | 対応ISSUE |
|---|---|---|
| `docs/20_SRS.md` | FR-019 line 1152 / FR-006 line 651 / per-mesh CSVカラム定義 line 689 / FR-018 line 815 / FR-019 line 1185 / FR-009 line 905-910 | 151/152/153/154 |
| `docs/decisions/ADR-SRS-019-land-summit-highest-peak-handling.md` | 層1の col_lat/col_lon=0.0 設定を Decision に追記 | 152 |
| `docs/decisions/ADR-SRS-042-matched-peak-excluded-from-dominant-candidate.md` | 新規作成（ISSUE-154 案①の根拠） | 154 |

## タスク一覧（Sonnet・実行順）

### 1. ISSUE-151: FR-019 line 1152 — delete_zone レイヤー表示対象の拡張

**対象**: `docs/20_SRS.md` line 1152

変更前:

```text
delete判定ゾーンポリゴン（new / dominant）を独立したトグルレイヤーとして追加（デフォルト ON・半透明）。
new は delete判定ゾーン内に既存サミットが存在しないことを、
dominant は delete判定ゾーン内に削除候補サミットが存在することを可視化する
```

変更後（「new / dominant」を削除し、matchedの意味を追記）:

```text
delete判定ゾーンポリゴンを独立したトグルレイヤーとして追加（デフォルト ON・半透明）。
new は delete判定ゾーン内に既存サミットが存在しないことを、
dominant は delete判定ゾーン内に削除候補サミットが存在することを可視化する。
matched は delete判定ゾーンが生成されるが（FR-016 は match_status を問わず全ピークに生成）、
主ピーク候補から除外されるため（ADR-SRS-042 参照）delete判定ゾーン内に削除候補サミットは存在しない
```

### 2. ISSUE-152: FR-006 line 651 — 海面確定規則にcol sentinel追記

**対象**: `docs/20_SRS.md` FR-006 line 651

末尾「島の最高峰はこの規則で 3×3 または広域解析内で自動確定する」の直後に追記:

```text
この場合も col_lat/col_lon は 0.0 に設定する
（key_col_resolved=true + col_lat/col_lon=0.0 の組み合わせが「海面を Key コルとして確定」の sentinel となる。
陸地最高峰リスト（層2・FR-008）と同一表現に統一する）
```

### 3. ISSUE-152: per-mesh CSV カラム定義 line 689 — col_lat/col_lon 説明の拡張

**対象**: `docs/20_SRS.md` line 689付近の col_lat・col_lon カラム行

変更前:

```text
| col_lat | float | 小数点8桁 | コル緯度（`key_col_resolved=false` の場合は 0.0） |
```

変更後:

```text
| col_lat | float | 小数点8桁 | コル緯度（`key_col_resolved=false` の場合、または海面確定（`key_col_resolved=true` + Key コル=0m）の場合は 0.0） |
```

col_lon カラムも同様に修正。

### 4. ISSUE-152: FR-018 line 815 — 島嶼部最高峰を明記

**対象**: `docs/20_SRS.md` line 815

変更前:

```text
陸地最高峰は `col_lat`/`col_lon`=0.0 sentinel のため除外
```

変更後:

```text
陸地最高峰・島嶼部最高峰（FR-006 海面確定規則による自動確定を含む）は
`col_lat`/`col_lon`=0.0 sentinel のため除外
```

### 5. ISSUE-152: ADR-SRS-019 — 層1のcol座標値を Decision に追記

**対象**: `docs/decisions/ADR-SRS-019-land-summit-highest-peak-handling.md`

Decision セクション「層1: 海面確定規則」の説明末尾に追記:

```text
col_lat/col_lon は 0.0 に設定する（層2の陸地最高峰リストと同一 sentinel 表現に統一）。
これにより FR-018 の key_col Point 除外ロジックが「col_lat/col_lon=0.0 かどうか」で統一できる。
```

### 6. ISSUE-153: FR-019 line 1185 — 統一ラベルの根拠を明記

**対象**: `docs/20_SRS.md` line 1185

末尾「陸地最高峰と島嶼部最高峰はビューア上で区別しない（`is_island` プロパティは使用しない）」を以下に置き換え:

```text
陸地最高峰と島嶼部最高峰はビューア上で区別しない（`is_island` プロパティは使用しない）。
どちらも col_lat/col_lon=0.0 sentinel で統一されておりビューアが区別できる内部属性を持たないため、
「未定義（陸地最高峰）」を統一ラベルとして使用する。
```

### 7. ISSUE-154: ADR-SRS-042 新規作成

**ファイル**: `docs/decisions/ADR-SRS-042-matched-peak-excluded-from-dominant-candidate.md`

フォーマット:

```markdown
| 状態 | 採用・未実装 |
| 決定日 | 2026-06-28 |

## Context
...
## Decision
matchedピークはdeleteサミットの主ピーク候補から除外する。
主ピーク特定ロジックの候補条件を「match_status=dominant のピーク」に絞る。
matchedのdelete_zone内にいるAZ外SOTAサミットは unmatched（要確認）として扱う。
## Alternatives
案②（matchedをdominantに昇格）は全国解析実データ確認後に再検討。
## Consequences
FR-009 line 905-910 に制約を追加。
```

### 8. ISSUE-154: FR-009 line 905-910 — 主ピーク特定に制約を追記

**対象**: `docs/20_SRS.md` line 906 の「候補とする」の直後に追記:

```text
ただし match_status=matched のピークは候補から除外する（[ADR-SRS-042] 参照）。
matchedピークのdelete_zone内にAZ外のSOTAサミットが存在した場合は unmatched（要確認）として扱う。
```

## 実施後の検証

1. `make lint` 警告ゼロ
2. ADR-SRS-042 へのリンク（`[ADR-SRS-042](...)`）が SRS 本文中で正しいこと
3. ISSUE-151〜154 を tracker で `close` する

## コミット方針

全 8 タスクを 1 コミットにまとめる。
