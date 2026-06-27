# 計画: unmatched サミット（地形変化で消滅）の「要確認」扱い

## Context

現行仕様（`FR-009` / `ADR-SRS-011` / `ADR-URD-007`、いずれも採用・未実装）では、既存 SOTA サミットを各ピークの AZ／delete判定ゾーンへの座標包含で判定し、**どちらにも入らない場合は `summit.match_status="unmatched"` として「閾値不備または解析欠落」とみなし処理停止**する。

しかし噴火・山体崩壊・カルデラ陥没で「山が実際に消えた」場合、旧サミット座標がどのピークのゾーンにも入らず `unmatched` になりうる。これは**システム不備ではなく正当な地形変化＝削除候補**であり、現行の「即停止」設計は実態を捉えていない盲点（ユーザー指摘）。

座標だけでは「山が消えた」と「閾値不備／解析バグ」を機械区別できないため、自動削除は誤削除リスクがある。本プロジェクト原則（成果物はドラフト・最終判断は日本支部担当者／ブロックより警告して続行）に沿い、**「要確認」として担当者に提示**する方針とする。

## 決定事項（ユーザー確認済み）

1. **`unmatched` を「要確認」状態として続行扱いにする**
   - 停止トリガーから外す。`is_unmatched_summit` を「停止フラグ」から「要確認カウント」へ格下げ
   - 申請書の「削除」行には**自動で載せない**。要確認として `merged_summit.xlsx`（`ADR-SRS-033`）と HTML ビューアのカテゴリ（`ADR-SRS-035`）に提示し、担当者が判断
2. **件数しきい値で解析バグを防御**
   - `unmatched` が少数なら要確認続行、しきい値超過なら「解析異常の疑い」として停止
   - しきい値は `params/config.ini` で設定可能（噴火＝少数 と 解析バグ＝大量 を件数で切り分け）
3. `is_area_incomplete` / `is_key_col_unresolved`（解析欠落・コル未確定）は**従来どおり停止**を維持

## 作業フロー

1. **ISSUE 登録**（仕様議論のため）: `venv/bin/python3 mgmt/tracker/track.py issue add`（type=設計、actor=モデル名）→ 着手時 `--status 対応中`
2. **新規 ADR 作成**: `docs/decisions/ADR-SRS-037-unmatched-summit-needs-review.md`（次の SRS 連番を採番前に要確認）
   - Context: 地形変化で消滅したサミットの盲点・座標では不備と区別不能
   - Decision: 要確認続行＋件数しきい値停止＋申請書非自動掲載
   - Alternatives: 自動削除候補化（誤削除リスクで却下）／現状維持＝即停止（正当変化でも停止で却下）／常時続行・しきい値なし（解析バグ見逃しで却下）
   - Consequences: 影響文書・パラメータ追加を列挙
3. **既存文書の同期更新**（片方だけ修正禁止の整合原則）:
   - `docs/decisions/ADR-SRS-011-delete-zone-polygon.md`: 「本来発生しないべき・停止」記述を「要確認・続行（しきい値超過時のみ停止）」へ修正。`unmatched` 行・末尾「未確定事項」を更新
   - `docs/decisions/ADR-URD-007-peak-match-status-terminology.md`: summit.match_status テーブルの `unmatched` 説明を「エラー停止」→「要確認（しきい値超過で停止）」へ
   - `docs/20_SRS.md`:
     - `FR-009`（L867 付近）: `unmatched` の処理を「要確認続行＋しきい値停止」に書き換え。`is_unmatched_summit` の役割変更。L888-891・L921 付近の停止記述を修正
     - データ辞書 2.2.1（L125 付近）: `unmatched 件数しきい値` パラメータを追加
   - `docs/decisions/ADR-SRS-033-defect-confirmation-via-xlsx.md`: 要確認サミットの `merged_summit.xlsx` 表示を追記
   - `docs/decisions/ADR-SRS-035-viewer-category-filter-feature-mapping.md`: 「要確認」カテゴリをフィルタ対応表に追加
   - `docs/00_GLOSSARY.md`: 「要確認サミット」用語を追加・`unmatched` 説明を更新
4. **新規 ADR には URD/SRS から相互リンク**（参照ルール）
5. **コミット**: ドキュメント更新ターン内に `make lint` 警告ゼロ確認 → Conventional Commits（日本語本文）でコミット（push は別途指示まで不要）
6. **ISSUE クローズ** → ユーザー verify

## 注意

- コードは未実装フェーズ（`ADR-SRS-011` が採用・未実装）のため、本変更の成果物は **SRS/ADR の文書のみ**。`.c`/`.py` の編集は発生しない
- パラメータ名・しきい値の既定値は ADR 内で命名・暫定値を提案し、ユーザー確認を取る（実測根拠がない既定値は「暫定」と明示）

## 検証

- `make lint`（lint-md / pymarkdown / `scripts/lint_docs.py`）で警告ゼロ
- `grep` で旧記述（`unmatched` の「停止」「本来発生しないべき」表現）の残存ゼロを確認
- ADR 相互参照リンクの切れがないこと（`grep` でアンカー確認）
- 5 文書（ADR-SRS-011 / ADR-URD-007 / SRS FR-009・データ辞書 / ADR-SRS-033 / ADR-SRS-035 / GLOSSARY）の記述が新方針で矛盾しないこと
