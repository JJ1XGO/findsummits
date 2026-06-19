# ADR-SRS-033: 不備確認責務を geojson metadata から xlsx へ移行

| 状態 | 採用・未実装 |
| 決定日 | 2026-06-19 |

## Context

FR-013 レビュー（ISSUE-118）で、現状設計の3つの問題が明らかになった。

### 問題 1: geojson metadata の boolean 不備フラグに消費者がいない

[ADR-SRS-011](ADR-SRS-011-delete-zone-polygon.md)・[ADR-SRS-013](ADR-SRS-013-merged-geojson-as-central-data.md) は不備フラグ（`is_unmatched_summit`/`is_area_incomplete`/`is_key_col_unresolved`）を `merged_summit.geojson` の top-level `metadata` に格納すると規定した。しかし実際の消費者がいない状態になっている:

- **FR-013 のスキップ判定**: FR-009 の exit code を使う（SRS L914）。boolean フラグは参照しない
- **地図上の可視化（警告色）**: per-feature プロパティ（`key_col_resolved`・`area_complete`）が担う。boolean サマリーは地図描画に寄与しない

結果として、top-level boolean フラグは「成功時は全 false（無意味）、失敗時は geojson の調査用」として格納されているが、失敗時の調査は **geojson のフィーチャ属性（per-feature プロパティ）** を見れば行えるため、boolean サマリーは vestigial（遺物）化している。

### 問題 2: 不備ゲート発動時に xlsx が出力される保証がない

SRS L914 は「`merged_summit.geojson` は不備フィーチャも含めて必ず出力してから停止する（調査用）」と明記している。しかし `merged_summit.xlsx` については何も記述がない。

一方で `merged_summit.xlsx`（サミット一覧（突合後））こそが、ユーザーが表形式で不備を確認する主役である（per-row で `key_col_resolved`・`stability`・`match_status` が見える）。不備調査のために止まったにもかかわらず、調査に使う xlsx が出力されない可能性は本末転倒。

### 問題 3: xlsx が不備を表形式で完全に表現できていない

- `area_complete` 列が xlsx に存在しない（`is_area_incomplete` に相当する per-row 情報が欠落）
- `unmatched` サミットが xlsx に行として出力されない（`match_status` 値域が matched/new/dominant のみ）

## Decision

### 1. geojson metadata から top-level boolean 不備フラグを削除

`merged_summit.geojson` の `metadata` から以下を削除する:
- `is_unmatched_summit`
- `is_area_incomplete`
- `is_key_col_unresolved`

per-feature プロパティ（`key_col_resolved`・`area_complete`）は維持する（地図警告色・ビューア UI が参照）。

### 2. 不備確認の責務を xlsx に集約

`merged_summit.xlsx` を「不備を含む全サミットの完全な確認表」として完成させる:

**列の追加**:
- `area_complete`（bool）: activation zone が解析範囲内で完結しているか。FR-016 出力の `area_complete` を per-row で格納

**match_status 値域の拡張**:
- `delete`: 既存サミットが削除候補（`summit.match_status="delete"`）の行を含める
- `unmatched`: 既存サミットがどのゾーンにも含まれない（`summit.match_status="unmatched"`）の行を含める

これにより xlsx が `merged_peak.csv`（ピーク中心）と `summitslist.csv`（サミット中心）の全エントリを網羅する確認表になる。

### 3. 不備ゲート発動時の出力保証

FR-009 の異常終了制御（SRS L914）を以下のように書き換える:

**変更前**:
> `merged_summit.geojson` は不備フィーチャも含めて必ず出力してから停止する（調査用）

**変更後**:
> `merged_summit.geojson` および `merged_summit.xlsx` をともに不備エントリを含めて必ず出力してから停止する（調査用）。この「不備ゲート」（データ品質による意図的な停止）とは区別して、入力欠落・例外によるハードクラッシュ時は出力を保証しない

## Alternatives

### 案 A: geojson metadata の boolean フラグを維持し xlsx にも追加（不採用）

両方に持つ案。geojson の boolean フラグに消費者がいない状況は解消されず、冗長な状態が続く。「真実の情報源は1箇所」の原則に反する。→ **却下**

### 案 B: geojson metadata の boolean フラグのみで管理（現状維持・不採用）

geojson のみに持ち、xlsx には追加しない案。問題 2・3 が解消されないため却下。ユーザーが不備を表形式で確認する手段がない。→ **却下**

### 案 C: ログ + exit code のみ（不採用）

不備を機械可読な形で成果物に持たず、ログと exit code だけで表現する案。失敗後に調査するユーザーがログを読む必要が生じ、UX が悪い。また案①不採用と違い、案 C は per-feature プロパティ（地図可視化用）の削除まで含意するため設計後退が大きい。→ **却下**

## Consequences

- **SRS FR-009**: 不備フラグの定義箇所（L910-915）を改訂。①metadata の boolean フラグ削除、②異常終了制御に xlsx セット出力を追記、③ハードクラッシュとの区別を明記
- **SRS FR-012（xlsx カラム定義）**: `area_complete` 列を追加、`match_status` 値域に `delete`/`unmatched` を追記
- **SRS 6.5（サミット一覧（突合後）仕様）**: 同上の変更を反映
- **SRS FR-013（metadata 一覧）**: boolean 不備フラグ群の記述を削除（per-feature プロパティは残す）
- **ADR-SRS-011 Consequences**: 「不備フラグは `merged_summit.geojson` のメタデータプロパティに格納」を本 ADR 決定に更新
- **ADR-SRS-013**: 「不備フラグを metadata に」という旧決定を本 ADR 決定で上書きと追記
