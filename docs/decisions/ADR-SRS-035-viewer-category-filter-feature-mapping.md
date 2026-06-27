# ADR-SRS-035: HTML ビューアのカテゴリ別表示フィルタ4分類とフィーチャの対応定義

| 状態 | 採用・未実装 |
| 決定日 | 2026-06-26 |

## Context

HTML ビューア（[FR-019](../20_SRS.md#fr-019-html-ビューア機能仕様)）のカテゴリ別表示フィルタは
フィーチャを **new / dominant / changed / unchanged** の 4 カテゴリに分類するが（FR-019）、
この 4 カテゴリが `merged_summit.geojson` のデータモデル（[FR-009](../20_SRS.md#fr-009-sotaリスト突合match_status-判定)
が定義する `feature_type` / `match_status` / `is_band_change_candidate`）のどの値に対応するか
SRS 本文に定義がなかった（ISSUE-131）。

`new` / `dominant` は `peak.match_status` と直接対応が取れるが、`changed` / `unchanged` は
どのプロパティ値に対応するかが宙に浮いていた。さらに、検索確定時に「非表示カテゴリのフィルタを
自動 ON にする」挙動（FR-019）について、4 カテゴリに帰属が定義されていない
`feature_type="summit"` ∧ `match_status="delete"` の削除候補サミット・`feature_type="key_col"` の
コルをどのカテゴリ扱いにすべきかが不定だった（ISSUE-133）。

表示マーカーは FR-009 スキーマ上、次の 3 種:

- `peak`（`match_status` ∈ {matched, new, dominant}、matched のみ `is_band_change_candidate`）
- `summit`（既存 SOTA。`match_status` ∈ {matched, delete}）
- `key_col`（`match_status` を持たず、`summit_code` で親ピークに紐づく）

## Decision

カテゴリ別表示フィルタとフィーチャの対応を以下に確定する（制定時は new / dominant / changed / unchanged の 4 分類。[ADR-SRS-037](ADR-SRS-037-unmatched-summit-needs-review.md) により needs_review を追加し 5 分類へ拡張）。

| カテゴリ | 対象フィーチャ |
|---|---|
| **new** | `feature_type="peak"` ∧ `match_status="new"`（および対応する `key_col`） |
| **dominant** | `feature_type="peak"` ∧ `match_status="dominant"`（および対応する `key_col`）、ならびに当該ピークに従属する `feature_type="summit"` ∧ `match_status="delete"`（削除候補サミット） |
| **changed** | `feature_type="peak"` ∧ `match_status="matched"` ∧ `is_band_change_candidate=true`（および対応する `key_col`・対応する matched `summit`） |
| **unchanged** | `feature_type="peak"` ∧ `match_status="matched"` ∧ `is_band_change_candidate=false`（および対応する `key_col`）、ならびに `feature_type="summit"` ∧ `match_status="matched"`（バンド変更なしの既存サミット） |
| **needs_review** | `feature_type="summit"` ∧ `match_status="unmatched"`（どのピークにも従属しない孤立サミット。地形変化による消滅の可能性。[ADR-SRS-037](ADR-SRS-037-unmatched-summit-needs-review.md) で追加） |

帰属の原則（ISSUE-133 の解決）:

1. **削除候補サミット（`summit.match_status="delete"`）は dominant カテゴリに含める**。
   その削除は dominant ピークがサミットを従属させたこと（[FR-009](../20_SRS.md#fr-009-sotaリスト突合match_status-判定)
   の主ピーク特定）が主因であり、削除候補サミットと dominant ピークは同一山塊として
   1 グループで表示・フィルタするのが自然。
2. **key_col は独立カテゴリ／独立トグルを持たず、親ピーク（`summit_code` で対応）のカテゴリに追従する**。
   コルはピークのプロミネンス根拠を示す従属フィーチャであり、ピークと表示単位を揃える。
3. **要確認サミット（`summit.match_status="unmatched"`）は独立した needs_review カテゴリとする**（[ADR-SRS-037](ADR-SRS-037-unmatched-summit-needs-review.md)）。
   delete サミットを dominant に同梱した原則 1 とは異なり、unmatched は従属する dominant ピークを
   持たない孤立フィーチャのため、いずれの既存カテゴリにも帰属できず独立カテゴリが必然となる。

検索確定時の自動フィルタ ON（FR-019）は、確定対象フィーチャが属する上記カテゴリを ON にする
（key_col 確定時は親ピークのカテゴリ、delete サミット確定時は dominant カテゴリ、unmatched サミット確定時は needs_review カテゴリ）。

## Alternatives

- **削除候補サミットを独立したカテゴリにする**: delete を新カテゴリとして分離する案。
  分類の対称性は増すが、dominant ピークと削除候補サミットは申請上一体（dominant ピーク追加と
  サミット削除がセット）で、別カテゴリにすると関連フィーチャがフィルタで分断され目視確認しづらい。
  delete は dominant に同梱するため不採用。（なお、従属ピークを持たない孤立サミット `unmatched` は
  同梱先がなく、独立した needs_review カテゴリとした。[ADR-SRS-037](ADR-SRS-037-unmatched-summit-needs-review.md) 参照）
- **key_col を独立トグルレイヤーにする**: コルを別レイヤーで ON/OFF する案。コルだけを単独で
  表示制御する運用上の要求がなく、親ピークと表示が分離するとプロミネンスの対応確認が
  かえって煩雑になるため不採用。親ピーク追従とする。

## Consequences

- FR-019（1124 カテゴリ別表示フィルタ・1135 検索自動フィルタ ON）に本対応表を反映する（本コミットで実施）。
- 配色は引き続き new=緑系 / dominant=赤系 / changed=橙系 / unchanged=灰系（具体値は HLD）。
  削除候補サミットは dominant カテゴリの配色に従う。needs_review の配色も HLD で規定する。
- ISSUE-131・ISSUE-133 を本 ADR で決着する。needs_review カテゴリの追加は [ADR-SRS-037](ADR-SRS-037-unmatched-summit-needs-review.md)（ISSUE-135）による。
- 実装（フィルタ判定ロジック）への反映は別タスク（HLD・実装フェーズ）で行う。
