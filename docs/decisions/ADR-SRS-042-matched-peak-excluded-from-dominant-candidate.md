# ADR-SRS-042: matched ピークを削除候補サミットの主ピーク候補から除外する

| 状態 | 却下・[ADR-SRS-043](ADR-SRS-043-matched-peak-as-delete-reference.md) により supersede |
| 決定日 | 2026-06-28 |

> **本 ADR は [ADR-SRS-043](ADR-SRS-043-matched-peak-as-delete-reference.md) により supersede された（2026-06-29）。**
> シナリオD（AZ1+delete_zone1 同一ピーク）で削除申請データを取りこぼす過剰補正であったため。
> 現行仕様は ADR-SRS-043 を参照すること。

## Context

[FR-016](../20_SRS.md#fr-016-ピーク域ポリゴン生成) は match_status を問わず全ピークに delete判定ゾーンを生成する。
[FR-009](../20_SRS.md#fr-009-sotaリスト突合match_status-判定) の主ピーク特定ロジックは全ピークの
delete判定ゾーンを対象にするため、以下のシナリオが仕様上排除されていなかった。

**問題シナリオ**:

- ピークAのAZ内にSOTAサミットX → ピークA.match_status = matched（確定）
- ピークAのdelete_zone内（AZ外）にSOTAサミットYが存在し、Yがどのピークの AZ にも含まれない
  → Y.match_status = delete
  → 主ピーク候補に matched のピークA が選ばれてしまう

この場合、matched のフィーチャ構成（[FR-009 参照](../20_SRS.md#fr-009-sotaリスト突合match_status-判定)）には
delete サミットの Point/LineString が含まれておらず、フィーチャ構成・ビューア表示が未定義になる。

**発生条件**: SOTAサミットYに対応するピークのプロミネンスが DEM 精度の誤差で
[FR-007](../20_SRS.md#fr-007-per-mesh-csv-出力プロミネンス閾値適用) のフィルタ閾値（150m）未満と
計算された場合。発生頻度は低いが排除できない。

## Decision

**matched ピークは delete サミットの主ピーク候補から除外する。**

[FR-009](../20_SRS.md#fr-009-sotaリスト突合match_status-判定) の主ピーク特定ロジックの候補条件を
「`match_status=dominant` のピークのみ」に絞り込む。

matched ピークの delete判定ゾーン内に AZ 外の SOTA サミットが存在した場合は、
そのサミットを `unmatched`（要確認）として扱う。担当者が目視で削除・存続を判断する
（`unmatched_review_threshold` の計上対象となる）。

## Alternatives

### 案②: matched ピークを dominant に昇格（却下・将来移行候補）

matched のdelete_zone内にAZ外の delete候補サミットYが入った場合、ピークの match_status を
dominant に昇格させ、AZ内のSOTAサミットX は「matched として存続」として扱う方式。

この方式では matched/dominant のフィーチャ構成が複合するケースの仕様整理が必要であり、
全国解析の実データなしでは「どの程度発生するか」の見通しが立たない。

全国解析実施後に発生件数を確認し、案②への移行要否を改めて判断する。

## Consequences

- [FR-009 主ピーク特定](../20_SRS.md#fr-009-sotaリスト突合match_status-判定) の候補条件に
  `match_status=dominant` の絞り込みを追加する（line 905-910 参照）
- matched ピークの delete_zone 内に AZ 外サミットが存在するケースは `unmatched`（要確認）として
  `unmatched_review_threshold` の計上対象となる
- 全国解析実施後、案②への移行要否を再検討する
