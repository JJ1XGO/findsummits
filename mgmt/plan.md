# 計画: SRS フェーズ構成再設計（FR-008 → FR-009 の架け橋）

## Context

SRS レビューで FR-008 を完了し FR-009 の入り口に到達した時点で、FR-008 → FR-009 の間に
構造的なギャップが見つかった。FR-008 後も以下 2 種類の「未解決」ピークが残るため、
FR-009 の point-in-polygon 突合が正しく動作しない：

| フラグ | 意味 | FR-009 への影響 |
|---|---|---|
| `is_tile_top=1` | Keyコルが 3×3 解析範囲外 → プロミネンス未確定 | 150m 超かどうか判定不能 |
| `area_truncated=true` | アクティベーションゾーンが 3×3 解析範囲外で途切れ | 本来 matched が deleted に誤分類 |

FR-014（独立峰の広域再解析）は現状フェーズ4 末尾にあるが、上記 2 フラグの解消手段である
ため FR-009 の前に置く必要がある。同時に per-mesh `*_activation.geojson` の統合ステップ
（ISSUE-017）も FR-009 の前に必要（per-mesh CSV 統合が FR-008 にあるのと対称）。

## 新フェーズ構成（ユーザー合意済み: 2026-05-09）

```
フェーズ1: データ取得・前処理・標高デコード（変更なし）
  FR-017 → FR-001 → FR-002 → FR-003

フェーズ2: ピーク・コル検出（変更なし）
  FR-004 → FR-005 → FR-006 → FR-007 → FR-015 → FR-016

フェーズ3: 統合・広域再解析（再構成）
  FR-008  per-mesh CSV 統合
  FR-018  per-mesh activation.geojson 統合（NEW・ISSUE-017 由来）
  FR-014  独立峰広域再解析（移動・責務拡張）
            - トリガー: is_tile_top=1 または area_truncated=true
            - 出力: 統合済み CSV・activation.geojson の該当レコードを上書き

フェーズ4: 突合・出力（縮小）
  FR-009 → FR-010 → FR-013 → FR-011 → FR-012
```

### 4 つの決定事項

| 決定 | 内容 |
|---|---|
| (1) フェーズ構成 | フェーズ3 を「統合・広域再解析」に改名・FR-014 を移動 |
| (2) ISSUE-017 の SRS 位置 | 新 FR-018 として独立（FR-008 に含めない） |
| (3) FR-014 の責務拡張 | トリガー = `is_tile_top=1` OR `area_truncated=true`、出力 = 統合済み CSV・activation.geojson の上書き |
| (5) FR-014 のメッシュサイズ（2026-05-10 確定） | 一次: 4×4+L14（111 km）→ エスカレーション: 5×5+L14（148 km）→ 最終残存: ログ警告・手動調査。6×6 は不採用（メモリが 3×3/L15 と同等でオーバースペック） |
| (4) FR-009 の入力記述 | 「FR-014 処理後の統合済み CSV + activation.geojson（両フラグ解消済み）」と明記 |

FR 番号は据え置き（振り直しはしない）。表示順だけフェーズ順に並べ替える。

## 次のステップ（順序）

1. SRS を一括修正・コミット:
   - 目次（TOC）のフェーズ3・フェーズ4 の見出しと FR 順
   - 3章 アーキテクチャ概要のフェーズ説明
   - フェーズ3 セクション見出しを「統合・広域再解析」に改名
   - FR-018 新設（per-mesh activation.geojson 統合）
   - FR-014 をフェーズ3 に移動・責務拡張・本文書き直し
   - FR-009 の「入力」記述を更新
2. SRS レビューを再開: FR-014（新位置）→ FR-018 → FR-009 → FR-010 → FR-013 → FR-011 → FR-012 → NFR-001〜007

## レビュー進捗

完了: FR-017 → FR-001 → FR-002 → FR-003 → FR-004 → FR-005 → FR-006 → FR-007 → FR-008 → FR-015 → FR-016
次: フェーズ再構成 SRS 一括修正 → FR-014（移動後）→ FR-018 → FR-009 → FR-010 → FR-013 → FR-011 → FR-012 → NFR-001〜007

## 修正対象ファイル

- `/workspace/docs/02_SRS.md`（目次・3章・フェーズ3 見出し・FR-008・FR-018 新設・FR-014 移動・FR-009 入力記述）

## 修正対象外（明示）

- 実装コード（`src/*.c`, `scripts/*.py`）—— SRS レビュー段階のため変更しない（仕様優先原則）
- ADR-004（FR-014 の実装設計）—— TBD-01 として SRS は留保。実装着手前に確定
- FR 番号の振り直し —— 影響範囲が大きいため見送り

## 検証方法

- SRS の TOC と本文のフェーズ番号・FR 順序が一致しているか目視確認
- FR-014 の入力（フラグ）と出力（更新先）が、FR-008・FR-018・FR-009 のインターフェースと
  整合しているかを SRS の該当箇所を読み合わせて確認
- FR-009 の入力記述が、FR-014 完了後の状態（両フラグ解消）を前提にしていることを確認

---

## ISSUE-014: 削除候補サミットの最近接検出ピーク特定（SRS コンテンツレビュー時に仕様化）

deleted サミットが「どの検出ピークに最も近いか（最近接検出ピーク）」をデータとして持つ。
- 追加カラム: `dominant_peak_code` / `dominant_peak_dist_m`（FR-012）
- GeoJSON に deleted → 最近接ピークの LineString 追加（FR-013）
- 申請書根拠列に最近接ピーク情報を自動記入（FR-011）
- SRS コンテンツレビュー FR-009・FR-012・FR-013・FR-011 時に仕様確定・更新する

---

## 未解決課題（要検討）

- **ISSUE-012: matched ピークの標高差扱い**
  - matched かつ `peak_elev ≠ sota_alt_m` の場合、申請書に「変更」として出力するかどうか
  - 閾値（何 m 以上の差異で変更扱いにするか）をユーザーが検討中
- **ISSUE-006（tolerance_px）**: FR-009 のアクティベーションゾーン方式への移行で実質解消済み。
  FR-009 レビュー時に正式クローズするか判断する

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
