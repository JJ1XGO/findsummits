# 計画: ビューア描画性能を URD/SRS/ADR に記録（RAIL 標準採用）

## Context

HTML ビューアのモックアップ開発時、地理院サーバ上のアイコン画像でマーカーを表示したところ、
マーカーが多い状態でのパン・ズームが遅く「使い物にならない」とユーザーが判断し、
**クライアント側 canvas 円描画方式**へ変更させた（`docs/mockup/viewer_mockup.html` に実装済み:
`summitRenderer = L.canvas` / `CircleMarker` / `preferCanvas:true`）。

しかしこの判断は **要求・決定としてどこにも記録されていない**:

- SRS FR-013 はマーカーの見た目（色・形状 ●▲▼・ズーム連動）は規定するが
  「形状の具体的描画方式（Canvas 等）は HLD に委ねる」と明記し、描画方式を意図的に外している
- 「なぜ地理院アイコンをやめたか（＝描画性能）」を記録した ADR も URD 要求も無い。HLD ファイルも未作成
- 既存 NFR-008（UI レスポンス「1秒以内」＝ Nielsen の単発応答閾値）は、
  **連続操作（パン・ズーム）のフレームレート＝今回破綻した観点**をカバーしていない

ユーザー判断で実装方式を変えた以上、その駆動要因（描画性能による使い勝手）を URD に要求として残し、
SRS に観測可能な実行方式と非機能数値を記載するのがあるべき姿。非機能の数値は恣意的に決めず
**公開標準（Google RAIL / Core Web Vitals）を引用**して根拠主義を満たす。

**採用標準（ユーザー確認済み: RAIL 完備）**:

| 観点 | 標準 | 値 |
|---|---|---|
| 連続操作（パン・ズーム）の滑らかさ | RAIL "Animation" | 60fps＝16ms/フレーム（実作業 ~10ms） |
| 入力→可視応答 | RAIL "Response" | 入力処理 ≤50ms（可視応答 ≤100ms） |
| 操作→次の描画（現行主指標・参考） | Core Web Vitals INP | "good" ≤200ms |

出典: [web.dev RAIL](https://web.dev/articles/rail)、Core Web Vitals INP（2024-03 に FID を置換）。

## 修正対象と内容

### A. `docs/10_URD.md`（新規 UR 追加）

- **UR-014**（新規）を「4. ユーザーニーズ」表に追加:
  > 目視確認用 HTML ビューアは、表示するマーカーが多い場合でも、地図のパン・ズーム等の操作が
  > 実用に耐える対話応答性・滑らかさで動作すること（根拠: [ADR-URD-017](...)）
- アンカー `<a id="ur-014"></a>` を付与（既存 UR と同形式）
- 定性表現に留め、具体数値は SRS（NFR）側へ委譲

### B. `docs/20_SRS.md`（NFR 新設 + FR-013 に実行方式1行）

1. **NFR-010**（新規）「描画応答性・連続操作の滑らかさ」を NFR-009 の後に追加:
   - **対応 UR**: UR-014
   - 連続操作（パン・ズーム）中は 60fps（フレーム ≤16ms）を目標とする（RAIL Animation）
   - マーカークリック・表示切替等の入力に対し、入力処理 ≤50ms・可視応答 ≤100ms を目標とする
     （RAIL Response。現行 Core Web Vitals INP "good" ≤200ms とも整合）
   - 適用対象は FR-013 の HTML ビューア。NFR-008（単発 UI 応答 1 秒）を補完する関係を明記
   - 出典として RAIL / INP を本文インライン注記
2. **FR-013** のマーカー記述（現 1113-1115 行付近）に観測可能な実行方式を1行追加:
   - 「マーカーはクライアント側で描画し、外部サーバ（地理院等）のアイコン画像取得に依存しない」
   - Canvas 等の**実装機構**は従来どおり HLD 委譲のまま（ADR-SRS-034 の方針を踏襲）

### C. `docs/decisions/ADR-URD-017-viewer-marker-rendering-performance.md`（新規 ADR）

- 採番: ADR-URD の現状最大 016 → **017**
- フォーマットは `docs/CLAUDE.md` ADR 規約に従う（状態/決定日 + Context/Decision/Alternatives/Consequences）
- 状態: 採用・実装済み（mockup に実装済みのため）
- 記録内容:
  - Context: 地理院アイコン方式の描画遅延（パン・ズーム時の fps 低下）でユーザーが実用不可と判断
  - Decision: クライアント側描画へ変更。性能基準として RAIL（60fps/16ms・応答≤100ms）を採用
  - Alternatives: ①サーバアイコン継続（却下: fps 破綻）②SVG/DOM マーカー（却下: マーカー多数で再描画コスト大）③Canvas 描画（採用）
  - Consequences: UR-014・NFR-010 を新設。実装機構の詳細は HLD/mockup に委譲

## ADR の要否（判断）

**必要**。UR 新設はメモリ規則「URD 要件追加・修正時は ADR を必ず作成」に該当し、
描画方式変更は代替案・却下理由を伴う設計判断。ADR-URD-017 として記録する。

## critical files

- `docs/10_URD.md` — UR-014 追加（A）
- `docs/20_SRS.md` — NFR-010 追加（NFR-009 直後 ≈1335 行）・FR-013 に1行（≈1113-1115 行）（B）
- `docs/decisions/ADR-URD-017-viewer-marker-rendering-performance.md` — 新規（C）
- 参照（編集なし）: `docs/mockup/viewer_mockup.html`（canvas 実装の実体）、`docs/decisions/ADR-SRS-034`（描画機構 HLD 委譲方針）、`ADR-SRS-006/015`（既存ビューア ADR）

## 採番の最終確認（編集時に再確認）

- UR-014 / NFR-010 / ADR-URD-017（本計画作成時点で最大値 +1 を確認済み）

## 検証

1. `grep -n 'UR-014' docs/10_URD.md docs/20_SRS.md` で URD 定義と SRS NFR-010 の対応 UR 双方向リンクを確認
2. `grep -n 'ADR-URD-017' docs/10_URD.md docs/decisions/` で UR-014 ↔ ADR 相互参照を確認
3. `grep -n 'NFR-010\|60fps\|RAIL' docs/20_SRS.md` で NFR-010 の数値・標準引用を確認
4. ADR の状態・決定日・4 セクション（Context/Decision/Alternatives/Consequences）が揃っていることを目視
5. `make lint`（pymarkdown + scripts/lint_docs.py）で **警告ゼロ** を確認
6. `git status` で野良ファイル無しを確認のうえ、Conventional Commits（本文日本語）でコミット（push しない）

## 補足

- 本計画ファイルは plan モード終了後、プロジェクト規約に従い `mgmt/plan.md` の扱いを確認のうえ整理する
  （`mgmt/plan.md` には別作業の計画が残っているため、上書きせずユーザーに確認）
- ADR 本文の文章化と代替案の評価でやや判断を要するが、標準は確定済みのため実装は Sonnet で対応可能
