# merged.csv 命名整理（FR-008 / FR-009 / FR-012）

## Context

SRS § 4.3〜4.4 において、`merged.csv` という同一ファイル名が **2 か所** で別の用途・別の FR から出力される構造になっており、データフロー読解の障害になっている：

| 行 | 出力 FR | 用途 |
|---|---|---|
| `docs/02_SRS.md:368` | FR-008 | 内部 work CSV（per-mesh 統合・プロミネンス 150m フィルタ後） |
| `docs/02_SRS.md:720` | FR-012 | 派生エビデンス CSV（`merged.geojson` から派生生成） |

さらに、FR-009（SOTA 突合・match_status 判定）はデータ概念が「ピーク中心 → サミット中心」に切り替わる重要な節目にも関わらず、メイン出力 `merged.geojson` は人間が中身を素早く確認しづらく、サミット一覧として可視化できる CSV が欲しいというニーズがある。

本タスクではファイル名の重複を解消し、データ概念の切り替わりを明示する命名体系に SRS を改訂する。

## Scope

- **含む**: SRS（`docs/02_SRS.md`）の命名・俯瞰図・FR 出力欄の改訂
- **含まない**:
  - 実装側修正（`scripts/merge.py` / `scripts/output_geojson.py`）→ 別 ISSUE で起票
  - HLD / LLD の更新（未着手）
  - **フェーズ5 関連（FR-011 / FR-012 / FR-019 / FR-020）の構造詰め**：未レビューのため、FR-012 出力ファイル名の確定はフェーズ5 レビュー時に持ち越し

## 命名整理（決定版）

| ファイル | 旧名 | 新名 | 出力 FR | 内容 |
|---|---|---|---|---|
| 内部 work CSV（peak 中心） | `merged.csv` | **`merged_peak.csv`** | FR-008 | per-mesh 統合・プロミネンス 150m フィルタ済み。FR-009 入力 |
| 中心成果物 | `merged.geojson` | （維持） | FR-009 | SOTA 突合済み・全フィーチャ・rationale |
| サミット一覧 CSV（中身確認用） | （新規追加） | **`merged_summit.csv`** | FR-009 | バッチ生成時点（ユーザー編集前）のサミット一覧。カラムは FR-012 派生エビデンス CSV と同じ |
| 派生エビデンス CSV（編集反映後） | `merged.csv` | **未確定（保留）** | FR-012 | FR-019 でユーザーが編集した山岳名・rationale を反映したエビデンス CSV。フェーズ5 レビュー時に名称確定 |
| activation zone GeoJSON | `merged_activation.geojson` | （維持） | FR-018 | 内部中間ファイル |
| ローカル HTML ビューア | `merged_viewer.html` | （維持） | FR-013 | GeoJSON 埋め込み・編集可能 |

### 命名軸の整理

- **`merged_<種別>.<拡張子>`** で統一（既存 `merged_activation.geojson` と同じパターン）
- **`peak`（ピーク中心）vs `summit`（サミット中心）** で FR-008 と FR-009 の概念切り替わりを明示
  - SOTA 業界の用語感（peak = 山頂候補、summit = SOTA 申請対象）とも一致

### FR-012 出力名が保留である理由

- 現状 SRS § 4.4 FR-012（行 722）では「`merged.csv` は純粋なバッチ解析結果であり、HTML ビューアでのユーザー入力は反映しない」と明記
- 一方、ユーザー意図は「FR-019 でユーザーが入力した山岳名を使って FR-012 がエビデンス CSV を生成する」 → FR-009 出力の `merged_summit.csv` と中身が変わる
- この責務再整理はフェーズ5 全体（FR-011 / FR-012 / FR-019 / FR-020）のレビューに含めるべきであり、本タスクのスコープ外
- 本タスクでは「FR-012 出力名は変更が必要」「`merged.csv` 名は廃止」とだけ確定し、新名はフェーズ5 レビュー時に決定する

## 修正対象ファイル（SRS）

### `docs/02_SRS.md`

