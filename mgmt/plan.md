# FR-020 spec-panel レビュー指摘の修正計画

## Context

`/spec-panel FR-020`（公開用 HTML ビューア生成）を 4 視点でレビューし、7 件の指摘を検出した。
最大の問題は **UR-011（国土地理院 帰属表示義務）へのトレーサビリティ欠落**で、公開用 HTML は本ツールが唯一「外部公開」する成果物にもかかわらず、対応 UR・6.13 に attribution の記載が無く、規約違反リスクがある。
本計画は 7 件を tracker（issue＋todo）で管理し、仕様判断を 1 論点ずつ確定したうえで URD/SRS を同期修正することを目的とする。

ユーザー確認済みの方針:

- 対象: 全 7 件
- 内部解析値（「全データ」popup・仮コード等）の露出可否: **後で 1 論点ずつ相談**（ISSUE-2 内で決定）
- tracker: **issue 登録して進める**

## 指摘一覧（7 件）と振り分け

| # | 重要度 | 指摘 | 該当 | 振り分け |
|---|---|---|---|---|
| 1 | 高 | UR-011 帰属表示のトレーサビリティ欠落（対応UR=UR-006のみ・6.13にattribution無し） | SRS:1060, 6.13(1509-1515), URD:50 | ISSUE-1 |
| 2 | 高 | 公開用に含める閲覧機能の範囲が未定義 | FR-020(1078-1082), FR-019(1110-1144) | ISSUE-2 |
| 4 | 中 | metadata 引き継ぎ・基準日表示が未記述 | 6.13, FR-019(1133-1136) | ISSUE-2 |
| 6 | 中 | 内部解析値の外部露出（全データpopup・仮コード） | FR-019(1123) | ISSUE-2（露出ポリシー、要相談） |
| 5 | 中 | localStorage マージ時の generated_at 不一致時の扱い未定義 | FR-020(1079), FR-019(1155-1158) | ISSUE-3 |
| 7 | 低 | 公開用「別テンプレート」採用の判断根拠(ADR)不在 | FR-020(1081) | ISSUE-4 |
| 3 | 中 | 入力「localStorage 編集内容＝必須」の矛盾（未編集でも生成可） | 入力表(1068) | todo（記述修正のみ） |

> 指摘 2・4・6 は「公開用に何を見せ・何を隠すか」という単一の上位論点のため ISSUE-2 に統合（1 論点 1 課題）。
> 指摘 3 は仕様議論を伴わない事実修正のため todo.md へ。

## ステップ 1: tracker 登録（plan 承認後・実行前にこのコマンドを流す）

impersonation 禁止に従い `--reporter "Opus"`、後続の update/close の `--actor` もモデル名。category は SRS 文書作業のため `その他`。

```bash
venv/bin/python3 mgmt/tracker/track.py issue add \
  --title "FR-020 公開用HTMLに UR-011 帰属表示を反映" --priority 高 --type 改善 \
  --stage SRS --category その他 --reporter "Opus" \
  --description "公開用HTML(FR-020)は唯一の外部公開成果物だが対応URがUR-006のみで、6.13にattribution記載が無い。FR-019の帰属表示は作業用ビューア向けで別テンプレートの公開用に引き継がれる保証が無い。" \
  --resolution "FR-020対応URにUR-011追加。6.13にattribution行(© 国土地理院常時＋OSM/OpenTopoMap各表示＋『加工して作成』)明記。UR紐付けのADR要否を判断。"

venv/bin/python3 mgmt/tracker/track.py issue add \
  --title "公開用HTMLの提供機能・公開情報範囲の定義" --priority 高 --type 設計 \
  --stage SRS --category その他 --reporter "Opus" \
  --description "公開用は除外項目(編集UI/XLSX/localStorage)のみ明示で、FR-019の閲覧系機能(背景切替/検索/フィルター/各レイヤー/全データpopup/相互ジャンプ/metadata基準日表示)のどれを含めるか未定義。全データpopupでの内部フラグ・仮コードの外部露出可否も未決(要1論点ずつ相談)。" \
  --resolution "公開用に含める閲覧機能の包含規則をFR-020/6.13に明記。内部値露出ポリシー・metadata基準日表示の要否を順に確定(露出判断はADR候補)。"

venv/bin/python3 mgmt/tracker/track.py issue add \
  --title "公開用エクスポート時の localStorage マージ仕様の明確化" --priority 中 --type 改善 \
  --stage SRS --category その他 --reporter "Opus" \
  --description "FR-020はlocalStorageをマージするとあるが、generated_at不一致(前回実行分)時にどの状態(現在のビューア表示状態 vs localStorage生データ)をマージするか未定義。" \
  --resolution "マージ元を『現在のビューア表示状態のスナップショット』と明記する。"

venv/bin/python3 mgmt/tracker/track.py issue add \
  --title "公開用 別テンプレート採用の設計判断記録(ADR要否)" --priority 低 --type 設計 \
  --stage SRS --category その他 --reporter "Opus" \
  --description "作業用テンプレートを条件分岐させず別ファイルにする設計判断の根拠が未記録。HLDで扱うかADR-SRS化するか未定。" \
  --resolution "HLD所掌か ADR-SRS 化かを判断し、ADR化なら作成。"
```

