# 計画: シナリオD で matched ピークを Key にした削除申請を自動生成する（ADR-042 見直し）

## Context

このプロジェクトの主成果物は **削除申請（XLSX）**。SOTA 既存サミット Y を「どのピークに従属しているか」付きで削除申請に自動掲載することが存在意義の中核。

ところが昨日決定した `ADR-SRS-042`（matched ピークを削除候補サミットの主ピーク候補から除外）は、次のシナリオD で**この主成果物を取りこぼす**ことが判明した。

- ピーク A の AZ 内に既存サミット X → `peak.match_status = matched`（X は正常存続）
- 同じ A の delete判定ゾーン内（AZ 外）に既存サミット Y、Y はどのピークの AZ にも入らない
- ADR-042 では「A は matched なので Y の主ピーク候補から除外」→ Y が `unmatched`（要確認）に降格 → `dominant_peak_code`/`dominant_peak_dist_m` が付かず、**申請書「削除」行に自動掲載されない**（`docs/20_SRS.md` line 886・909・911）

このシナリオD は理論ではなく、過去の九州・四国解析で**実際に1件発生済み**（事前データ: D=1件 / 「AZ0+delete複数」=0件）。`ADR-SRS-042` は viewer 上の `A→Y` 接続線の置き場の曖昧さを避けるために、**XLSX 削除申請に必要なデータ link ごと捨てた過剰補正**だった。

なお `docs/20_SRS.md` line 885 は元々「delete判定ゾーン内・AZ 外の既存サミット = `delete`」と定義しており、ADR-042（line 909/911）がこれを `unmatched` に上書きしている。本修正はその上書きを撤回し、line 885 本来の挙動へ戻すもの。

**意図する結果**: シナリオD でも Y を `delete` として扱い、`dominant_peak_code = matched ピーク A の既存 SOTA コード`を付与し、削除申請を自動生成する。A は `matched` のまま（X は正常存続）。

## 設計方針（軽量修正）

案②（matched→dominant 昇格＝存続/削除のステータス複合）は採らない。A のステータスは単一（`matched`）に保ち、Y を独立した `delete` エンティティとして表現することで、案②の複合フィーチャ仕様整理を回避する。

- delete候補サミットの主ピーク特定で **matched ピークを候補から除外しない**（ADR-042 の除外を撤回）
- 主ピークが matched の場合、`dominant_peak_code` は当該 matched ピークの**既存 SOTA コード**（X のコード）
- `unmatched` は「全 AZ・全 delete判定ゾーンのいずれにも含まれない真の孤立サミット」のみに戻す
- viewer の `A→Y` 接続線は、matched フィーチャ構成に「従属 delete サミット Point + ピーク→サミット LineString（複数可）」を追加して定義する。X（AZ 存続サミット）と Y（従属 delete サミット）は別 Feature・別 `summit.match_status` で区別され、概念混在（旧 merge.py status 列の轍）には当たらない

「削除申請の自動生成」という主目的を満たし、案②より波及が小さい。複合構成の汎用的必要性は全国解析の件数を見てから再検討（ADR に明記）。

## タスク

> 全タスク **Sonnet**（仕様文書の編集。設計判断は本計画で確定済み。残作業は確定方針に沿った文章修正と整合確認で、Opus 継続の必要なし）。

### 1. 課題登録（issue）— Sonnet

- 仕様の再決定を伴うため `issue add`（type: 設計）。`venv/bin/python3 mgmt/tracker/track.py issue add`、`--actor` はモデル名（Sonnet）
- スコープ: 「ADR-042 がシナリオD で削除申請データを取りこぼす問題の再決定（ADR 改訂 + SRS 反映）」

### 2. 新規 `ADR-SRS-043`（ADR-042 を supersede）— Sonnet

- 採番は `ls docs/decisions/ | grep ADR-SRS` で最大値を確認して +1（想定 043、要確認）
- ファイル: `docs/decisions/ADR-SRS-043-<kebab>.md`（例: `matched-peak-as-dominant-reference-for-delete`）
- 状態: `採用・未実装`
- 内容:
  - Context: シナリオD で削除申請を取りこぼす問題（主成果物の欠落）。九州・四国の事前データ（D=1 / AZ0+複数=0）を根拠として記載
  - Decision: 上記「設計方針（軽量修正）」
  - Alternatives: ①案②（matched→dominant 昇格・複合構成。重く汎用的だが本軽量案で要件充足のため不採用、全国解析後に再検討）/ ②ADR-042 維持（主目的を取りこぼすため不採用）
  - Consequences: シナリオD で削除申請が自動生成される / `unmatched` は真の孤立のみに戻る / `ADR-SRS-042` は supersede