| 行（概算） | 変更内容 |
|---|---|
| 7 | ヘッダーステータス欄の補足文言確認 |
| 38 | （目次：変更なし。FR 番号は維持） |
| 120 | コンポーネント表「統合・突合コンポーネント」出力欄に `merged_summit.csv` 追加。`merged.geojson` 表記維持 |
| 144 | フェーズ3 俯瞰図：`merged.csv` → `merged_peak.csv` |
| 149-150 | フェーズ4 俯瞰図：`merged.geojson` 維持、`merged.csv`（派生エビデンス）は **TBD（フェーズ5 レビュー後確定）** マーカー。新たに FR-009 出力として `merged_summit.csv` を追加 |
| 156 | エビデンス CSV のパス表記を TBD マーカーに置換 |
| 267 | FR-009 参照テキスト：影響なし |
| 308-309 | FR-018 → FR-009 のデータフロー記述：CSV 言及なし、変更不要 |
| 363-378 | **FR-008 出力**: `merged.csv` → `merged_peak.csv` に変更。詳細欄の「再入可能性」記述も追従 |
| 385-389 | FR-018 出力（`merged_activation.geojson`）：変更なし |
| 395-425 | **FR-014**: `merged.csv` → `merged_peak.csv` への参照修正（複数箇所） |
| 441-451 | **FR-009 出力**: `merged.geojson`（中心成果物）に加えて `merged_summit.csv`（サミット一覧 CSV、中身確認用）を追加。「ピーク中心 → サミット中心への切り替わり」を概要文に明記 |
| 446 | 入力欄：`merged.csv` → `merged_peak.csv` |
| 457 | 「`municipality` カラムを merged.csv に付与」→ `merged_summit.csv` に付与 |
| 514-520 | 不備フラグ・exit code 制御：CSV 名変更なし（merged.geojson 維持） |
| 533-534 | FR-010：CSV 名変更なし |
| 543-552 | FR-013：`merged.geojson` 維持 |
| 577, 601 | FR-013 内 `rationale` プロパティ説明：変更なし |
| 644 | フェーズ5 説明：変更なし（FR-012 出力名は TBD のため触らない） |
| 649-672 | FR-019：変更なし |
| 694, 710-713 | FR-011 ※ 注釈：変更なし |
| **718-722** | **FR-012**: 出力欄の `merged.csv` を **TBD マーカー**（例: `<TBD: フェーズ5 レビュー後確定>`）に置換。概要文と「純粋なバッチ解析結果」記述は触らない（責務再整理はフェーズ5 レビュー時） |
| 720 | （上記と同じ箇所） |
| 794 | 仮サミットコード採番説明：変更なし |
| 874-895 | § 6.x 成果物一覧表：エビデンス CSV ファイル名を TBD マーカーに更新。中心成果物 `merged.geojson` の生成元説明は維持 |
| 887-888 | 中心成果物テーブル：FR-009 出力に `merged_summit.csv` を補記 |
| 894-900 | 作業用 HTML ビューア：変更なし |
| 946, 959, 982 | 用途欄：FR-009 引用なので CSV 名変更追従不要 |
| 994-1008 | merged_activation.geojson / 公開用 HTML：変更なし |

### 検証用 grep

改訂完了後、以下を実行して残存を確認：

```bash
# 単独 merged.csv のみを検出（merged_peak/merged_summit には誤マッチさせない）
grep -nE "(^|[^_a-z])merged\.csv" docs/02_SRS.md  # 期待: TBD マーカー周辺のみ（FR-012 の 1〜2 箇所）
grep -n  "merged_peak\.csv"   docs/02_SRS.md       # 期待: FR-008 / FR-014 / FR-009 入力欄 / 俯瞰図
grep -n  "merged_summit\.csv" docs/02_SRS.md       # 期待: FR-009 出力 / コンポーネント表 / 成果物一覧 / 行 457 市区町村カラム付与先
grep -n  "merged\.geojson"    docs/02_SRS.md       # 期待: 変更なし（既存件数維持）
```

**行 457（市区町村カラム付与先）の注意**: 現状「`municipality` カラムを merged.csv に付与」と書かれているが、これは FR-009 が直接書き戻すのではなく FR-009 出力の `merged_summit.csv` に付与される、と読み替える必要がある。改訂時に明示的に `merged_summit.csv` に書き換えること。

## 後続 ISSUE（本タスクでは起票のみ・実装着手しない）

1. **実装側ファイル名追従**: `scripts/merge.py:50` の `DEFAULT_OUTPUT` を `merged_peak.csv` に変更、`scripts/output_geojson.py:34` の `DEFAULT_INPUT` を `merged_peak.csv` に変更。FR-009 から `merged_summit.csv` 出力ロジック新規追加
2. **HLD / LLD の整合追従**: HLD / LLD 着手時に新命名で記述
3. **フェーズ5 全体レビュー**: FR-011 / FR-012 / FR-019 / FR-020 の責務・データフロー整理（FR-012 の出力名・編集反映タイミング含む）
4. **CLAUDE.md / README.md のドキュメント追従**: `merged.csv` 言及箇所（CLAUDE.md L63 ほか）の更新

## 検証

SRS 改訂後の最終確認:

1. **grep 検証**（上記）で残存箇所が意図通りであること
2. **俯瞰図整合性**: § 3.3 ASCII 俯瞰図が、フェーズ3 → フェーズ4 のファイル名遷移を正しく示している
   - フェーズ3 末尾に `merged_peak.csv` が現れる
   - フェーズ4 で FR-009 出力として `merged.geojson` + `merged_summit.csv` が現れる
   - FR-012 出力は TBD マーカー
3. **FR-009 概要文**: 「ピーク中心 → サミット中心」の概念切り替わりが言語化されている
4. **FR-012 整合**: 「`merged.csv` という具体名」と「`merged_summit.csv` との混同」が解消されている（責務記述自体はフェーズ5 まで未変更）
5. **ISSUE-051 タイトル修正**（既存 todo）も併せて反映（「SRS フェーズ3.5:」→「SRS フェーズ3（FR-014）:」）

## 留意点

- **仕様優先原則** に従い、SRS のみ改訂。実装変更は別 ISSUE で扱う
- **コミット粒度**: 「SRS merged ファイル命名整理」の単一コミット。ISSUE-051 タイトル修正は別コミット
- **plan.md（mgmt/plan.md）と トラッカー** にも本タスクを反映（ISSUE 新規起票）
- **TBD マーカーの記法**: SRS 内で曖昧さを許容しない方針との整合を保つため、`<TBD: フェーズ5 レビュー後確定（ISSUE-XXX）>` のように **後続 ISSUE 番号を明記する** 形でプレースホルダーを置く