todo（指摘 3）: `mgmt/todo.md` に 1 行追記。

- `[ ] SRS FR-020 入力テーブル: 「localStorage 編集内容」を必須→任意（デフォルト=初期値テンプレート）に修正`

## ステップ 2: 仕様判断（1 論点ずつ相談 → 確定）

実装（文書編集）前に、以下を順に合意する。ISSUE-2 が判断の塊。

1. ISSUE-2a 公開用に含める閲覧機能の包含規則（推奨: 「FR-019 の閲覧系機能のうち編集・エクスポート・localStorage 系を除く全て」と包含で定義）
2. ISSUE-2b 内部値露出（「全データ」popup・仮コード）の可否 ← **ユーザーと個別相談**
3. ISSUE-2c metadata 引き継ぎ・基準日表示の要否（推奨: 引き継ぎ・表示する。証跡性のため）
4. ISSUE-1 UR-011 紐付けの ADR 要否（既存 UR のリンク追加＝トレーサビリティ修正のため ADR 不要見込み。ただし公開情報範囲＝スコープ決定の ISSUE-2b は ADR 候補）
5. ISSUE-4 別テンプレートの ADR 要否

## ステップ 3: 文書同期修正（各論点確定後にまとめて編集）

対象ファイル:

- `docs/20_SRS.md`
  - FR-020 対応 UR に `UR-011` 追加（行 1060 付近）
  - FR-020 入力表「localStorage 編集内容」必須→任意（行 1068）
  - FR-020 説明: マージ元スナップショット定義・公開機能包含規則・内部値露出方針を追記（行 1078-1082）
  - 6.13 表: attribution 行・metadata/基準日表示行を追加（行 1509-1515）
- `docs/10_URD.md`
  - UR-011 が公開用 HTML を含む旨は既記載。FR-020 からの逆リンク整合のみ確認（行 50）
- 必要時 `docs/decisions/ADR-SRS-0NN-*.md`（ISSUE-2b/ISSUE-4 が ADR 化と判断された場合のみ。採番は `docs/CLAUDE.md` の ADR ルールに従い既存最大+1）

## ステップ 4: 検証・記録

1. `make lint` 警告ゼロ確認（lint-md 等）
2. リンク切れ確認: `grep -n "FR-020\|UR-011\|6.13" docs/20_SRS.md docs/10_URD.md` でアンカー整合
3. 各 issue を `issue close --actor "Opus" --comment "..."`（todo は完了行を削除）
4. ドキュメント更新ターン内に Conventional Commits でコミット（例: `fix(srs): FR-020 に UR-011 帰属表示と公開機能範囲を明記`）。push は別途指示まで不要
5. `mgmt/lessons.md` に学び（公開成果物の attribution トレーサビリティ確認）を必要に応じ追記

## 注意

- 計画ファイルは承認後 `mgmt/plan.md` へ `mv`（プロジェクト規約）。
- 仕様優先原則: コードは参照せず URD/SRS を正とする。
