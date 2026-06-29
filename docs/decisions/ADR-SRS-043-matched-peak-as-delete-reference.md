# ADR-SRS-043: matched ピークを delete サミットの主ピーク参照先として許容する

| 状態 | 採用・未実装 |
| 決定日 | 2026-06-29 |

> **本 ADR は [ADR-SRS-042](ADR-SRS-042-matched-peak-excluded-from-dominant-candidate.md) を supersede する。**

## Context

[ADR-SRS-042](ADR-SRS-042-matched-peak-excluded-from-dominant-candidate.md) は「matched ピークを
delete サミットの主ピーク候補から除外し、matched ピークの delete 判定ゾーン内に AZ 外の
SOTA サミットが存在した場合は `unmatched`（要確認）として扱う」と決定した。

しかしこの方針は以下の**主成果物の欠落**をもたらす（シナリオD）:

- ピーク A の AZ 内に既存サミット X → `peak.match_status = matched`（X は正常存続）
- 同じ A の delete 判定ゾーン内（AZ 外）に既存サミット Y、Y はどのピークの AZ にも入らない
- ADR-042 の除外ルールにより Y が `unmatched`（要確認）に降格
- → `dominant_peak_code` / `dominant_peak_dist_m` が付与されず、**削除申請「削除」行に自動掲載されない**

このプロジェクトの主成果物は削除申請 XLSX であり、「どのピークに従属しているか」付きで
削除申請を自動生成することが存在意義の中核である。シナリオD での取りこぼしは
ADR-042 が viewer 上の接続線の置き場の曖昧さを避けるために削除申請データのリンクごと
捨てた**過剰補正**であった。

**事前データ（九州・四国解析）**: シナリオD（AZ1+delete_zone1 同一ピーク）= 1件発生。
「AZ=0 かつ delete_zone に複数サミット」= 0件。

## Decision

**matched ピークも delete サミットの主ピーク候補に含める。**

[FR-009](../20_SRS.md#fr-009-sotaリスト突合match_status-判定) の主ピーク特定ロジックから
`match_status=matched` のピーク除外ルールを撤回し、以下の方針に変更する:

- delete サミット Y の主ピーク候補として **matched ピーク A も対象とする**
- 主ピークが matched の場合、`dominant_peak_code` は当該 matched ピーク A の**既存 SOTA コード**
  （X のコード）を付与する
- Y は `summit.match_status = delete` として扱われ、**削除申請「削除」行に自動掲載**される
- ピーク A は `peak.match_status = matched` のまま（X の存続申請には影響しない）
- matched フィーチャ構成に「**従属 delete サミット Point + ピーク→サミット LineString（複数可）**」
  を追加定義する。X（AZ 存続サミット）と Y（従属 delete サミット）は別 Feature・
  別 `summit.match_status` で区別され、概念の混在（1フィールドへの複数概念詰め込み）は生じない
- `unmatched` は「全 AZ・全 delete 判定ゾーンのいずれにも含まれない真の孤立サミット」のみとする

## Alternatives

### 案②: matched ピークを dominant に昇格させて複合構成を正式定義（将来移行候補）

matched ピークを dominant に昇格し、matched/dominant 複合のフィーチャ構成を新設する方式。
X は「matched として存続」、Y は「dominant ピークに従属して削除」を1つのピークで表現する。

本軽量案（本 ADR）で「削除申請の自動生成」という主要件が充足できるため、
複合構成定義の追加コストを払う必要がない。全国解析実施後に発生件数（シナリオD 件数・
AZ0+delete複数件数）を確認し、案②への移行要否を改めて判断する。

### ADR-SRS-042 維持（却下）

主要件（削除申請の自動生成）を取りこぼすため採用しない。

## Consequences

- [FR-009 主ピーク特定](../20_SRS.md#fr-009-sotaリスト突合match_status-判定) の
  候補条件から `match_status=dominant` 絞り込みを削除し、matched ピークも候補とする
- `dominant_peak_code` がシナリオD では既存 SOTA コードになりうることを SRS に明記する
- matched フィーチャ構成（`merged_summit.geojson` スキーマ）に従属 delete サミット
  Point/LineString を追加する（matched 行の更新）
- `unmatched` の定義が「全ゾーン外の真の孤立」に戻り、シナリオD で誤って unmatched
  カウントされていたサミットが delete に正しく分類される
- 全国解析実施後、案②への移行要否を再検討する
- [ADR-SRS-042](ADR-SRS-042-matched-peak-excluded-from-dominant-candidate.md) は supersede
