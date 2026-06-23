# docs/ markdown lint 残件（MD040/MD041・13件）の修正

## Context（なぜ / 何を解決するか）
前タスクで markdown 構造系 55 件（MD031/MD032/MD034/MD028）を修正・コミット済み（`ff8ec24`）。
`make lint-md` の残違反は MD040（11件）と MD041（2件）の計 13 件のみ。これらは「言語名・先頭見出し」に
判断を要するため構造系とは分けて後回しにしていた。本タスクで全件を解消し `make lint-md` を exit 0 にする。

中身の調査結果:
- **MD040（言語名なしコードブロック・11件）**: 中身はすべてシンタックスハイライト不要なもの
  （処理フロー俯瞰図・グリッド図・ツリー構造図などの ASCIIアート、計算式、申請書の書式テンプレート、テーブル例示）。
- **MD041（先頭が見出しでない・2件）**: `ADR-SRS-012` と `ADR-SRS-018` だけ、先頭の
  `# ADR-SRS-NNN: タイトル`（H1）が欠落し、いきなり `| 状態 | … |` テーブルから始まっている。
  他の ADR（例: `ADR-SRS-013`/`ADR-SRS-015`）は全て H1 タイトルで始まる。書式統一の観点でも補完が妥当。

## 実装内容

### 1. MD040: コードブロックに ` ```text ` を付与（11件）
言語名を一律 `text` に統一する（中身がいずれも図・式・テンプレートで、ハイライト対象言語がないため）。
各箇所、開きフェンス ` ``` ` を ` ```text ` に変更する（インデント付きフェンスはインデントを保持）。

対象（ファイル:行）:
- `docs/20_SRS.md`: 198（フェーズ別処理フロー図）, 436（オーケストレーション順序図）,
  895 / 905 / 911（申請書 rationale 書式テンプレート）
- `docs/CLAUDE.md`: 84（ヘッダーテーブルの例示）
- `docs/decisions/ADR-SRS-004-…`: 83（コーナー配置表）
- `docs/decisions/ADR-SRS-011-…`: 42（delete_zone_max_drop 計算式）
- `docs/decisions/ADR-SRS-013-…`: 42（merged_summit.geojson ツリー構造図）
- `docs/decisions/ADR-SRS-014-…`: 45（FR 記述構造テンプレート）
- `docs/decisions/research/3x3-mesh-analysis-study.md`: 97（グリッド境界図）

※ `CLAUDE.md:84` と `ADR-SRS-014:45` は markdown テーブルを含むため意味的には ` ```markdown ` も可だが、
  説明文混在のテンプレートであり、全体の統一性を優先して `text` で揃える。

### 2. MD041: 欠落している H1 タイトルを補完（2件）
正常 ADR と同じ `# ADR-SRS-NNN: タイトル` + 空行 を先頭に挿入する。タイトルは Context の内容と
ファイル名に基づく:
- `docs/decisions/ADR-SRS-012-terrain-image-downscaling-method.md`
  → 先頭に `# ADR-SRS-012: 標高地形図の縮小方式` を追加
- `docs/decisions/ADR-SRS-018-northern-territories-skip-at-tile-fetch.md`
  → 先頭に `# ADR-SRS-018: 北方領土除外のタイル取得段階での実施` を追加

### 3. コミット
`docs/` 配下の変更を Conventional Commits・本文日本語でコミット（push は別途指示まで不要）。
例: `style(docs): markdown lint 残件を解消（MD040 言語名付与・MD041 ADR見出し補完）`

## 検証
1. `make lint-md` を実行し、**exit 0（違反0件）** になることを確認する。
2. `git diff` で、MD040 はフェンス行のみ変更・MD041 は先頭2行追加のみ（本文無改変）であることを目視確認する。

## モデル運用メモ
本実装はファイル編集中心の単純作業（フェンス書き換え11箇所＋見出し追加2箇所）。
ExitPlanMode 承認後は **Sonnet** での実施を推奨する。
