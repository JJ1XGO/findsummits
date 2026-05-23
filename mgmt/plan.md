# URD/SRS および関連文書の整合性レビュー・修正

## Context

「フェーズ3.5: 独立峰対応（広域再解析）」の方針が ADR-011（2026-05-21）で**コル探索に特化・ポリゴン生成は実行しない**に確定された。しかし旧方針の記述が、ADR-011 以前に書かれた ADR-004（2026-05-18）と SRS の数箇所に残っており、ドキュメント間で矛盾している。

ユーザーは SRS L351 の `widearea_*_activation.geojson` の残存に気付き、他にも漏れがあるのではと懸念。3つの Explore エージェントで網羅調査した結果、**高優先（仕様矛盾）8件・中優先（記述漏れ）5件・低優先（用語ゆらぎ）3件**を検出。

### ユーザー決定事項
1. **ADR-004 は全面改訂**（ADR-011 整合に書き直し、Alternatives に旧設計の経緯を残す）
2. **高＋中 を本セッションで修正**（低3件は ISSUE 登録のみ）
3. **本セッションで実装まで一気に**（Plan 承認 → /model Sonnet 切替 → 実装）

### 正本（修正方針の判断基準）
- 広域モード: **CSV のみ出力、GeoJSON は出力しない**（ADR-011 L105）
- `key_col_resolved=false` の delete判定ゾーン: **生成する**（閾値 `peak_elev - delete_zone_max_drop`、ADR-011 L34）
- FR-014 トリガー: **`key_col_resolved=false` のみ**（`area_complete=false` は対象外、ADR-011 L75）
- `area_complete=false` 想定外発生時: `is_area_incomplete` 不備フラグで処理停止

---

## 修正項目（高＋中、計 13 件）

### A. SRS 修正（`docs/02_SRS.md`、計 8 件）

#### A1. [高] L351 削除（広域 GeoJSON ファイル名定義）
- 該当: `- GeoJSON: $DATA_DIR/results/csv/widearea_<対象peak識別>_<n>x<n>_<col>_<row>_activation.geojson`
- 修正: この1行を削除。L349 の CSV のみ残す。

#### A2. [高] L777 修正（`key_col_resolved=false` の delete判定ゾーン生成可否）
- 該当: `「key_col_resolved=false のピークは delete判定ゾーンを生成しない（[ADR-011]）」`
- 修正: 該当文を削除。ADR-011 L34・SRS L253 と整合させる（`key_col_resolved=false` でも `peak_elev - delete_zone_max_drop` で生成する）。

#### A3. [高] L220 修正（FR-004 共通モジュールから FR-016 除外）
- 該当: `「解析・ピーク検出・コル検出のロジック自体は通常モードと共通（FR-005・FR-006・FR-016・FR-007）」`
- 修正: FR-016 を除外し、「広域モードは FR-016 のポリゴン生成を呼び出さない（詳細は FR-014 参照）」を補足。

#### A4. [高] L313 修正（FR-018 入力範囲明示）
- 該当: `「[FR-016] で出力された per-mesh *_activation.geojson を1つの統合 GeoJSON にまとめる」`
- 修正: 「通常 per-mesh の `<meshcode>_activation.geojson` のみを統合対象とする。広域モード（FR-014）は GeoJSON を生成しない」と明示。

#### A5. [高] L833 修正（merged_activation.geojson 役割記述）
- 該当: `「SOTA リスト突合・HTML ビューア生成・独立峰の広域再解析の入力として使用する」`
- 修正: 「独立峰の広域再解析の入力として使用する」を削除（FR-014 は GeoJSON を読まない）。

#### A6. [中] L307 / L317 周辺修正（FR-018 再入可能性）
- 該当: FR-008 には「再入可能性: FR-014 ループから複数回呼び出され」が L307 にあるが、FR-018 には記述なし
- 修正: FR-018 に「FR-014 のループ中は再入しない（広域モードで GeoJSON を生成しないため、最初の1回のみ）」を明示。