### 3. `ADR-SRS-042` を supersede 状態に更新 — Sonnet

- `docs/decisions/ADR-SRS-042-matched-peak-excluded-from-dominant-candidate.md`
- 状態欄を `却下・ADR-SRS-043 により supersede` に変更し、冒頭に supersede 注記＋ADR-043 へのリンクを追加（決定経緯は残す）

### 4. `docs/20_SRS.md` FR-009 反映 — Sonnet

- **主ピーク特定（line 906-909）**: 「ただし `match_status=matched` のピークは候補から除外する（ADR-042）…unmatched として扱う」を削除。「matched ピークも主ピーク候補に含める。主ピークが matched の場合 `dominant_peak_code` は当該ピークの既存 SOTA コードとする」に書き換え。参照を ADR-043 に張り替え
- **line 911**: 「matched を除外した結果として候補なしになった場合を含む」を削除。`unmatched` を line 886 の定義（全ゾーン外）に統一
- **line 912**: `dominant_peak_code` 説明に「主ピークが matched の場合は既存 SOTA コード」を補足
- **※4 削除根拠（line 925-928）**: `{dominant_peak_code}` が既存コードになりうる旨を注記（フォーマット自体は変更不要）
- **フィーチャ構成 matched 行（line 964）**: 「＋ 従属 delete サミット Point ＋ LineString（ピーク→サミット、当該 delete判定ゾーンの削除候補数だけ・複数可）」を追加。X（AZ 存続）と Y（従属 delete）の区別を注記
- **line 885/886 整合確認**: 修正後に line 885（delete 定義）と矛盾しないことを確認

### 5. 波及確認（grep + 関連 FR）— Sonnet

- `grep -rn "ADR-SRS-042" docs/` で残存参照を洗い、撤回した内容を指すものを ADR-043 に整理（または削除）
- `FR-011`（申請書 XLSX 生成）が delete サミットを自動で削除行に出すこと、matched 由来の除外を持たないことを確認（持てば追従）
- `FR-019`（HTML ビューア機能仕様）が matched フィーチャに従属 delete サミット/LineString が増えても破綻しないことを確認（必要なら一文追記）

### 5b. todo.md への分解（実装タスク）— Sonnet

- 本件は ADR-042 が「採用・未実装」段階での仕様変更。FR-009 実装コードはまだ存在しないため、実装は将来の FR-009 実装時に追従する。実装観点（主ピーク特定で matched 除外しない／matched の従属 delete サミット feature 生成）を `mgmt/todo.md` に追記し取りこぼしを防ぐ

### 6. lint・コミット — Sonnet

- `make lint`（特に `lint-md`）警告ゼロを確認
- ドキュメント更新ルールに従い**当ターン内に commit**（Conventional Commits、本文日本語）。push は別途指示まで不要
- `mgmt/plan.md` への移動: 本計画は承認後 `.claude/plans/` から `mgmt/plan.md` へ `mv`（CLAUDE.md 規約）

## 検証

- **整合**: `grep -rn "ADR-SRS-042" docs/` で撤回済み内容への参照が残っていないこと。ADR-043 と FR-009 の用語・参照リンクが相互に整合していること
- **シナリオD トレース（机上）**: 「A=matched(X in AZ) / Y in A.delete_zone, AZ外 / Y は他 AZ になし」で、`summit.match_status(Y)=delete`・`dominant_peak_code(Y)=X の既存コード`・`peak.match_status(A)=matched` になることを SRS 記述で追えること。`unmatched` は「全ゾーン外」のみであること
- **lint**: `make lint` 警告ゼロ
- **コード**: 本件は仕様フェーズ（FR-009 未実装）。コード変更・`make`・解析実行は不要。実装追従は todo.md に記録
- **ドキュメント整合性原則**: ADR-042 の supersede 状態と SRS 記述・他 FR が矛盾しないこと
