# 計画: SRS レビュー継続

## ISSUE-014: 削除候補サミットの最近接検出ピーク特定（SRS コンテンツレビュー時に仕様化）

deleted サミットが「どの検出ピークに最も近いか（最近接検出ピーク）」をデータとして持つ。
- 追加カラム: `dominant_peak_code` / `dominant_peak_dist_m`（FR-012）
- GeoJSON に deleted → 最近接ピークの LineString 追加（FR-013）
- 申請書根拠列に最近接ピーク情報を自動記入（FR-011）
- SRS コンテンツレビュー FR-009・FR-012・FR-013・FR-011 時に仕様確定・更新する

---

# 計画: SRS 機能要件レビュー（次回セッション）

## Context

FR-008 → FR-009 の間にアーキテクチャ課題が発見された。次回はまずフェーズ構成の変更方針を議論する。

## 重要: FR-014 を FR-009 の前に移動する必要がある

FR-008 完了後も `is_tile_top=1`（プロミネンス未確定）と `area_truncated=true`（ゾーン途切れ）の
ピークが残る。これらは FR-009 の point-in-polygon 突合が正しく動作する前に解消が必要。
→ FR-014（広域再解析）を FR-009 の前に移動し、両フラグをクリアしてから突合に入る設計に変更する。

ISSUE-017（activation.geojson 統合）は FR-008 とは別立ての新 FR として追加する方向。

## 次回の進め方（要議論）

1. SRS フェーズ構成の変更方針を合意する
   - FR-014 の移動先（フェーズ3.5 的な位置）
   - ISSUE-017 の FR 番号・位置
   - FR-009 の入力記述更新
2. 合意後、SRS を修正する
3. FR-014 レビュー → ISSUE-017（新 FR）→ FR-009 の順でレビュー再開

## レビュー進捗

完了: FR-017 → FR-001 → FR-002 → FR-003 → FR-004 → FR-005 → FR-006 → FR-007 → FR-008 → FR-015 → FR-016
次回: フェーズ再設計合意 → FR-014（移動後）→ ISSUE-017 → FR-009 → FR-010 → FR-013 → FR-011 → FR-012 → NFR-001〜007

---

## 次回の作業方針

- SRS 機能要件を FR 番号順にレビューする（FR-001 から）
- 各 FR について: 実装との整合・TBD 残存・抜け漏れを確認する
- ISSUE-006（tolerance_px）: FR-009 のアクティベーションゾーン方式への移行で実質解消済みのため、
  次回レビュー時に正式クローズするか判断する

---

## 未解決課題（要検討）

- **ISSUE-012: matched ピークの標高差扱い**
  - matched かつ `peak_elev ≠ sota_alt_m` の場合、申請書に「変更」として出力するかどうか
  - 閾値（何 m 以上の差異で変更扱いにするか）をユーザーが検討中

---

<!-- 以下は実装済み（今セッション）-->
# 実装済み: 新規ピークへの都道府県ベース仮コード割り振り（完了）

## 実装内容

- `scripts/preprocess_pref_boundaries.py` 新規作成
  - 国土数値情報 N03-2026 GeoJSON → 都道府県/振興局単位に dissolve → `$DATA_DIR/ref/N03-2026_regions.geojson`
  - 47都道府県 + 北海道14振興局（計61地域）対応
- `scripts/merge.py` 更新
  - `load_regions()` / `assign_temp_summit_code()` 追加
  - 新規ピークに `JA/<area>-A<seq>` 形式の仮コード付与（海上・未判定は `ZZ/ZZ-A<seq>`）
  - `--regions-file` 引数追加（デフォルト: `$DATA_DIR/ref/N03-2026_regions.geojson`、未存在時は自動フォールバック）
- ドキュメント更新: SRS（FR-017, 6.9, セクション3, セクション7）・environment.md・SOURCES.md・CLAUDE.md・.gitignore
- ISSUE-009 対応完了

## ユーザー作業（未実施）

1. https://nlftp.mlit.go.jp/ksj/gml/datalist/KsjTmplt-N03-2026.html から全国 GeoJSON をダウンロード
2. `$DATA_DIR/ref/N03-2026.geojson` に配置
3. `python3 scripts/preprocess_pref_boundaries.py` を実行 → `$DATA_DIR/ref/N03-2026_regions.geojson` 生成

---

<!-- 以下は実装済み（前セッション）-->
# 実装済み: GLOSSARY・SOURCES へのアクティベーションゾーン追加（完了）

アクティベーションゾーン（Activation Zone）の用語定義を GLOSSARY・SOURCES に追加。
SOTA日本支部 FAQ Q12 へのリンク付き。

---

# 実装済み: SRS セクション3・FR-009・FR-013・FR-016 の設計変更（完了）

- セクション3 アーキテクチャ図: 3ステップ運用フロー・フェーズ番号追記
- FR-009: tolerance_px 廃止 → アクティベーションゾーン Flood Fill 方式
- FR-013: GeoJSON フィーチャ構成にアクティベーションゾーンポリゴン追加（TBD-03 解消）
- FR-016 新規: アクティベーションゾーン計算（findsummits C）
- 6.8 新規: アクティベーションゾーン GeoJSON インターフェース
