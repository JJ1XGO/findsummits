# 計画: FR-021 spec-panel 指摘対応

## Context

spec-panel で FR-021（申請エビデンス ZIP 生成）をレビューした結果、高指摘 2 件・中指摘 4 件・低指摘 1 件が検出された。
ユーザーが方針を確定済みのため、仕様議論は不要。全 7 件を文書修正として実施する。

主な変更:

- UR-005 の「CSV」→「XLSX」（ユーザーが途中で方針転換したが URD 修正が漏れていた）
- FR-021 入力テーブルの localStorage 必須/任意を FR-020 と整合させる
- 用語・記述の揺れを 5 箇所修正

## 対象ファイル

- `docs/10_URD.md` — UR-005 文言修正（指摘②）
- `docs/20_SRS.md` — FR-021 本体 + §6.18 + NFR の 6 箇所修正（指摘①③④⑤⑥⑦）
- `docs/decisions/ADR-URD-018-evidence-format-csv-to-xlsx.md` — 新規作成（指摘②の ADR）
- `mgmt/todo.md` — 7 件追記（完了後に削除）

## タスク一覧

### タスク 0: todo.md 追記（モデル: Sonnet）

`mgmt/todo.md` に指摘①〜⑦を追記する（完了後に順次削除）。

---

### タスク 1: ADR-URD-018 作成（モデル: Sonnet）

`docs/decisions/ADR-URD-018-evidence-format-csv-to-xlsx.md` を新規作成。

```markdown
| 状態 | 採用・未実装 |
| 決定日 | 2026-06-28 |

## Context

UR-005 はエビデンスの出力形式として「CSV」を規定していたが、
実装設計が進む中でサミット一覧（申請内容反映版）XLSX（FR-012）＋
カテゴリ別 GeoJSON 4 件（FR-021 ZIP 同梱）で同等以上の情報をカバー
できることが判明し、方針を XLSX+GeoJSON に転換した。
URD の修正が漏れていたため、本 ADR で根拠を記録しつつ URD を是正する。

## Decision

UR-005 の出力形式を CSV から「サミット一覧（申請内容反映版）XLSX
（申請エビデンス ZIP に同梱）」に変更する。

## Alternatives

- CSV を維持する: CSV は機械可読性が高いが、ユーザーが直接確認するには
  XLSX の方が視認性が高く、列名・書式が整っている。GeoJSON で座標情報も
  含むため情報量として CSV に劣らない。

## Consequences

- UR-005 の文言を XLSX ベースに更新する
- FR-012・FR-021 はすでに XLSX+GeoJSON で実装予定のため、コード変更不要
```

---

### タスク 2: UR-005 修正（モデル: Sonnet）

`docs/10_URD.md` の UR-005 行を修正。

**変更前**（UR-005 L.45）:

```text
| UR-005 | 申請内容のエビデンスとなるCSV（突合結果・サミット座標・標高・Keyコル座標・標高・プロミネンス）を生成できること |
```

**変更後**:

```text
| UR-005 | 申請内容のエビデンスとなるサミット一覧（申請内容反映版）XLSX（突合結果・サミット座標・標高・Keyコル座標・標高・プロミネンスを含む）を申請エビデンス ZIP に同梱して出力できること（根拠: ADR-URD-018） |
```

---

### タスク 3: 指摘①〜⑦ を SRS に反映（モデル: Sonnet）

`docs/20_SRS.md` の 6 箇所を修正。

#### 指摘①: localStorage 必須→任意（FR-021 入力テーブル L.1298）

`localStorage 編集内容` 行を以下に変更:

- 必須/任意: `必須` → `任意`
- デフォルト（任意時）: `—` → `初期値（FR-009 自動生成テンプレート）`
- 備考: 現行のまま

#### 指摘③: 用語統一（FR-021 同梱ファイル一覧 L.1314）

`dominant.geojson` の内容欄:

- 変更前: `削除候補ピークおよびその従属サミット`
- 変更後: `差替候補ピークおよびその従属サミット`

#### 指摘⑤: localStorage マージ方式の補足（FR-021 説明 L.1320）

現行:

```text
GeoJSON の生成は localStorage の編集内容（山岳名JP/EN・rationale 編集値）を埋め込みデータにマージしたうえで行う
```

変更後:

```text
GeoJSON の生成は localStorage の編集内容（山岳名JP/EN・rationale 編集値）を埋め込みデータにマージしたうえで行う（localStorage を直接読むのではなく、FR-019 の引き継ぎ確認を経た現在の編集状態のスナップショットを使用する。FR-020 と同方式）
```

#### 指摘⑥: unchanged.geojson の同梱根拠（FR-021 説明欄への追記）

同梱ファイル一覧の箇条書き末尾（L.1321 の後）に追記:

> `unchanged.geojson` は申請対象外だが、SOTA 日本支部担当者が現行サミット全件の状態をエビデンスとして確認できるよう同梱する

#### 指摘④: §6.18 ZIP ファイル名命名規則（L.1552）

現行:

```text
| ファイル名 | （HLD で確定） |
```

変更後:

```text
| ファイル名 | `sota_evidence_YYYYMMDD.zip`（`YYYYMMDD` は `metadata.generated_at` の日付部分。完全仕様は HLD に委ねる） |
```

#### 指摘⑦: NFR に ZIP サイズ言及（NFR-002 末尾に追記）

NFR-002（メモリ使用量）の末尾に追記:

> - ブラウザ内 ZIP 生成（FR-021）: unchanged.geojson を含む全日本解析の ZIP サイズ目安は未測定（実装後に計測して HLD に反映する）

---

### タスク 4: make lint + commit（モデル: Sonnet）

```bash
make lint   # 警告ゼロを確認
```

確認後、変更ファイルをまとめてコミット:

```text
docs(srs/urd): FR-021 spec-panel 指摘①〜⑦ 対応（UR-005 CSV→XLSX・localStorage 任意化・用語統一他）
```

## 検証手順

1. `make lint` 警告ゼロ（lint-md / lint-py / lint-geojson / lint-html）
2. `grep -n "CSV.*エビデンス\|エビデンス.*CSV" docs/10_URD.md` → 旧表現が残っていないこと
3. `grep -n "削除候補ピーク" docs/20_SRS.md` → FR-021 箇所が「差替候補ピーク」に変わっていること
4. FR-021 入力テーブルの localStorage 行が「任意」になっていること（目視）

## モデル推奨

全タスクとも文書の文言修正のみ。**Sonnet 4.6** で十分。
（既に Sonnet 4.6 で作業中なので `/model` 切替不要）
