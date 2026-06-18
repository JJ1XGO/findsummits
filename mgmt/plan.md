# 計画: ISSUE-106 — UR-013（中間可視化）ピーク↔コル対応の表示方式（完了）

FR-016/FR-018 の中間 GeoJSON に peak/key_col Point・peak_col_link LineString を追加（ADR-SRS-026）。
SRS FR-016/FR-018/FR-009/NFR-009 反映・ADR-SRS-022 前方リンク追加。ISSUE-106 対応完了。
ISSUE-100/101/102 の resolved_date を 2026-06-15 に補完。
コード追従（mesh_analyze.c・merge.py・FR-009絞り込み）は todo.md に転記済み。

---

# 計画: UR-013（観測可能性）の SRS 反映 第1段（完了）

NFR-009「観測可能性（中間成果物の可視化）」新設・FR-018トレース追加・ADR-SRS-025作成。
ISSUE-107 対応完了。ピーク↔コル対応の表示方式（ISSUE-106）は第2段（別セッション）。
詳細: `docs/decisions/ADR-SRS-025-observability-nfr-ur013-srs-scope.md`

---

# 計画: 観測可能性要件（中間成果物の可視化検証）を URD に追加（完了）

UR-013 新設・ADR-URD-016 作成・ISSUE-105（対応完了）・ISSUE-106（次工程: UR-013 を満たす
SRS 設計検討、ピーク↔コル対応の表示方式）。詳細は `docs/decisions/ADR-URD-016-observability-intermediate-visualization-ur.md` 参照。

---

# 計画: FR-018 統合タイミング・絞り込み仕様の確定（SRS 反映）（完了）

## Context

FR-018（per-mesh activation.geojson 統合）のレビューで、ユーザーから
「FR-018 の入力に merged_peak.csv が無くて良いのか」という論点が提起された。
精読の結果、以下が確認された。

- FR-016 は FR-007 の一次フィルタ（130m）通過ピークをポリゴン化するため、
  per-mesh GeoJSON には 130〜150m で最終脱落するピークのポリゴンが含まれる。
- FR-008 の最終150mフィルタは merged_peak.csv 側にのみ掛かる。
- 現状 FR-018 は merged_peak.csv を参照せず全ポリゴンを統合するため、
  最終ピーク集合とポリゴン集合が一致しない。これにより
  (A) `is_area_incomplete` フラグの判定母集団がずれ偽陽性で異常終了しうる、
  (B) FR-009 の point-in-polygon で脱落予定ゾーンに SOTA サミットが誤マッチしうる。

さらに、広域解析（FR-014）は重く待ち時間が長く、その間人間は各世代の
merged_activation.geojson を地理院地図で確認したい（コル確定済みピークのゾーン確認・
未確定ピークの位置の見当付け）という運用要件が決定打となった。FR-018 を最終段1回にすると
この途中経過が失われる。FR-018 は FR-014 に比べ軽処理であり、毎ループ再生成してもコストは
無視できる。

→ **決定**: FR-018 を FR-008 とセットで FR-022 ループ内を毎回再入させ、
その世代の merged_peak.csv のピーク集合（peak_lat/peak_lon）でポリゴンを絞る。

（スコープ外・追って検討: 可視化を見てユーザーが解析を途中中断するシナリオ）

ADR-SRS-024 は Opus 継続中に作成済み（`docs/decisions/ADR-SRS-024-fr018-loop-reentry-and-peak-filter.md`）。
ここから Sonnet で SRS 本文反映を進める。

## タスク1: SRS 本文反映（`docs/20_SRS.md`）

1. **FR-018 入力テーブル**（行726付近）
   - 「統合ピーク候補 work CSV（merged_peak.csv）」を必須入力として追加。
     備考: その世代の FR-008 出力。peak_lat/peak_lon でポリゴンの絞り込みに使用。

2. **FR-018 説明**（行737-743）
   - 絞り込みロジックを追記: per-mesh GeoJSON を統合後、merged_peak.csv に存在する
     peak_lat/peak_lon のポリゴンのみを採用する（最終ピーク集合と一致させる）。
   - 再入可能性（行743）を書き換え: 「フェーズ2-2 実行前の1回のみ」→
     「FR-008 とセットで FR-022 ループ内を毎回再入し、その都度 merged_activation.geojson を
     再生成する（人間が各世代を可視化確認できるようにするため）」。
   - is_area_incomplete（行741）の判定母集団が絞り込み後の最終ピーク集合になる旨を明記。

3. **FR-009 説明**（行805付近）
   - merged_activation.geojson が最終ピーク集合と一致するため orphan ポリゴンは
     発生しない旨を明文化。

4. **データフロー処理俯瞰図**（行208-224付近）
   - FR-018 を FR-022 ループ内（FR-008 の直後）に配置。最終段は
     FR-008 → FR-018 → FR-009 の並びにする。

5. **FR トレーサビリティ表**（行179付近）
   - FR-018 のフェーズ列を実態（ループ内 = フェーズ2-1/2-2 双方）に合わせて調整。

6. **merged_activation.geojson の更新方式**
   - 出力備考に「同一ファイルを毎世代上書き再生成」を明記。

## タスク2: トラッカー

- 本論点（FR-018 入力・ループ内再入・可視化要件）を issue として登録（仕様議論を伴うため）。
- SRS/ADR 反映完了後 `issue close`（対応完了）。

## 検証

- `grep -nE "1回のみ|フェーズ2-2 実行前" docs/20_SRS.md` で FR-018 の旧記述が残らないこと。
- ADR-SRS-024 へのリンクがリンク切れしないこと。
- データフロー俯瞰図とトレーサビリティ表で FR-018 の配置が説明本文と整合すること。
- ドキュメント更新後はその作業ターン内で commit（push は別途指示）。