#### A7. [中] L372 修正（FR-009 入力前提）
- 該当: `「最終 merged.csv および merged_activation.geojson」`の前提記述
- 修正: 「merged_activation.geojson は通常 per-mesh の GeoJSON のみから統合される」を補足。

#### A8. [中] L117 修正（3.2 コンポーネント表）
- 該当: 地形解析エンジン出力欄「per-mesh CSV、per-mesh activation GeoJSON、標高地形図 PNG」
- 修正: 「処理モードにより出力が異なる（通常モード: CSV / activation GeoJSON / 地形図、広域モード: CSV のみ）」と注記。

---

### B. ADR-004 全面改訂（`docs/decisions/ADR-004-level14-max-pooling-isolated-peaks.md`、計 4 件）

ADR-011 整合に Decision／Consequences を書き直す。**Alternatives に旧設計の経緯を残す**ことで意思決定の履歴は維持。

#### B1. [高] L53 修正（トリガー条件）
- 旧: `「key_col_resolved=false または area_complete=false のピークを対象とする」`
- 新: `「key_col_resolved=false のピークを対象とする（ADR-011 によりトリガーを縮小）」`

#### B2. [高] L55-67 修正（呼び出しチェーン・出力スコープ・ファイル命名）
- L55 旧 `「FR-004→FR-005→FR-006→FR-016→FR-007」` → 新 `「FR-004→FR-005→FR-006→FR-007（FR-016 のポリゴン生成は呼び出さない）」`
- L56 旧 `「FR-008/FR-018 に再投入」` → 新 `「FR-008 のみに再投入（FR-018 は再入しない）」`
- L65 削除: `「area_complete=false のみのピークは 4×4 固定」` ← トリガー対象外なので不要
- L66 旧 `「広域 per-mesh CSV/GeoJSON のみを新規出力」` → 新 `「広域 per-mesh CSV のみを新規出力（GeoJSON は出力しない）」`
- L67 旧 `「..._activation.geojson」例示` → 削除（CSV のみ）

#### B3. [高] L96-98 削除（area_complete=false をトリガーに加えた理由）
- 該当段落をまるごと削除。
- 代わりに「ADR-011 により本トリガー条件は縮小された」を簡潔注記。

#### B4. [高] L138-142, L154-159 修正（Consequences）
- L138-142 旧「merged_activation.geojson を再生成」 → 「merged_activation.geojson は再生成しない（広域モードで GeoJSON を出力しないため）」
- L154-159 Python 責務リストから `area_complete=false` 抽出を削除し、`key_col_resolved=false` のみとする。FR-018 再実行も削除し FR-008 再実行のみとする。

#### B5. [中] ADR-004 メタ情報に ADR-011 改訂の注記追加
- メタテーブル直下に「※ ADR-011（2026-05-21）により Decision／Consequences の一部を改訂済（ポリゴン生成廃止・area_complete=false トリガー対象外化）」を追記。

---

### C. ISSUE 登録（低3件 + 既存1件）

低優先3件と未登録の中優先1件（M2: ADR-004 と ADR-011 の時系列履歴記録）を登録。ただし B5 で M2 を直接対応済みになるので、ISSUE 登録は **低3件のみ** で OK。

#### C1. SRS L327-329 と L336-337 の重複記述整理
- ステージ: SRS
- 推奨優先度: 低
- 内容: フェーズ3.5 セクション冒頭と FR-014 トリガー条件で `area_complete=false` トリガー外しが2重記述。整理して片方に集約。

#### C2. SRS FR-014 で「`key_col_resolved=false` ＝ 独立峰」の対応関係を明示
- ステージ: SRS
- 推奨優先度: 低
- 内容: L335-336 周辺に「Key コルが 3×3 解析範囲外にある独立峰」と注記を追加。

