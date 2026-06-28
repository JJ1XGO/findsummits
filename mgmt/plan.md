# FR-012 spec-panel レビュー対応計画

## Context

`spec-panel` で FR-012（サミット一覧（申請内容反映版）生成）を 4 視点レビューした結果、
**FR-012 の出力カラム定義と入力（`merged_summit.geojson`）が整合していない**構造的問題と、
3 件の文言不整合が見つかった。

最大の論点は、FR-012 が「`merged_summit.geojson` のみから生成（ブラウザ内・FR-013 が
geojson のみ埋め込む設計）」と「`merged_summit.xlsx`（突合後）とカラム構成を同一にする
（`docs/20_SRS.md` の §6.5 該当・FR-012 カラム参照）」の 2 要請を負うが、geojson スキーマに
診断系・行政区域系プロパティが無いため両立していない点。

ユーザー承認方針: **案A（`merged_summit.geojson` スキーマに不足プロパティを追加し、
FR-012 が geojson のみで両 XLSX と同一カラムを生成できるようにする）**。情報欠落なし・
カラム統一維持を優先。

## 指摘一覧（確定）

| # | 重要度 | 指摘 | 該当箇所 | 対応 |
|---|---|---|---|---|
| ① | 高 | 出力カラムの `col_margin_px`・`analysis_count`・`expected_count`・`municipality`・`dominant_peak_code`・`dominant_peak_dist_m` が geojson スキーマに無く、FR-012 から生成不能 | カラム `docs/20_SRS.md:1266-1277`／geojson スキーマ `docs/20_SRS.md:954-1010` | 案A: geojson スキーマ拡張＋ADR |
| ② | 中 | `summit_name_jp` 説明「本 FR が geojson_v{N} から取得し格納」が誤り（FR-012 は geojson_v{N} 非アクセス） | `docs/20_SRS.md:1256` | 「FR-009 が格納済みの値を `merged_summit.geojson` から引き継ぐ」へ修正 |
| ③ | 中 | 概要「山岳名・rationale を反映」と説明「rationale プロパティは含めない」が矛盾 | 概要 `docs/20_SRS.md:1227`・`docs/20_SRS.md:1244` vs 説明 `docs/20_SRS.md:1246` | 概要側を「山岳名（summit_name_jp）を反映」へ修正。rationale は FR-011 側で反映する旨を明記 |
| ④ | 中 | 出力備考「HTML ビューアからブラウザダウンロード」が §6「単独ダウンロードせず ZIP に同梱」と矛盾 | 備考 `docs/20_SRS.md:1240` vs §6 `docs/20_SRS.md:1113` | 「FR-021 の ZIP に同梱してダウンロード（単独ダウンロードしない）」へ統一 |

## タスク

すべて `docs/20_SRS.md` の編集と ADR 新規作成（設計判断は確定済み・機械的整合作業）。実行モデル: **Sonnet**。

### タスク1: tracker 登録（Sonnet）

- ISSUE（type=設計）: 「FR-012 出力カラムと `merged_summit.geojson` スキーマの整合（案A: geojson 拡張）」← 指摘①
- `mgmt/todo.md` に指摘②③④（FR-012 文言整合）を 1 行ずつ追記
- 登録は `venv/bin/python3 mgmt/tracker/track.py issue add`／`--actor` はモデル名（impersonation 禁止）

### タスク2: ADR 作成（Sonnet）

- `docs/decisions/ADR-SRS-NNN-merged-geojson-schema-for-revised-xlsx.md`（NNN は `ls docs/decisions/ | grep ADR-SRS` の最大値+1 で実装時採番）
- Decision: `merged_summit.geojson` スキーマに診断系・行政区域系プロパティを追加し、FR-012 が geojson のみで両 XLSX と同一カラムを生成できるようにする
- Alternatives: 案B（FR-012 カラム削減）→ 申請有用情報（municipality 等）欠落・カラム統一前提（§6.5）崩壊のため却下
- Consequences: FR-009 スキーマ拡張・FR-012 生成可能化・FR-013 埋め込み（geojson のみ）は変更不要

### タスク3: FR-009 geojson スキーマ拡張（Sonnet）

`docs/20_SRS.md:954-1010` の各フィーチャ・プロパティ表に追加:

- **Point: ピーク**（`954-967`）: `analysis_count`・`expected_count`・`municipality`
- **Point: コル**（`973-978`）: `col_margin_px`
- **Point: 既存 SOTA サミット**（`982-991`）: `municipality`・`dominant_peak_code`・`dominant_peak_dist_m`（dominant 従属の delete のみ）

留意:

- 各プロパティの定義・適用条件は既存記述（`col_margin_px`=`docs/20_SRS.md:679`、`analysis_count`/`expected_count`=`docs/20_SRS.md:765-766`、`municipality`=`docs/20_SRS.md:861`、`dominant_peak_code`/`dominant_peak_dist_m`=`docs/20_SRS.md:893`）と整合させる
- FR-009 の `merged_summit.xlsx` カラム定義箇所（§6.5 付近・`docs/20_SRS.md:1436-1458` 周辺）を確認し、geojson と xlsx と FR-012 の三者でカラムが揃うことを担保する
- ADR-SRS-013（geojson が中心データ・スキーマ正本）への準拠を維持

### タスク4: FR-012 文言修正（Sonnet）

- 指摘②: `docs/20_SRS.md:1256` の `summit_name_jp` 取得元表現を「FR-009 が格納済み・引き継ぎ」に
- 指摘③: `docs/20_SRS.md:1227`・`docs/20_SRS.md:1244` の「rationale を反映」を「山岳名を反映」に修正し、`docs/20_SRS.md:1246` と整合
- 指摘④: `docs/20_SRS.md:1240` の出力備考を §6（`docs/20_SRS.md:1113`）と統一

### タスク5: 検証・コミット（Sonnet）

- `make lint`（警告ゼロ確認）
- 整合確認（下記）
- Conventional Commits でコミット（ドキュメント更新のため作業ターン内に commit）
- ISSUE close（①の決着＝ADR 記録＋SRS 反映）・`mgmt/todo.md` の②③④消化

## 検証

- `grep -n "col_margin_px\|analysis_count\|expected_count\|municipality\|dominant_peak_code\|dominant_peak_dist_m" docs/20_SRS.md` で、FR-009 geojson スキーマ（`954-1010`）に 6 プロパティが追加されたことを確認
- FR-012 出力カラム（`1252-1278`）の全項目が geojson スキーマ上のプロパティ／geometry で賄えることを 1 項目ずつ突き合わせ
- FR-012 概要・説明・備考に rationale・単独ダウンロードの矛盾が残っていないことを確認
- `make lint` 警告ゼロ
- リンク切れ・内部トラッカーID 混入なし（`make lint-md` 検査C）

## tracker 振り分け方針

- 指摘①（仕様議論＋ADR を伴う設計判断）→ **issue（設計）**
- 指摘②③④（文言の不整合修正・議論不要）→ **`mgmt/todo.md`**（①の SRS 修正と同一ターンで一括処理）
