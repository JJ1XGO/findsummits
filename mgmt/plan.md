# 計画: SRS 機能要件レビュー（次回セッション）

## Context

今セッションで構想フェーズ（SRS 設計変更・新規スクリプト実装）が一段落した。
次回は SRS の機能要件を FR-001 から順番にレビューし、未解決 TBD の解消と実装との整合確認を行う。

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
