# ADR-URD-007: match_status 用語整理（peak / summit 独立定義）

| 状態 | 採用・未実装 |
| 決定日 | 2026-05-14 |
| 最終更新日 | 2026-05-21 |

## Context

`match_status` の値が主語ごとに異なる意味を持ちながら同一フィールドで混在しており、一貫していなかった:

- **peak 側**: `matched` / `new` … ピーク自身の状態（ピーク中心の記述）
- **peak 側**: `deleted` … ピーク自身ではなく**近傍 SOTA サミットへのアクション**を記述（誤解を招く）

また peak feature と sota_summit feature の両方が `match_status` を持ち、 feature type 名 `sota_summit` も SOTA 限定の概念に対して冗長だった。

## Decision

**(1) peak / LineString / CSV 側の `deleted` → `dominant` リネーム（2026-05-14 決定済み）**

| 対象 | 変更前 | 変更後 |
|---|---|---|
| peak feature の `match_status` | matched / new / deleted | matched / new / dominant |
| LineString (coord_diff) の `match_status` | matched / deleted | matched / dominant |
| merged.csv の `match_status` カラム値 | matched / new / deleted | matched / new / dominant |

`dominant` の定義: 検出ピークの delete判定ゾーン内に既存 SOTA サミット座標が存在するが、アクティベーションゾーン外（= そのサミットが削除候補となり、このピークがその dominant peak になる）。

（注: 2026-05-21 [ADR-SRS-011](ADR-SRS-011-delete-zone-polygon.md) 適用により、`dominant` 判定の根拠ポリゴンは「コル等高線ポリゴン」から「delete判定ゾーンポリゴン」に変更された。判定構造（matched / new / dominant）は維持。）

**(2) summit feature のリネームと summit.match_status の整理（2026-05-16 追加決定）**

| 対象 | 変更前 | 変更後 |
|---|---|---|
| GeoJSON feature type | `sota_summit` | `summit` |
| summit feature の `match_status` | matched / deleted | matched / **delete** |

`delete`（命令形）を採用した理由: 申請書アクション列の「削除」と意味的に対応し、かつ「削除済み」を意味する過去形 `deleted` との混同を避けるため。

**peak.match_status と summit.match_status は独立した2属性**（それぞれ別フィーチャが保持する）:

| 値 | peak.match_status | summit.match_status |
|---|---|---|
| matched | AZ 内に既存サミット座標あり | 対応検出ピークの AZ 内に存在（正常存続） |
| new | AZ・delete判定ゾーン内に既存サミットなし（新規候補）| — |
| dominant | delete判定ゾーン内かつ AZ 外に既存サミット座標あり | — |
| delete | — | delete判定ゾーン内かつ AZ 外に存在（削除候補） |
| unmatched | — | AZ にも delete判定ゾーンにも該当しない（エラー、処理中止。[ADR-SRS-011](ADR-SRS-011-delete-zone-polygon.md) 参照） |

**申請書生成への影響**:
- `追加` 行: peak.match_status ∈ {new, dominant} のピーク（仮サミットコードを使用）
- `削除` 行: summit.match_status="delete" のサミット（SummitCode を使用）
- `変更` 行: ユーザーが HTML ビューアで名称修正した matched ピーク

## Alternatives

**sota_summit feature 名の維持**  
プロジェクト内で "summit" は SOTA 限定で使用されているため、`sota_summit` は冗長。`summit` のみで意味が通じると判断し採用しない。

**deleted の維持（summit 側）**  
「削除候補」の意味で `deleted` を使い続けると、申請書の「削除」アクションとの対応関係は自然だが、「削除済み」という過去形が状態の正確な記述として不適切。命令形 `delete` がより正確。

**peak_status / summit_status への完全分離（フィールド名変更）**  
フィールド名自体を分けることで主語の混乱は完全に解消できる。ただし GeoJSON 仕様変更コストが大きいため不採用。`match_status` を feature ごとに独立した値域で定義することで対応する。

## Consequences

- 各フィーチャの `match_status` がそのフィーチャ自身の状態を表し、一貫性が向上する
- Python 実装（merge.py）で `deleted` → `dominant`（peak 側）および `deleted` → `delete`（summit 側）の出力値変更が必要
- GeoJSON の feature type `sota_summit` → `summit` への変更が必要
- HTML ビューアのフィルター UI は「削除」というユーザー向けラベルを維持しつつ、内部の `data-cat` 値を `dominant`（peak）/ `delete`（summit）で管理する
- dominant ピークにも仮サミットコードを採番するため、申請書「追加」行が dominant ピーク分だけ増える
