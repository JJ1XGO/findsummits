# ADR-007: peak の match_status 用語整理

| 状態 | 採用・未実装 |
| 決定日 | 2026-05-14 |

## Context

`match_status` の3値は主語が一貫していなかった:

- `matched` / `new` … ピーク自身の状態（ピーク中心の記述）
- `deleted` … ピーク自身ではなく **近傍 SOTA サミットへのアクション** を記述

特に GeoJSON の peak feature において `match_status="deleted"` は「このピーク自身が削除される」と誤読される。実際の意味は「このピークは dominant peak であり、近傍の SOTA サミットが削除候補となる」である。

SOTA summit feature は **サミット自身の運命** を直接表すため `matched` / `deleted` はそのまま維持する。

## Decision

peak feature / LineString (coord_diff) / merged.csv 行における `deleted` を `dominant` にリネームする。

| 対象 | 変更前 | 変更後 |
|---|---|---|
| peak feature の `match_status` | matched / new / deleted | matched / new / dominant |
| LineString (coord_diff) の `match_status` | matched / deleted | matched / dominant |
| merged.csv の `match_status` カラム値 | matched / new / deleted | matched / new / dominant |
| **sota_summit feature の `match_status`** | **matched / deleted** | **変更しない** |

`dominant` の定義: 検出ピークのコル等高線内に既存 SOTA サミット座標が存在するが、アクティベーションゾーン外（= そのサミットが削除候補となり、このピークがその dominant peak になる）。

申請書 XLSX 生成時は peak の `match_status="dominant"` → 削除アクションにマッピングする（SOTA summit feature の `match_status="deleted"` を使って削除行を生成する）。

## Alternatives

**現状維持（注釈で説明）**  
実装コストはゼロだが、GeoJSON を外部ツールで参照した場合や将来の実装者が誤読するリスクが継続する。

**peak_status / summit_status への完全分離**  
2つのプロパティにすることで主語の混乱は完全に解消できる。ただし GeoJSON の仕様変更と実装コストが大きく、`match_status` が1フィールドで完結する現行設計のシンプルさを失う。今回は3値それぞれの定義を統一することで混乱を解消できると判断し不採用。

## Consequences

- 用語の一貫性が向上し、GeoJSON の各フィーチャの `match_status` がそのフィーチャ自身の状態を表す
- Python 実装（merge.py）で `deleted` → `dominant` の出力値変更が必要
- HTML ビューアのフィルター UI は「削除」というユーザー向けラベルを維持しつつ、内部の `data-cat` 値を `dominant` に変更する
