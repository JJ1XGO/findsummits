# 計画: 申請カテゴリのプロパティ化と FR-009 フィーチャ構成表のサミット中心再編

## Context（なぜやるか）

FR-009 の「フィーチャ構成（match_status 別）」表は、実体は `merged_summit.geojson` の**ジオメトリ在庫表**（各分類にどの Point/Polygon/LineString が何個あるか）で、幾何がピーク周りに群がるため **peak.match_status 軸**で書かれている。しかし:

- 消費側は既にサミットアクション軸で再グルーピングしている。FR-011 申請書 XLSX は**アクション別**（追加/削除/変更）、FR-019/FR-020 ビューアは**カテゴリ別**（new/dominant/changed/unchanged、`ADR-SRS-035` の実行時導出）。
- 同一データに分類軸が3つあり、FR-009 の表だけがピーク軸の在庫表で浮いている。表の見出しが「match_status 別」とだけ書かれ、peak 値（new/dominant）と summit 値（unmatched）を黙って混在させていて人間可読でない。
- FR-009 はピーク中心→サミット中心への転回点であり、ここで中心データを**サミット中心の申請カテゴリ**で表現するのが自然。

ユーザー決定（本セッションの Q&A で確定）:

1. **データ構造も見直す** — 申請カテゴリを GeoJSON の**格納プロパティ `category`** にし、生成者 FR-009 が算出して各フィーチャに刻む（消費側の導出ロジックを廃し、スキーマ正本＝FR-009 の建前と整合）。
2. **削除を独立カテゴリに昇格** — delete サミットを親ピーククラスタから切り出す（1クラスタが2カテゴリに跨ることを許容）。
3. **追加に統合（5カテゴリ）** — new と dominant をともに「追加」に。dominant の副作用（削除）は紐づく削除サミット側で表現。

## 確定設計

### 申請カテゴリ（5分類・サミット中心）

格納値は既存 `match_status`/`feature_type` と同様 English キー、表示ラベルは日本語（要確認なら調整可）:

| `category`（格納） | 表示ラベル | 主語 | 由来条件 |
|---|---|---|---|
| `add` | 追加 | 新設サミット | `peak.match_status` ∈ {new, dominant} |
| `band_change` | 変更あり | 既存サミット | `peak.match_status="matched"` ∧ `is_band_change_candidate=true` |
| `no_change` | 変更なし | 既存サミット | `peak.match_status="matched"` ∧ `is_band_change_candidate=false` |
| `delete` | 削除 | 既存サミット | `summit.match_status="delete"` |
| `review` | 要確認 | 既存サミット | `summit.match_status="unmatched"` |

※ `match_status`・`feature_type`・`is_band_change_candidate` は**廃止せず存続**。`category` はそれらから算出する直交プロパティ。

### per-feature の `category` 割り当てルール

全フィーチャに `category` を付与する（ビューアは各フィーチャの値を読むだけで導出不要に）:

- peak: 上表の由来条件で `add` / `band_change` / `no_change`
- key_col・activation_zone・delete_zone・LineString（peak→col）: **親ピークの category を継承**
- matched summit（AZ 内存続）＋ LineString（peak→matched summit）: 親ピークを継承（`band_change`/`no_change`）
- delete summit ＋ LineString（親ピーク→delete summit）: `delete`（親が dominant でも matched でも。`ADR-SRS-043`）
- unmatched summit: `review`

帰結: dominant/matched ピーク本体は `add`/`band_change`/`no_change`、その従属 delete サミットは `delete` となり、山塊クラスタが2カテゴリに跨る（合意済み）。

### フィーチャ構成表（再編後・追加と削除の分離で簡潔化）

delete サミットを `delete` へ外出しした結果、`add` は new/dominant とも同一構成になる:

- `add`: Point(ピーク)＋Point(コル)＋Polygon(AZ)＋Polygon(delete判定ゾーン)＋LineString(ピーク→コル)
- `band_change`/`no_change`: 上記＋Point(AZ 内存続 SOTA サミット)＋LineString(ピーク→AZ 内サミット)
- `delete`: Point(delete サミット)＋LineString(親ピーク→delete サミット)。親ピークは `add`/`band_change`/`no_change` 行に存在
- `review`: Point(孤立既存サミット)のみ

### サミット一覧 XLSX（FR-012 列）への反映

`merged_summit.xlsx`（6.2.7・FR-009 生成）と `merged_summit_revised.xlsx`（6.2.3・FR-012 生成）は**列定義を `FR-012` で共用**し、「Point 1個＝1行」のサミット中心一覧。現状その列に `match_status`（`20_SRS.md:1280`、ピーク行 matched/new/dominant・既存サミット行 matched/delete/unmatched）があり、GeoJSON 表と同型の muddiness を抱える。

- **`category` 列を追加**（各行が 追加/変更あり/変更なし/削除/要確認 を直読可能に）。
- **`match_status` 列は残す**（6.2.7 が不備調査で `match_status=unmatched`・`area_complete=false`・`key_col_resolved=false` を per-row 参照する用途を明記）。
- XLSX は元々ピーク行と既存サミット行が別行なので、削除独立とも自然に整合。

### 申請エビデンス ZIP（FR-021 分割）の再編

`FR-021` の ZIP 分割（`20_SRS.md:1333-1337`）も旧 match_status ベースの4ファイル（new/dominant/changed/unchanged.geojson、delete は dominant.geojson に同梱）。これを **category ベースに再編（削除独立）** する:

