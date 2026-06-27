# 計画: モックアップの位置付け明文化と SRS への必要レベル反映

> 注: 本ファイルは作業用の計画メモ。ファイル参照はコード表記（リンクにしない）で記す
> （`mgmt/` 起点では `docs/` 相対リンクが解決せず lint の broken-link になるため）。

## Context（なぜやるか）

`docs/mockup/viewer_mockup.html`（＋ `summits_data.js`）は HTML ビューアの UI を確定させる
プロトタイプだが、SRS からは §6.6・§6.13 で「画面サンプル」リンクとして参照されるのみで、
位置付け（プロトタイプか正式か・更新方針・SRS/HLD/LLD との関係）が未文書化だった。

ユーザー方針:

- モックアップを「UI を確定させるプロトタイプ（紙芝居レベル）」と位置付け、逐次更新して
  正しい状態に保つ。内容を必要レベルで SRS（以後 HLD/LLD も）へ反映する。
- 反映範囲は A: 位置付けの明文化＋SRS 観測挙動の不足分のみ。配色値・マーカーサイズ・
  `maxBounds`・z-index 等の実装具体値は SRS が既に「HLD に委ねる」と明記しており、
  HLD/LLD 未作成の現段階では mockup が暫定保持。HLD/LLD 起票時に抽出する。
- サンプルデータの要修正はユーザーが mockup 側で対応（本作業のスコープ外）。

調査結果の要点（Explore 2本）:

- SRS は既にモックアップの観測可能挙動をほぼ網羅（UI 約95%が実装済みかつ FR-019 と整合）。
- `docs/30_HLD.md` / `docs/40_LLD.md` は未作成。
- SRS の観測挙動の不足は既定背景タイルの明示（mockup 既定＝国土地理院標準地図）程度。

## 作業項目（実施済み）

1. `docs/decisions/ADR-SRS-039-mockup-positioning-and-spec-reflection.md`（新規）
   モックアップを UI 確定用プロトタイプと位置付けつつ、正は SRS/HLD/LLD（仕様優先原則維持）。
   観測挙動→SRS／実装具体値→HLD/LLD の抽出原則、HLD/LLD 起票時の抽出運用を記録。
2. `docs/20_SRS.md`
   - §6.6 画面サンプル行に位置付け注記（blockquote）＋ ADR-SRS-039 参照を追加。
   - §6.13 画面サンプル行に ADR-SRS-039 参照を併記。
   - §6.6 背景タイル行・FR-019 背景タイル行に「既定: 国土地理院標準地図」を明示。
3. `docs/00_GLOSSARY.md`: 用語「画面サンプル（モックアップ）」を追加。
4. `docs/CLAUDE.md`: 「モックアップの扱い」節を追加（反映運用ルール・ADR-SRS-039 参照）。

## スコープ外（明示）

- `docs/30_HLD.md` の新規作成（反映範囲 A により次段階）。
- mockup の配色値・サイズ・`maxBounds` 等を SRS へ取り込むこと（HLD 委譲を維持）。
- mockup サンプルデータ（new/delete/change）の修正（ユーザー対応）。

## 検証

1. `make lint` 警告ゼロ。
2. ADR-SRS-039 採番が未使用であること（前タスクで 038 作成済み＝次は 039）。
3. SRS・GLOSSARY・docs/CLAUDE.md から ADR-SRS-039 への参照が解決（broken-link ゼロ）。
4. ドキュメント更新ターン内に commit（即 commit ルール）。push は別途指示まで不要。

## 残課題（プロセス）

- `mgmt/plan.md` は `mv` 後に lint されず、`docs/` 相対リンクが broken になる構造的問題があった。
  本ファイルはコード表記で回避。恒久対策（plan.md の lint 除外 or リンク規約）は別途検討。
