# FR-007 レビュー反映 — per-mesh CSV 出力仕様の修正

## Context

FR-006 レビュー（ISSUE-094/095・ADR-SRS-020）に続く **FR-007（per-mesh CSV 出力）レビュー**。
SRS FR-007 を周辺仕様（データ辞書・FR-006 出力・FR-008 消費・FR-014 広域モード）と
突き合わせた結果、以下の欠陥・曖昧さ・冗長を検出し、ユーザーと方針を合意した。

本作業は **仕様（SRS）の修正のみ**。コードは仕様優先原則に従い後続で追従する
（現行 merge.py は広域未対応のため設計根拠にしない）。

## 合意済みの修正方針

### 1. 🔴 `key_col_resolved=false` ピークへの一次フィルタ適用（最重要・欠陥）
- 現状: 「一次フィルタ閾値を超えたピーク候補のみ出力」だが、`false` 行は prominence が空欄で判定不能。
  文面通りだと独立峰が出力されず、FR-014 広域解析に回らず消える（パイプライン破綻）。
- 決定: **`key_col_resolved=false` 行は一次フィルタを適用せず無条件出力**。
  一次フィルタは `key_col_resolved=true`（prominence 確定済み）行のみに適用。

### 2. 🟡 一次フィルタの境界整合
- 現状: 一次フィルタ「超えた(>)」／最終フィルタ・データ辞書「以上(≥)」で不整合。
- 決定: 一次フィルタも **「以上(≥)」** に統一。

### 3. provenance 列を解析識別子へ集約（center_mesh / zoom_level 廃止）
- 現状: `center_mesh`（広域で中心メッシュ概念が無く誤名）＋ `zoom_level`（接頭辞から導出可・現状未消費）。
- 決定: 両列を廃止し **`analysis_id`（解析識別子: `3-5239` / `4-5239-NW`）1 列**に集約。
  mode/mesh/corner/zoom を 1 トークンに内包。後段が必要分をパースする
  （FR-008 は接頭辞 `3` vs `4/5/6` で通常/広域を判別 — SRS 既定の規約）。

### 4. 派生列 `points` を除外
- 現状: `points` は peak_elev からの純粋派生。FR-008 は読まず（捨てる）、FR-009 は peak_elev から再計算、
  GeoJSON/xlsx も peak_elev/sota_alt_m から算出。FR-007 で書いても即捨てられるデッドカラム。
- 決定: **FR-007 から `points` を削除**。消費する段（FR-009/出力生成）で peak_elev から算出。

### 5. 🟡 `col_margin_px` の意味明確化
- `key_col_resolved=false` 時の測定起点（海面ボーダー接触ピクセル → グリッド端、典型的に小さい値）を明記。
- col_margin_px はズーム依存（通常=L15px・広域=L14px）であることを注記（analysis_id の接頭辞で判別可）。

### 6. 🟢 広域モードのフィルタ関係明確化
- 地理的範囲フィルタ（通常・広域共通）と対象ピーク絞り込み（広域のみ）の適用関係を 1 行明記。

## 変更対象ファイルと箇所

### `docs/20_SRS.md`（FR-007 = 591〜634 行が主対象）
- 594 / 613 行: 一次フィルタ文言 — `key_col_resolved=false` 無条件出力を追記、「超えた」→「以上」
- 615〜630 行 出力カラム表:
  - `points`（622）削除
  - `center_mesh`（629）+ `zoom_level`（630）削除 → `analysis_id` (str) 追加
  - `col_margin_px`（628）: false 時の起点・ズーム依存を明確化
- 614 / 632〜634 行: 広域モードのフィルタ適用関係を明記
- **FR-008 消費記述**（687〜704）: FR-008 が analysis_id 接頭辞で通常/広域を判別し、
  広域行は expected_count（3×3 隣接カウント）をスキップする旨を追記。points 依存があれば除去
- **points 帰属の整合**: 1176 行付近（merged 出力カラム）等で points が「FR-007 由来」になっていないか確認し、
  「peak_elev から算出（FR-009/出力段）」へ統一
- 1239 行（NFR）: 一次フィルタ文言の整合確認

### `docs/decisions/ADR-SRS-021-per-mesh-csv-column-design.md`（新規）
- 決定: per-mesh CSV の provenance を解析識別子 1 列へ集約・派生列 points を除外
- Alternatives: 分解列案（mode/mesh/corner/zoom を個別列）・center_mesh 維持＋広域空欄案 → 却下理由を記録
- Consequences: FR-007/FR-008 の SRS 記述更新、コード追従は別タスク

## トラッカー登録（ユーザー確認後に add）
- ISSUE: FR-007 一次フィルタの `key_col_resolved=false` 適用未定義 → 無条件出力で決着（記録: SRS FR-007）
- ISSUE: FR-007 per-mesh CSV カラム設計整理（analysis_id 集約・points 除外）→ ADR-SRS-021
- todo.md: 境界文言（超えた→以上）・col_margin_px 文言・FR-008 消費記述追従・コード追従
  （mesh_analyze.c CSV 出力／merge.py の analysis_id 対応）

## 検証
- `grep -n "center_mesh\|zoom_level" docs/20_SRS.md` が **0 件**（残存なし）
- `grep -n "points" docs/20_SRS.md` の各ヒットが「peak_elev から算出」で、FR-007 由来でないこと
- FR-007 出力カラム表 ↔ FR-008 消費記述 ↔ 1504 行データ表の三者整合
- 解析識別子の形式（348 行 `3-<…>`・429 行 `<N>-<…>-<コーナー>`）と analysis_id 列値が一致
- ADR-SRS-021 が自己完結（mgmt/ 参照なし）・採番重複なし
- ドキュメント更新後その作業ターン内に commit（push は別途指示）

## スコープ外（本作業で触らない）
- コード（src/*.c, scripts/*.py）の実装追従 — 仕様確定後の別タスク
- FR-008 の stability/expected_count 広域セマンティクスの詳細設計 — FR-008 レビュー時に詰める