- `add.geojson` / `changed.geojson` / `unchanged.geojson` / `delete.geojson`（delete サミット＋親ピーク→delete サミット LineString を独立ファイル化）。
- `review.geojson`（unmatched）の同梱可否は ADR で決める（unchanged も「申請対象外だが参照用に同梱」しているため、要確認も同梱が一貫しうる）。
- 各ファイルの関連フィーチャ（col/AZ/delete_zone/prominence_range/coord_diff）の同梱ルール（`summit_code` 紐付け）は現行踏襲。

## タスク（各タスクに実行モデルを明記）

0. **issue 起票**（type=設計、actor=モデル名）: 「申請カテゴリをプロパティ化し FR-009 フィーチャ構成表をサミット中心5分類へ再編」。着手時 `--status 対応中`。— プロセス
1. **`docs/decisions/ADR-SRS-044-*.md` 新規作成** — Sonnet
   - 決定: `category` プロパティ化＋サミット中心5分類＋per-feature 割り当てルール＋削除独立。`ADR-SRS-035` を supersede。採番・フォーマットは `docs/CLAUDE.md` に従う。
2. **`docs/decisions/ADR-SRS-035-*.md` を「廃止（superseded by ADR-SRS-044）」に更新** — Sonnet
3. **`docs/20_SRS.md` FR-009 改訂** — Sonnet（相互参照に注意）
   - `category` 算出仕様（5分類・by-feature ルール）を追記。フィーチャ構成表を「申請カテゴリ別」に再編（上記）。見出し・アンカー「match_status 判定」は維持しつつ表の軸を変更。
4. **`docs/20_SRS.md` FR-011 改訂** — Sonnet
   - アクション別カラムマッピング表は維持。各アクションのデータソース選択を `match_status` 由来から `category` ベースへ（追加←`add`、削除←`delete`、変更←`band_change`）。`no_change`/`review` は XLSX 行なし。
5. **`docs/20_SRS.md` FR-012 改訂** — Sonnet
   - 出力カラム表に `category` 列を追加（`match_status` 列は残す）。merged_summit.xlsx（6.2.7）・merged_summit_revised.xlsx（6.2.3）は本列定義を共用するため両方に効く。
6. **`docs/20_SRS.md` FR-013 改訂** — Sonnet
   - スキーマ参照文言「フィーチャ構成（match_status 別）」→「申請カテゴリ別」。
7. **`docs/20_SRS.md` FR-019 改訂** — Sonnet
   - カテゴリ別フィルターを 4→5 分類（`add`/`band_change`/`no_change`/`delete`/`review`、表示は 追加/変更あり/変更なし/削除/要確認）。各フィーチャの格納 `category` を読む形に変更（導出表は撤去し `ADR-SRS-044` 参照）。配色・「検索確定時フィルタ自動 ON」記述を5分類に更新。
8. **`docs/20_SRS.md` FR-020 確認** — Sonnet
   - フィルターは FR-019 を参照継承（line 1109）。ハードコードの旧4分類記述が無いか確認、あれば追従。
9. **`docs/20_SRS.md` FR-021 改訂** — Sonnet
   - ZIP 分割を category ベースに再編（add/changed/unchanged/delete.geojson、削除独立。review.geojson 同梱可否は `ADR-SRS-044` の決定に従う）。関連フィーチャ同梱の `summit_code` 紐付けは現行踏襲。
10. **`docs/00_GLOSSARY.md` に「申請カテゴリ」用語追加** — Sonnet
11. **検証・コミット** — Sonnet（下記）
12. **(follow-up・別タスク)** `docs/mockup/viewer_mockup.html` を5分類に更新して視覚検証 — Sonnet

## スコープ外（仕様確定後の別タスク）

- 実装（`merge.py` の `category` 算出・ビューア JS のフィルタ）。本計画は SRS/ADR の仕様確定までを範囲とする（プロジェクトの仕様優先・スキーマ正本＝FR-009 の方針に従い、コードは仕様ロック後に追従）。

## 検証

- `make lint` 警告ゼロ（`lint-md` 中心。ドキュメントのみの変更）。
- ADR/SRS 相互リンクの健全性（`ADR-SRS-044`↔FR-009/011/019、`ADR-SRS-035` の supersede 表記、`ADR-SRS-043` との整合）。
- マッピングの網羅性・排他性チェック: 全 `match_status`×`feature_type`×`is_band_change_candidate` の組合せが必ず1つの `category` に落ち、重複しないことを机上確認。
- XLSX アクション対応の一致確認: `add`→追加 / `delete`→削除 / `band_change`→変更 / `no_change`・`review`→行なし、が FR-011 と矛盾しないこと。
- 成果物横断の `category` 一貫性: FR-009（GeoJSON プロパティ）・FR-012（XLSX 列）・FR-021（ZIP ファイル分割）・FR-019/020（ビューアフィルタ）が同一の5カテゴリ定義を参照し、旧4分類（new/dominant/changed/unchanged）の記述が残存しないこと。
- ドキュメント更新ターン内に commit（push は別途指示まで不要）。

## 実行モデル

design は本セッションで確定済みのため、残作業は相互参照に注意した文書編集が中心。**推奨: Sonnet**。承認後・編集着手前に `/model` 切替を促す。`.claude/plans/` の本ファイルは承認後 `mgmt/plan.md` へ `mv` する。
