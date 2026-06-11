# 計画: match_status の意味明確化と dominant_peak_dist_m の復活

## Context

`match_status` の3つの値は、整理すると主語が一貫していない：
- `matched` / `new` … ピーク自身の状態を記述
- `deleted` … ピーク自身ではなく**近傍 SOTA サミットへのアクション**を記述

特に GeoJSON の peak feature において `match_status="deleted"` は「このピーク自身が削除される」と誤読される。実際は「このピークは dominant peak であり、近傍の SOTA サミットが削除候補となる」という意味。

すべての値をピーク中心の状態記述で揃えるため、peak / LineString / CSV の `deleted` を `dominant` にリネームする。SOTA summit feature だけは「サミット自身の運命」を直接表すので `matched` / `deleted` のまま維持する。

加えて、先のコミットで `dominant_peak_dist_m` を「使われていないだろう」と誤判断して削除してしまったが、FR-009 のフォールバック処理（最近接検出ピーク）で内部的に必要な計算値で、証跡 CSV にも残すべき値であることが判明した。復活させる。

## 変更内容

### 0. ADR を 2 件追加

**ADR-URD-007: peak match_status の用語整理**（新規作成）

- ファイル: `docs/decisions/ADR-URD-007-peak-match-status-terminology.md`
- 状態: 採用・未実装
- 決定日: 2026-05-14
- Context: match_status の3値が主語不整合（matched/new は状態記述、deleted はアクション記述）。GeoJSON peak feature で "deleted" が「ピーク自身の削除」と誤読される問題
- Decision: peak / LineString / CSV row の `deleted` を `dominant` にリネーム。SOTA summit feature の `deleted` は維持（サミット自身の運命）
- Alternatives: 現状維持（注釈で説明）／完全分離（peak_status と summit_status に分離）の検討内容を残す
- Consequences: 用語整合性向上、申請書 XLSX 生成時は dominant → 削除アクションにマッピング

**ADR-SRS-008: dominant peak 特定アルゴリズム**（新規作成）

- ファイル: `docs/decisions/ADR-SRS-008-dominant-peak-identification.md`
- 状態: 採用・未実装
- 決定日: 2026-05-14
- Context: 削除候補 SOTA サミットに対し dominant peak（従属先）を特定する必要がある。コル等高線ポリゴンによる包含判定が基本だが、複数包含・包含なしのエッジケースが存在しうる
- Decision: 3段階の判定:
  1. dominant peak のコル等高線ポリゴン内に SOTA サミット座標が含まれる検出ピークを採用
  2. 複数のポリゴンに含まれる場合は最も近い検出ピークを採用（タイブレーク）
  3. いずれのポリゴンにも含まれない場合（フォールバック）: 最近接検出ピーク
- 距離計算: Haversine 公式（測地線距離）
- Alternatives:
  - 単純最近接のみ（コル等高線判定を使わない）→ プロミネンス計算結果との整合が取れず却下
  - 複数包含時のタイブレークを「最高コル」に → 実装複雑、最近接で十分と判断
  - フォールバックなし（包含なしはエラー） → メッシュ境界 truncation や SOTA リスト座標精度の問題で実用上必須
- Consequences: 縦走路上のサミットや古い座標データでも安定して dominant peak が特定できる

**SRS からのリンク**:
- FR-013 Point: 検出ピーク `match_status` の説明に ADR-URD-007 へのリンク
- FR-009 「dominant peak 特定」セクションに ADR-SRS-008 へのリンク
- 参照形式は ADR-SRS-006 と同じ: `[ADR-URD-007](decisions/ADR-URD-007-peak-match-status-terminology.md)`

### 1. peak の match_status 値を "deleted" → "dominant" にリネーム

**`docs/02_SRS.md` の変更箇所:**

- **FR-008** （L318-321）match_status 値の定義をピーク中心の表現に書き直す:
  - `matched`: 検出ピークのアクティベーションゾーン内に既存 SOTA サミット座標が存在する
  - `new`: 検出ピークのコル等高線内に既存 SOTA サミット座標が存在しない
  - `dominant`: 検出ピークのコル等高線内に既存 SOTA サミット座標が存在するが、アクティベーションゾーン外（= サミットが削除候補となり、このピークがその dominant peak になる）

- **FR-009** （L321 周辺）`deleted サミットの従属ピーク特定` のセクション名と本文の用語を更新:
  - セクション名: `dominant peak 特定` 等に変更
  - 本文の「deleted サミット」を「削除候補サミット」または「subordinate サミット」に置換

- **FR-011** （L455-459）申請書 XLSX マッピング表の「削除」アクション行:
  - 「| 削除 | SummitCode | 削除 | ... |」 のマッピングは `match_status="dominant"` の行に対して適用と SRS に明記
  - 注釈の※4 削除根拠フォーマットはそのまま使う

- **FR-013** フィーチャ構成テーブル（L358-360）の行ヘッダーを `deleted` → `dominant` に変更