#### C3. `docs/figures/context.drawio` ラベル大文字統一
- ステージ: SRS（図整備の一環）
- 推奨優先度: 低
- 内容: 「申請書 xlsx」「エビデンス csv」を「申請書 XLSX」「エビデンス CSV」に統一。SVG エクスポートはユーザー対応。

---

## 修正順序

1. **B (ADR-004 全面改訂)** から着手 — 修正範囲が最大、根本仕様文書なので最初に固める
2. **A (SRS 修正)** を ADR-004 改訂後に実行 — ADR-004 と整合させながら書く
3. **検証** — grep で「widearea」「activation.geojson」「FR-016」「area_complete=false」を引いて整合確認
4. **C (ISSUE 登録)** — 上記完了後に低3件を `track.py issue add` で登録
5. **コミット** — Conventional Commits 形式・本文日本語で個別ファイル指定コミット
6. **トラッカー対応** — 該当 ISSUE（既存があれば close、なければ新規 ISSUE として登録してこのレビュー作業を記録）

### コミット粒度の案
- コミット1: `docs(ADR-004): ADR-011整合に全面改訂（広域モードのポリゴン生成廃止反映）`
- コミット2: `docs(SRS): フェーズ3.5広域再解析の旧方針記述を ADR-011 整合に修正`
- コミット3: `chore(tracker): フェーズ3.5レビュー残課題3件を ISSUE 登録`

---

## 検証方針

修正完了後、以下のgrep が**いずれも空 or 想定箇所のみ**であることを確認する：

```bash
# 広域 GeoJSON 関連の残存チェック
grep -n "widearea.*activation\|widearea.*geojson" docs/02_SRS.md docs/decisions/ADR-004*.md

# 広域モードで FR-016 呼び出しの古い記述
grep -n "FR-016.*広域\|広域.*FR-016" docs/02_SRS.md docs/decisions/ADR-004*.md

# area_complete=false が FR-014 トリガーとされる古い記述
grep -n "area_complete=false" docs/02_SRS.md docs/decisions/ADR-004*.md
# → トリガー対象「外」と書かれた箇所、または不備フラグで処理停止の箇所のみのはず

# FR-018 再入の古い記述
grep -n "FR-018.*再" docs/decisions/ADR-004*.md
# → 「再入しない」「再投入しない」表現のみのはず
```

修正後の機能要件チェック（ヒューマンレビュー）:
- ADR-011 / ADR-004 / SRS 三者で**広域モードの出力（CSV のみ）が一致**しているか
- `key_col_resolved=false` の delete判定ゾーン生成可否が SRS 内2箇所（L253・L777周辺）で**一致**しているか
- ADR-004 の Alternatives セクションに**旧設計の経緯**が（一行注記でなく）残されているか

---

## 関連ファイル

### 修正対象
- `docs/02_SRS.md` — A1〜A8（L117, L220, L307/L317, L351, L372, L777, L833）
- `docs/decisions/ADR-004-level14-max-pooling-isolated-peaks.md` — B1〜B5（全面改訂）

### 参照（読み取りのみ）
- `docs/decisions/ADR-011-delete-zone-polygon.md` — 正本
- `docs/figures/phases_overview.drawio.svg` — フェーズ3.5 が現方針整合（GeoJSON 表示なし）であることを確認済み

### 影響なし（今回触らない）
- `docs/01_URD.md` — 本方針変更は UR レベルに影響しない
- `docs/00_GLOSSARY.md` — delete判定ゾーン定義は ADR-011 整合済み
- `docs/decisions/ADR-007/008/010/012` — 影響なし

### ISSUE 関連
- ISSUE-045（`config.ini.example` に `delete_zone_max_drop=250` 追加・COD ステージ）: 既登録。今回は触らない
- 本レビュー作業自体を ISSUE として登録するか: 作業完了後に判断（既存 ISSUE-031/032 とは別件）
