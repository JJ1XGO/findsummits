# ADR-SRS-044: 申請カテゴリプロパティ化とサミット中心5分類への再編

| 状態 | 採用・未実装 |
| 決定日 | 2026-06-29 |

> **本 ADR は [ADR-SRS-035](ADR-SRS-035-viewer-category-filter-feature-mapping.md) を supersede する。**

## Context

### 問題

[FR-009](../20_SRS.md#fr-009-sotaリスト突合match_status-判定) の「フィーチャ構成（match_status 別）」表は、
`merged_summit.geojson` のジオメトリ在庫表として `peak.match_status` 軸（matched/new/dominant/unmatched）
で書かれていた。これは幾何がピーク周りに群がる（1ピーク＝AZ＋コル＋ゾーン＋複数の線）という
構造的理由からであるが、以下の問題を生んでいた:

1. **軸の混在**: peak 値（new/dominant）と summit 値（unmatched）を黙って1表に混在させており、
   表の見出し「match_status 別」だけでは主語が不明。
2. **消費側との乖離**: 消費側（FR-011 申請書 XLSX・FR-019/020 ビューア・FR-021 ZIP）は既に
   サミットアクション軸で再グルーピングしているが、生成者 FR-009 のスキーマは
   peak.match_status 軸のままで軸が3つ（FR-009/FR-011/FR-019）存在していた。
3. **実行時導出の分散**: カテゴリ分類（[ADR-SRS-035](ADR-SRS-035-viewer-category-filter-feature-mapping.md)）は
   消費側（FR-019/020 ビューア・FR-021 ZIP）が `feature_type`/`match_status`/`is_band_change_candidate`
   から実行時に導出していた。生成者 FR-009 がスキーマ正本であるにもかかわらず、
   分類ロジックが消費側に分散していた。
4. **XLSX の同型 muddiness**: `merged_summit.xlsx`（6.2.7）・`merged_summit_revised.xlsx`（6.2.3）の
   `match_status` 列も同じ問題を抱えていた（ピーク行: matched/new/dominant、
   既存サミット行: matched/delete/unmatched）。

### FR-009 の位置づけ

FR-009 はデータ概念が「ピーク中心→サミット中心」へ切り替わる節目として `20_SRS.md` に明記されている。
ここで中心データを**サミット中心の申請カテゴリ**で表現するのが設計の一貫性に即している。

## Decision

### 申請カテゴリ（5分類）の定義

FR-009 が各フィーチャに `category` プロパティを算出・付与する。
`match_status`/`feature_type`/`is_band_change_candidate` は廃止せず存続し、
`category` はそれらから算出する直交プロパティとする。

| `category`（格納値） | 表示ラベル | 主語 | 由来条件 |
|---|---|---|---|
| `add` | 追加 | 新設サミット | `peak.match_status` ∈ {new, dominant} |
| `band_change` | 変更あり | 既存サミット | `peak.match_status="matched"` ∧ `is_band_change_candidate=true` |
| `no_change` | 変更なし | 既存サミット | `peak.match_status="matched"` ∧ `is_band_change_candidate=false` |
| `delete` | 削除 | 既存サミット | `summit.match_status="delete"` |
| `review` | 要確認 | 既存サミット | `summit.match_status="unmatched"` |

- `add` は new/dominant を統合する。dominant ピークの「削除を伴う置き換え」という副作用は、
  紐づく `delete` サミット側で表現する（XLSX アクション列でも両方「追加」であるため一致）。

### per-feature の `category` 割り当てルール

全フィーチャに `category` を付与する:

| フィーチャ種別 | `category` の値 |
|---|---|
| peak（match_status=new または dominant） | `add` |
| peak（match_status=matched ∧ is_band_change_candidate=true） | `band_change` |
| peak（match_status=matched ∧ is_band_change_candidate=false） | `no_change` |
| key_col・activation_zone・delete_zone・LineString（peak→col） | 親ピークの `category` を継承 |
| matched summit（AZ 内存続）＋ LineString（peak→matched summit） | 親ピークの `category` を継承（`band_change` または `no_change`） |
| delete summit ＋ LineString（親ピーク→delete summit） | `delete`（親ピークが add/band_change/no_change いずれでも [ADR-SRS-043](ADR-SRS-043-matched-peak-as-delete-reference.md) 参照） |
| unmatched summit | `review` |

帰結: dominant/matched ピーク本体は `add`/`band_change`/`no_change`、その従属 delete サミットは
`delete` となり、山塊クラスタが2カテゴリに跨る。これは人間可読性（各フィーチャが自身のアクションを
直接示す）を優先した選択であり、ユーザーと確認のうえ許容する。

### 成果物全体への展開

| 成果物 | 旧方式 | 新方式 |
|---|---|---|
| `merged_summit.geojson`（FR-009） | フィーチャ構成表が peak.match_status 軸 | 全フィーチャに `category` プロパティ付与。フィーチャ構成表を申請カテゴリ別に再編 |
| `merged_summit.xlsx` / `merged_summit_revised.xlsx`（FR-012） | `match_status` 列のみ（値域が行種別で異なり直読困難） | `category` 列を追加（`match_status` 列は残す） |
| ビューアカテゴリフィルター（FR-019/020） | 消費側が実行時に `feature_type`/`match_status`/`is_band_change_candidate` から導出（ADR-SRS-035） | 格納済み `category` プロパティを読む（導出ロジックを廃止） |
| 申請エビデンス ZIP 分割（FR-021） | new/dominant/changed/unchanged.geojson（4ファイル、delete サミットは dominant に同梱） | add/changed/unchanged/delete.geojson（4ファイル、削除独立）＋ review.geojson（要確認、同梱） |

### FR-021 ZIP への `review.geojson` 同梱

`unchanged.geojson` は「申請対象外だが参照用に同梱」している。同様の理由で
`review.geojson`（unmatched サミット）も同梱し、担当者が ZIP 単体で全件を確認できるようにする。

## Alternatives

### 案①: 表の見出しのみ修正（データ構造は現状維持）

フィーチャ構成表の見出しを「peak.match_status 軸（unmatched のみ summit 起点）」と正直に明記し、
消費側の実行時導出は継続する。工数は最小だが、生成者スキーマ正本の建前と実態が乖離したままになり、
消費側の整合崩れリスクが残る。

### 案②: GeoJSON に category を持たせず XLSX のみ追加

XLSX は直読が多いため category 列を追加するが、GeoJSON は現状維持。
導出ロジックが XLSX 生成時（FR-012）と GeoJSON 消費側（FR-019）の2箇所に分散して保守コストが増す。

## Consequences

- `category` プロパティは FR-009 の算出・付与から始まり、FR-011/012/013/019/020/021 および
  `00_GLOSSARY.md` への追従が必要。
- 旧4分類（new/dominant/changed/unchanged）の記述が全成果物から消滅し、
  新5分類（add/band_change/no_change/delete/review）に統一される。
- `ADR-SRS-035` は本 ADR により廃止（superseded）。
