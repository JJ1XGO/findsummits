# FR-009 レビュー反映：全国一括突合への純化 + 異常終了一本化 + GeoJSON 名称統一

## Context

FR-009（SOTAリスト突合）のレビューで、ユーザー確認を経て3つの方針が確定した。

1. **運用前提＝「全国を統合してから一括突合」**。部分解析前提の機構（FR-010・メッシュリスト入力・範囲外/DEM無効フラグ）は原理的に不要。
2. **FR-009 の異常終了フロー**が「即終了」と「最後にまとめて」の2通りに読め、調査用出力方針と矛盾。一本化が必要。
3. **GeoJSON 名称が実態と乖離**。UR-013 で中間 GeoJSON が5種フィーチャ（ピーク/コル Point・activation_zone/delete_zone Polygon・対応線）を含むようになり、`activation` 名はセットの CSV とも噛み合わない。

仕様優先原則によりコードは対象外（コード追従は別途 todo.md）。

## 確定した設計判断（ユーザー合意済み）

- 運用前提＝全国一括突合。FR-010・FR-009 メッシュリスト入力・`is_out_of_range_summit`・`is_dem_invalid_summit` を削除。
- FR-009 の「止める」不備フラグは異常3つ（`is_unmatched_summit`/`is_area_incomplete`/`is_key_col_unresolved`）に限定。異常終了は「全判定後にまとめて。merged.geojson は調査用に出力してから停止」に一本化。
- GeoJSON ファイル名を**セットの CSV と同 basename（拡張子違い）**に統一。`feature_type` の `activation_zone`/`delete_zone`（ゾーン種別名）は据え置き。

## 安全性・前提（確認済み）

- UR-003 は FR-009 ほか複数 FR でカバーされ、FR-010 削除で要件の穴は開かない。
- `analysis_count`/`expected_count`/`stability` は FR-008 が merged_peak.csv に算出済み（762-764行）。FR-009 は読むだけ。
- 役割分担（形状=GeoJSON / 属性=CSV）は ADR-SRS-022 の意図的設計。CSV 入力は維持。

---

## 作業1: ADR-SRS-028（全国一括突合・FR-010削除）

`docs/decisions/ADR-SRS-028-nationwide-batch-matching-fr010-removal.md`
- **状態**: 採用・未実装 / 決定日 2026-06-18
- **Decision**: 運用前提を全国一括突合に確定。FR-010・メッシュリスト入力・範囲外/DEM無効フラグを削除。
- **Alternatives**: (a) FR-010 を残す（却下: YAGNI・二重管理）/ (b) フラグは記録のみ残す（却下: 全国一括では発生せず死蔵）
- **Consequences**: FR-009 不備フラグが3つに簡素化。将来部分解析が必要なら本 ADR を見直す。

## 作業2: SRS（docs/20_SRS.md）— FR-010削除・異常終了一本化・記載補完

**削除**: 47行（目次 FR-010）・187行（RTM FR-010）・840行（FR-009 入力メッシュリスト）・917行（is_out_of_range_summit）・918行（is_dem_invalid_summit）・928-949行（FR-010本体）

**修正**:
- 832行（概要）: 突合はGeoJSONのポリゴン・属性はCSVからjoinする旨を補強（指摘⑥）
- 864行: unmatched「ログ警告して異常終了」→「`is_unmatched_summit` を立てる」（即終了をやめる）
- 879行: 「メッシュ範囲指定（FR-010）の有無に関わらず」→ FR-010 参照を除去
- 882-884行（stability 再定義）: 二重定義のため削除し「FR-008 が付与した値を使用」へ参照化（指摘⑤⑦）
- 919行: 異常終了対象を3フラグに限定＋「全判定後にまとめて、merged.geojson 出力後に停止」を明記（指摘④）
- 1536行: メッシュリストの「使用 FR」から FR-009 を除外

**追加**: 835-841行（入力テーブル）に「N03 前処理済み 地域/市区町村 GeoJSON」2行（指摘①）＋ merged_peak.csv 備考に「analysis_count/expected_count/stability 列を含む（FR-008 算出）」

## 作業3: ADR-SRS-029（GeoJSON 名称統一）

`docs/decisions/ADR-SRS-029-geojson-naming-align-with-csv.md`
- **状態**: 採用・未実装 / 決定日 2026-06-18
- **Decision**: 中間 GeoJSON をセットの CSV と同 basename に統一。`activation_zone`/`delete_zone` の feature_type は据え置き。
  - per-mesh: `<解析識別子>_activation.geojson` → `<解析識別子>.geojson`
  - merged: `merged_activation.geojson` → `merged_peak.geojson`
  - 和名: 「per-mesh アクティベーションゾーン GeoJSON」→「per-mesh ピーク候補 GeoJSON」、「統合済みピーク域 GeoJSON」→「統合ピーク候補 GeoJSON」
- **Context/理由**: UR-013 で中間 GeoJSON が5種フィーチャを含むようになり `activation` 名が実態より狭い。CSV とセットで生成されるのに basename が不揃いだった。
- **Alternatives**: peak_zone（ピーク域）案（却下: CSV と basename 不一致）/ 現状維持（却下: 実態乖離）

## 作業4: SRS + 関連 ADR の名称一括置換

- **ファイル名**: `merged_activation.geojson`（12箇所）→ `merged_peak.geojson`、`<識別子>_activation.geojson`（20箇所）→ `<識別子>.geojson`
- **呼称**: 「per-mesh アクティベーションゾーン GeoJSON」「統合済みピーク域 GeoJSON」を新和名へ
- **据え置き**: `feature_type="activation_zone"` 等のゾーン種別名（「アクティベーションゾーン」26箇所は"ファイル呼称"か"ゾーン種別"かを1件ずつ精査して使い分け）
- **対象範囲**: データフロー図（209-220行）・データ一覧（7.2）・FR-007/016/018/008/009、関連 ADR（022/024/026/013/011/023/027 のファイル名参照）

## 作業5: トラッカー登録（1論点1課題）

- **issue①**: 運用前提＝全国一括突合の確定・FR-010 等削除（種別: 設計）
- **issue②**: FR-009 異常終了フローの一本化（種別: 設計）
- **issue③**: GeoJSON 名称を CSV に揃える統一（種別: 設計）
- 記載補完（指摘①⑤⑥⑦）は作業2 と同時実施・todo.md 管理

## 検証

```bash
grep -n "FR-010\|is_out_of_range\|is_dem_invalid" docs/20_SRS.md          # 残存ゼロ
grep -rn "_activation\.geojson\|merged_activation" docs/                  # 残存ゼロ
grep -rn "activation_zone\|delete_zone" docs/                            # feature_type は残る（据え置き確認）
grep -rn "ADR-SRS-028\|ADR-SRS-029" docs/                                # 相互参照確認
```
- RTM から FR-010 行が消え整合
- ドキュメント・ADR・トラッカー分を Conventional Commits でコミット（push は別途指示まで不要）

## 補足（別件・保留中）
- 初回依頼の「対応完了 ISSUE-108/109/110 の解決済み化（`issue verify`）」は本計画と独立。ユーザー確認後に実行。
