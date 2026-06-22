# 計画: FR-013レビュー由来 課題6件＋TODO3件の対応

## Context

2026-06-21 の `/spec-panel FR-013` レビューで洗い出した指摘を tracker 登録した（ISSUE-121〜126 + todo.md 3件）。
本セッションでその9件を対応する。**全件 SRS 文書（`docs/20_SRS.md`）の記述整理であり、コード変更は伴わない**（仕様優先原則）。
根因は「viewer が消費する `merged_summit.geojson` のスキーマ正本が、生成者 FR-009 ではなく消費者 FR-013 に置かれている」こと。これが metadata/プロパティの重複・不一致・死に仕様の温床になっている。

## 対象9件の分類

| ID | 種別 | 内容 | 検討要否 |
|---|---|---|---|
| ISSUE-121 | 設計/高 | スキーマ正本を FR-013 → FR-009 へ移設 | 方向は明確（推奨で実施） |
| ISSUE-122 | 改善/中 | FR-013(生成)↔FR-019(ブラウザ機能) 役割境界整理 | 推奨で実施 |
| ISSUE-123 | 改善/中 | viewer 使用 metadata キー列挙の重複解消 | 推奨で実施 |
| ISSUE-124 | 改善/中 | AZ `area_complete=false` 死に仕様の明確化 | 推奨で実施 |
| ISSUE-125 | 設計/中 | `key_col_resolved=false` 時の prominence 値表現 | **要決定** |
| ISSUE-126 | 設計/中 | dominant 複数削除候補の対応識別プロパティ | **要決定（ADR要否含む）** |
| TODO 1 | 低 | summit_name_jp 取得元記述修正 | 機械的 |
| TODO 2 | 低 | feature_type 用語ゆれ統一（col→key_col） | 機械的 |
| TODO 3 | 低 | FR-013概要に「作業用のみ生成」明記 | 機械的 |

## 推奨する役割分担（121/122/123 の核）

3者の責務を以下に再整理する：

- **FR-009（生成者・スキーマ正本）**: `merged_summit.geojson` の全フィーチャ構成（match_status 別）＋各フィーチャのプロパティ表＋metadata 定義を保持。現状 FR-013 L959-1045 にある表を FR-009 説明セクション（L917 metadata 付近）へ移設。
- **FR-013（生成手順）**: 入力=merged_summit.geojson / 出力=merged_viewer.html。テンプレート同梱・JS変数埋め込み・出力パスの「生成機構」記述（現状 FR-019 L1099-1100）をこちらへ移す。スキーマは FR-009 を参照。
- **FR-019（ブラウザ提供機能）**: 表示・編集・検索・エクスポート。スキーマは FR-009 参照。viewer が表示に使う metadata キー一覧はここに一本化（FR-013 L954-958 の重複リストは削除し FR-009 参照に）。

## 各件の対応方針

### ISSUE-121（移設・要注意の大作業）
- FR-013 L959-1045 の「フィーチャ構成」表＋「各フィーチャのプロパティ」全表を FR-009 へ移設。
- FR-013 側は「生成するフィーチャ／プロパティの定義は FR-009 を参照」に置換。
- **他8件はこの移設後の位置（FR-009内）に対して適用する**ため、121 を最初に実施。

### ISSUE-122
- FR-019 L1099-1100 の生成機構記述を FR-013 へ移動。FR-019 は「FR-013 が生成した HTML をブラウザで開いた際の機能を定義」に集約。

### ISSUE-123
- metadata キー列挙の正本は FR-009（L917-923）。FR-013 L954-958 の6キー列挙を削除し FR-009 参照に。
- 「viewer が表示に使うキー」一覧は FR-019 L1123-1126 に一本化（summitslist_date / gsi_tile_latest_date / generated_at）。

### ISSUE-124
- FR-009 移設後のスキーマで `area_complete` は true/false 両値を定義（FR-009 は不備ゲートで false を検査するため意味を持つ）。
- FR-013/FR-019 側に「viewer 到達 geojson では常に true（false は上流 FR-009 で停止し非到達）」と注記。死に仕様ではなく文脈差として明確化。

### ISSUE-125（決定済み: null）
- peak Point の `prominence` プロパティは `key_col_resolved=false` 時に **`null`** とする（キーは常に存在・値のみ null）。
- FR-009 移設後のスキーマ（peak Point プロパティ表 `prominence` 行）に「`key_col_resolved=false` 時は `null`」と明記。
- FR-019 表示「未定義」と整合（JS は `prominence ?? '未定義'` で処理可能）。

### ISSUE-126（決定済み: 現状維持＋明記）
- 新プロパティは追加しない（実害限定的・地図描画は幾何で成立）。ADR 不要・FR-009/019/012/021 への波及なし。
- FR-009 の dominant 説明（または移設後の coord_diff プロパティ表）に「dominant で削除候補が複数の場合、各線は同一の `summit_code`（ピーク仮コード）を持ち、線の属性では個別の削除候補を識別しない。対応は幾何（線の終点座標）で成立する」と明記して決着。

### TODO 1
- `summit_name_jp`（FR-013 L977・L1005）の「geojson_v{N} から取得」を「FR-009 が geojson_v{N} から取得し格納」と読める表現へ。移設後は FR-009 内で「本 FR が取得し格納」と表現。

### TODO 2
- FR-019 L1106 `col=▼` → `key_col=▼`、L1133「Keyコル（col）popup」→「Keyコル（key_col）popup」。feature_type 値の正（key_col）に統一。

### TODO 3
- FR-013 概要 L938 に「作業用ビューアのみ生成。公開用は FR-020 が別途生成」を1行明記。

## 決定事項（確定）

- **ISSUE-125**: prominence 未確定時は `null`（キーは残す）
- **ISSUE-126**: 新プロパティ追加せず。現状維持を SRS に明記して決着（ADR 不要）

## 実装順序

1. ISSUE-121（スキーマ移設）← 最初。他はこの位置に適用
2. TODO 1/2/3（機械的文言修正）
3. ISSUE-122/123（役割境界・重複解消）
4. ISSUE-124（注記）
5. ISSUE-125（Q1 決定反映）
6. ISSUE-126（Q2 決定反映。ADR 作成する場合はここで）

## 検証

- `docs/20_SRS.md` 内 grep で旧記述残存ゼロを確認（`col=▼`、FR-013 内のプロパティ表重複、metadata 6キー重複）
- FR-009/FR-013/FR-019 の相互リンクが切れていないこと（アンカー確認）
- tracker: ISSUE-121〜126 を `issue close`、todo.md 3件を削除
- ドキュメント更新後、作業ターン内に commit（push は別途）

## 注記（モデル）

- 大半は文書編集。ただし 121 の移設は参照整合に注意が必要、125/126 は設計判断を含む。
- ExitPlanMode 承認後にモデル推奨を提示する。
