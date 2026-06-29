# ISSUE-158: サミット一覧 XLSX 行集約モデルの SRS 定義

## Context

`merged_summit.geojson` は 4 カテゴリ（add/band_change/no_change/delete/review）のクラスタに
peak/col Point・AZ/delete_zone Polygon・各種サミット Point・LineString を大量に束ねている。
一方 `merged_summit.xlsx`（FR-009 出力）と `merged_summit_revised.xlsx`（FR-012 出力）は、これを
**サミット中心に 1 行ずつほどいて**カラム化する。この「geojson クラスタ → サミット行」への非対称な
アンフォールド規則（行の単位・各カラムの取得元フィーチャ）が SRS 未定義というのが ISSUE-158。

`ADR-SRS-041` は「全カラムが geojson から生成可能」（スキーマ拡張）までしか定めておらず、行集約
モデルが前提として欠落している。本タスクはそれを補完し、FR-009 を行集約モデルの正本として定義する。

加えて確認の過程で、現カラム表の `sota_*` 注記「matched・dominant のみ」が 5 カテゴリモデル
（`ADR-SRS-044`）と矛盾していること（dominant ピークは add＝既存登録なしなので sota_* は空が正）が判明。
あわせて修正する。

## 確定済みの決定事項（本セッションでユーザー合意済み）

- **行モデル**: 1 行 = 1 サミット（申請の主語）。1 Point = 1 行ではない。peak Point＋col Point＋AZ 内
  matched サミット Point は 1 行に集約する。Polygon / LineString は行を生まない
- **delete 行**: `peak_*`・`col_*` は空。`dominant_peak_code`＋`dominant_peak_dist_m`（距離）を残す。
  主ピークは matched / dominant 両方ありうる（既存仕様 `ADR-SRS-043` で対応済み）
- **dominant 行の sota_***: new と同じく空に統一（「matched・dominant のみ」→「matched のみ」）

### 行 → ソースフィーチャ対応（カラム群単位）

| category | 行の主語 | 行を生むフィーチャ | peak_* | col_* | sota_* | dominant_* | area_complete | is_band_change_candidate |
|---|---|---|---|---|---|---|---|---|
| add (new) | 新設＝ピーク | peak Point | このピーク | このコル | 空 | 空 | このピーク | 空 |
| add (dominant) | 新設＝ピーク | peak Point | このピーク | このコル | 空 | 空 | このピーク | 空 |
| band_change | 既存サミット | matched peak Point | このピーク | このコル | AZ 内既存サミット | 空 | このピーク | true |
| no_change | 既存サミット | matched peak Point | このピーク | このコル | AZ 内既存サミット | 空 | このピーク | false |
| delete | 削除対象既存サミット | delete summit Point | 空 | 空 | この削除サミット | 主ピーク（code+dist） | 空 | 空 |
| review | 孤立既存サミット | unmatched summit Point | 空 | 空 | この孤立サミット | 空 | 空 | 空 |

## タスク（すべて Sonnet。設計判断は確定済み・残りは文書編集と整合チェックのみ）

### 1. `ADR-SRS-045` 新規作成

ファイル: `docs/decisions/ADR-SRS-045-summit-xlsx-row-aggregation-model.md`

- 状態: 採用・未実装 / 決定日: 当日（`date` で取得）
- Context: `ADR-SRS-041` は全カラムが geojson から取れることを定めたが、行集約モデル
  （4 クラスタ → サミット行の非対称アンフォールド）が未定義だった
- Decision: 1 行 = 1 サミット。上記「行 → ソースフィーチャ対応」表を掲載。delete 行は
  `peak_*`/`col_*` 空・`dominant_peak_code`+`dominant_peak_dist_m` を保持。dominant 行 sota_* は空
- Alternatives: 1 Point = 1 行（却下: peak/col/sota が別行に散り、申請単位＝サミットと一致しない）
- Consequences: FR-009 に行モデル正本を追加。FR-012・6.2.7 の文言/注記を追従修正
- 注: `mgmt/` 参照禁止・トラッカー ID 非記載（`docs/CLAUDE.md` ADR ルール）

### 2. FR-009 に行集約モデルを正本として追加（`docs/20_SRS.md`）

- FR-009 内（`merged_summit.geojson` フィーチャ構成定義の近辺、959〜979 行付近の後）に
  新サブセクション「`merged_summit.xlsx` の行生成モデル」を追加
- 内容: アンフォールド規則（1 行 = 1 サミット、複数 Point を集約、Polygon/LineString は行を生まない）
  ＋「行 → ソースフィーチャ対応」表（カラム群単位）。`ADR-SRS-045` を参照
- カラムの詳細定義表は FR-012 側に残す（行モデル＝FR-009 正本、カラム意味＝FR-012 で参照する役割分担）

### 3. FR-012 修正（`docs/20_SRS.md`）

- 1298 行「**Point フィーチャのみが行に変換される**」→ 行モデルは FR-009 参照に変更し
  「1 行 = 1 サミット（クラスタ内の複数 Point を集約。Polygon/LineString は行を生まない）」へ文言修正
- `sota_lat`/`sota_lon`/`sota_alt_m`/`sota_points`（1323〜1326 行）の注記
  「matched・dominant のみ・new は空欄」→「matched のみ（band_change/no_change）。new/dominant は空欄」
- `peak_*`・`col_*` カラム（1311〜1320 行）に「delete/review 行は空」の旨を注記、または行モデル参照を付記

### 4. 6.2.7 修正（`docs/20_SRS.md` 1583 行）

- 「含む情報: Point フィーチャの属性のみ」→ 行モデル（FR-009 参照）に整合する文言へ修正
  （「1 行 = 1 サミット。複数 Point を集約」を明示）

### 5. トラッカー更新

- 着手時: `venv/bin/python3 mgmt/tracker/track.py issue update ISSUE-158 --status 対応中 --actor Sonnet`
- 完了時: `issue close`（記録完了＝対応完了。理由に ADR-SRS-045 作成・FR-009/FR-012/6.2.7 修正を記載）

## 検証

- `make lint` 警告ゼロ（`lint-md` の broken-link・MD 規約・トラッカー ID 検査を含む）
- `grep -n "dominant のみ\|matched・dominant" docs/20_SRS.md` で旧注記の残存ゼロを確認
- `grep -n "Point フィーチャのみが行に変換" docs/20_SRS.md` で旧文言の残存ゼロを確認
- ADR 相互参照（`ADR-SRS-041` ↔ `ADR-SRS-045`・FR-009 ↔ ADR-SRS-045）のリンク整合を目視
- ドキュメント更新ルールに従い、当ターン内で Conventional Commits でコミット（push は別途指示まで不要）

## モデル指定

全タスク **Sonnet**。設計判断は本セッションで確定済みで、残作業は文書編集・文言修正・整合チェックのみ。
実装着手前に現セッションが Opus のため、ユーザーへ `/model` で Sonnet 切替を促す。
