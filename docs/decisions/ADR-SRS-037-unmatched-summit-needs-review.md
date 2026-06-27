# ADR-SRS-037: 地形変化で消滅したサミット（unmatched）の要確認扱い・即停止の見直し

| 状態 | 採用・未実装 |
| 決定日 | 2026-06-27 |

## Context

[FR-009（SOTAリスト突合）](../20_SRS.md#fr-009-sotaリスト突合match_status-判定) は、既存 SOTA サミット座標を各ピークのアクティベーションゾーン（AZ）・delete判定ゾーンへの point-in-polygon で判定する（[ADR-SRS-011](ADR-SRS-011-delete-zone-polygon.md)・[ADR-URD-007](ADR-URD-007-peak-match-status-terminology.md)）。

- AZ 内 → `matched`（存続）
- delete判定ゾーン内かつ AZ 外 → `delete`（削除候補）
- いずれにも入らない → `unmatched`

現行仕様は `unmatched` を「`delete_zone_max_drop` 値の不備または解析欠落を示す、**本来発生しないべき状態**」と定義し、1 件でも発生すれば不備ゲートで**処理停止**する。

しかし、噴火・山体崩壊・カルデラ陥没で**山が実際に消失・大幅低下した**場合、旧 SOTA サミット座標はどのピークのゾーンにも入らず `unmatched` になりうる。これはシステム不備ではなく**正当な地形変化＝削除すべきサミット**であり、現行の「即停止」設計はこのケースを想定していない盲点である。

問題の核心は、**座標だけでは「地形変化で消滅した（削除候補）」のか「閾値不備・解析バグ（システム不備）」なのかを機械的に区別できない**点にある。両者は「どのゾーンにも入らない」という同一症状を示す。

- そのまま申請書「削除」行へ自動掲載すると、システム不備による偽陽性をそのまま誤削除してしまう
- 一方で現行の「即停止」では、正当な地形変化のたびにパイプラインが止まり運用を妨げる

本プロジェクトの原則（成果物はドラフトであり最終判断は SOTA 日本支部担当者が行う／ブロックより警告して続行を基本とする）に照らし、`unmatched` を**担当者が判断する「要確認」状態**として扱う方針に改める。

## Decision

### 1. `unmatched` を「要確認」状態として続行扱いにする

`summit.match_status="unmatched"` の意味を「即停止すべき不備」から「**担当者の確認を要する孤立サミット**（地形変化による消滅の可能性）」へ転換する。値名 `unmatched` は維持し、意味づけのみ変更する。

- `unmatched` 単独では**処理を停止しない**（不備ゲートの無条件停止条件から外す）
- 申請書（[FR-011](../20_SRS.md#fr-011-申請書-xlsx-生成)）の「削除」行には**自動掲載しない**。`unmatched` は `delete` とは別物として扱い、担当者が地形変化を確認したうえで手動で削除申請に回す
- 確認手段として、`merged_summit.xlsx`（[ADR-SRS-033](ADR-SRS-033-defect-confirmation-via-xlsx.md) で既に `unmatched` 行を出力対象に含む）と HTML ビューア（[FR-019](../20_SRS.md#fr-019-html-ビューア機能仕様)）の「要確認」カテゴリ（本 ADR §3）で担当者に提示する

### 2. 件数しきい値による解析バグ防御

`unmatched` を続行扱いにすると、本物の解析バグ（ポリゴン生成漏れ等）でサミットが大量に紐付かなくなったケースも止まらず流れる懸念がある。噴火・崩壊による消滅は全国でも極めて稀（数年に数件レベル）である一方、解析バグは多数のサミットを同時に `unmatched` 化しうる。この件数差を切り分けに用いる。

- `unmatched` 件数が**しきい値以下**なら要確認として続行
- しきい値を**超過**した場合は「解析異常の疑い」として不備ゲートで停止する

しきい値は `params/config.ini` の設定項目とする。

- **パラメータ名**: `unmatched_review_threshold`（データ辞書表記「要確認サミット件数しきい値」）
- **既定値（暫定）**: `10`。全国一括突合では正常時 `unmatched` は理論上ゼロ（`delete_zone_max_drop=250m` が JA 全サミットの SOTA 登録標高 − DEM 標高の実測最大差 52.26m を大きくカバー、[ADR-SRS-011](ADR-SRS-011-delete-zone-polygon.md)）であり、正当な地形変化は数件以内に収まる想定から保守的に設定した。**実測根拠を持たない暫定値**であり、初回全国解析の実データで見直す（§未確定事項）

### 3. HTML ビューアに「要確認（needs_review）」カテゴリを追加

[ADR-SRS-035](ADR-SRS-035-viewer-category-filter-feature-mapping.md) が定義するカテゴリ別表示フィルタ（new / dominant / changed / unchanged の 4 分類）に、第 5 カテゴリ **needs_review（要確認）** を追加する。

| カテゴリ | 対象フィーチャ |
|---|---|
| **needs_review** | `feature_type="summit"` ∧ `match_status="unmatched"`（どのピークにも従属しない孤立サミット） |

`unmatched` サミットは従属する dominant ピークを持たない孤立フィーチャのため、[ADR-SRS-035](ADR-SRS-035-viewer-category-filter-feature-mapping.md) が `delete` を dominant に同梱した論理（削除候補と dominant ピークは同一山塊で一体）は適用できない。独立カテゴリとするのが必然であり、4 分類を 5 分類へ拡張する。配色は HLD で規定する。

### 4. 従来どおり停止を維持する不備

`unmatched` の扱い変更は、他の 2 つの不備ゲート条件には影響しない。以下は引き続き 1 件でも発生すれば停止する（解析の前提が崩れており、続行しても成果物が信頼できないため）。

- `is_area_incomplete`（AZ／delete判定ゾーンポリゴンの `area_complete=false`）
- `is_key_col_unresolved`（N=6 まで使い切っても Key コル未確定）

## Alternatives

### 案 A: `unmatched` を削除候補（`delete`）として申請書に自動掲載（不採用）

ユーザーの当初発想どおり「どのピークにも紐付かない＝削除候補」とみなし、申請書「削除」行へ自動で載せる案。座標だけでは地形変化と解析不備を区別できないため、システム不備による偽陽性をそのまま誤削除する重大リスクがある。SOTA の削除申請は実在の登録サミットを消す不可逆操作であり、自動化は危険。担当者の確認を挟む方針を採る。

### 案 B: 現状維持（`unmatched` 1 件で即停止）（不採用）

正当な地形変化（噴火・崩壊）でもパイプラインが毎回停止し、運用を妨げる。`unmatched` を「本来発生しないべき不備」と決め打ちする前提自体が、地形変化のケースを見落としている。

### 案 C: 件数しきい値なしで常に続行（不採用）

`unmatched` をすべて要確認として無条件に流す案。シンプルだが、本物の解析バグでサミットが大量に紐付かなくなったケースを見逃すリスクが残る。担当者が件数を見て異常に気づく前提は安全弁として弱い。件数しきい値で「噴火＝少数」と「解析バグ＝大量」を切り分け、後者は明示的に停止させる。

## Consequences

### 影響を受ける文書

- **[FR-009](../20_SRS.md#fr-009-sotaリスト突合match_status-判定)**:
  - `summit.match_status="unmatched"` の定義（L868 付近）を「本来発生しないべき不備・停止」から「要確認・続行」へ改訂
  - 主ピーク特定（L891 付近）の「`unmatched` はエラー停止が発動」を「要確認として記録（停止しない）」へ改訂
  - 不備ゲート（L920-921 付近）の条件を「`unmatched` ≥ 1 件で停止」から「`unmatched` 件数 > `unmatched_review_threshold` で停止」へ改訂
  - `merged_summit.geojson` フィーチャ構成（L942 付近）に `unmatched` 行（孤立 summit Point のみ）を追加。既存 SOTA サミット Point の `match_status` 値域（L983 付近）に `unmatched` を追記
- **SRS データ辞書 [2.2.1](../20_SRS.md#221-設定可能項目)**: `unmatched_review_threshold`（要確認サミット件数しきい値）を追加
- **[ADR-SRS-011](ADR-SRS-011-delete-zone-polygon.md)**: `unmatched` の「停止」記述・末尾「未確定事項」を本 ADR 決定に整合
- **[ADR-URD-007](ADR-URD-007-peak-match-status-terminology.md)**: summit.match_status テーブルの `unmatched` 説明を「エラー、処理中止」から「要確認（件数しきい値超過で停止）」へ
- **[ADR-SRS-033](ADR-SRS-033-defect-confirmation-via-xlsx.md)**: `unmatched` 行の xlsx 出力は維持。ただし出力の動機が「不備ゲートで停止する対象」から「要確認として続行・提示する対象」へ変わる旨を補足
- **[ADR-SRS-035](ADR-SRS-035-viewer-category-filter-feature-mapping.md)**: カテゴリ別表示フィルタを 4 分類から 5 分類（needs_review 追加）へ拡張
- **[GLOSSARY](../00_GLOSSARY.md)**: 「要確認サミット」用語を追加。`match_status` を「matched / delete の 2 値」から「matched / delete / unmatched」へ更新
- **実装**: コードは未実装フェーズ（[ADR-SRS-011](ADR-SRS-011-delete-zone-polygon.md) が採用・未実装）のため、本 ADR の反映対象は SRS/ADR の文書のみ

### 未確定事項

- `unmatched_review_threshold` の既定値（暫定 10）の妥当性。初回全国解析の実データで `unmatched` 発生件数の分布を確認し、確定値を別途決定する（実測根拠を持つまでは暫定値として運用）