- **FR-013** Point: 検出ピーク の `match_status` 値定義（L366）: `matched / new / dominant`
- **FR-013** Point: 既存 SOTA サミット の `match_status` 値（L385）: **`matched / deleted` のまま維持**
- **FR-013** LineString: ピーク → SOTA サミット の `match_status` 値（L410）: `matched / dominant`（peak と整合）

- **FR-012** CSV `match_status` カラムの値説明（L482）: `matched/new/dominant`

### 2. dominant_peak_dist_m を復活

**`docs/02_SRS.md` の変更箇所:**

- **FR-009** L331 の付与カラム記述に追加:
  - `付与するカラム: dominant_peak_code（検出ピークコード）、dominant_peak_dist_m（dominant peak から SOTA サミット座標までの距離 m。Haversine 公式で計算）`

- **FR-012** CSV カラム表に1行追加（dominant_peak_code の直後）:
  - `| dominant_peak_dist_m | dominant peak から SOTA サミット座標までの距離 m（Haversine 公式）。dominant のみ |`

### 3. GLOSSARY 更新

**`docs/00_GLOSSARY.md` の変更:**

- 「サミットコード」関連用語の近くに `dominant peak` 用語を追加
  - 例: `| dominant peak | 削除候補となる SOTA サミットが従属する検出ピーク。サミットがそのピークのコル等高線内に含まれることで判定される。SRS FR-009 参照 |`

### 4. モックアップ更新

**`docs/mockup/viewer_mockup.html` の変更:**

- ダミーデータ（L157-175 の deleted ブロック）:
  - peak feature の `match_status: "deleted"` → `"dominant"`
  - LineString (coord_diff) の `match_status: "deleted"` → `"dominant"`
  - **SOTA summit feature の `match_status: "deleted"` はそのまま**

- ポップアップ（L510-518 のピーク deleted ブロック）:
  - バッジ表示を `dominant` に変更
  - ラベル「コード」のままで OK

- カテゴリ判定 `categorize()`（L399-407）:
  - `matchStatus === "deleted"` の判定を `=== "dominant"` に変更
  - フィルター UI ラベル「削除」は維持（peak match_status="dominant" を「削除」フィルタにマッピング）

- sota_summit セクション（L546 周辺）:
  - peak match_status="dominant" の場合の処理を追加（現状 sota_summit の match_status="deleted" でカテゴリ判定しているが、これは維持して OK）

## Critical Files

- `docs/02_SRS.md` （FR-008, FR-009, FR-011, FR-012, FR-013）
- `docs/00_GLOSSARY.md`
- `docs/mockup/viewer_mockup.html`
- `docs/decisions/ADR-URD-007-peak-match-status-terminology.md`（新規）
- `docs/decisions/ADR-SRS-008-dominant-peak-identification.md`（新規）

## 懸念事項（実装時の注意）

1. **FR-010 削除候補のスコープ** … この章は SOTA サミット側の選定処理なので、用語「削除候補」を維持（peak の `dominant` とは別概念）。本計画では特に変更不要。

2. **CSV 行の denormalization** … 1 つの dominant peak が複数の subordinate サミットを持つ場合、CSV では subordinate サミット数分の `dominant` 行ができ、各行に同じ peak 情報が重複して入る。これは現状の SRS のままで、本計画では変更しない（別途検討事項として残す）。

3. **テキスト検索の徹底** … `grep -n "deleted" docs/02_SRS.md docs/00_GLOSSARY.md docs/mockup/viewer_mockup.html` でヒットしたものをすべて確認し、SOTA summit feature の文脈以外で `deleted` が残っていれば `dominant` に置換する。`merged_activation.geojson`（中間ファイル）には match_status はないため対象外。

4. **モックアップのコメント** … 「deleted: 削除候補」等のコメントも「dominant: 削除対象サミットを持つピーク」等に書き直す。

## Verification

実装完了後:

1. **SRS 整合性**:
   ```bash
   grep -n "deleted" docs/02_SRS.md
   ```
   ヒット箇所が「Point: 既存 SOTA サミット の match_status」「FR-010 削除候補のスコープ」「FR-011 申請書の "削除" アクション」「※4 削除根拠フォーマット」など、SOTA サミット側または申請書アクション側の文脈のみに限定されていることを確認。

2. **モックアップ動作確認**:
   - ブラウザで `docs/mockup/viewer_mockup.html` を開く
   - dominant ピーク（JA/YN-099 の例）のポップアップに `dominant` バッジが表示される
   - SOTA サミット側のポップアップは `deleted` バッジのまま
   - フィルター UI の「削除」チェックボックスのオン/オフで dominant ピークと subordinate サミットの両方が表示・非表示される
   - ピーク・コル・サミット・LineString・コル等高線が正しく紐付いて表示される

3. **コミット**:
   - 1コミットでまとめて OK（リネーム + dominant_peak_dist_m 復活 + 関連文書更新）
   - コミットメッセージ例: `refactor: peak match_status を dominant にリネーム・dominant_peak_dist_m 復活`
