# handover の体裁修正 ＋ Obsidian でサッと閲覧（symlink方式・MCPなし）

## Context（なぜやるか）
handover 文書はローカル専用の markdown 群で（`/workspace/.gitignore:89` で除外＝GitHub には載らない）、
Obsidian で閲覧・全文検索・`[[ ]]` バックリンクを効かせたい。目的は「自分が見られればいい」
（MCP・プラグインで Claude に能動操作させる用途は今回スコープ外）。
あわせて、緊急時に手作業で代理作成され **markdown 文法が崩れている handover** が混在しているため、
体裁を整える（Obsidian でまともに表示させるための前提整備でもある）。

### 環境・調査の確定事実
- `/workspace`・`/home/node/.claude` は**ホストの実ディスク（nvme0n1p5）を bind mount したもの**。
  ホストの Obsidian から直接読める。MCP は閲覧目的には不要。
- handover 正本の保存先は `/workspace/.claude/handovers/`（`~/.claude/commands/handover.md` の規約、174件）。
- vault はコンテナにマウントされていない → **symlink 作成はホスト側でユーザーが実行**する必要がある。
- 文法崩れの handover は**2件のみ**（横断スキャンで確定。未閉フェンス無し、`2026-04-26_2300.md`の罫線は正規の遷移図で対象外）。

---

## Part 1: 崩れている handover の体裁修正（Claude が実施）

### 対象ファイル（2件）
1. `/workspace/.claude/handovers/2026-06-13-1632.md`
   - 崩れ: 全行が4スペース字下げ＝コードブロック化／見出し皆無／「作業:」「再開時の手順:」等が地の文。
2. `/workspace/.claude/handovers/2026-06-19_2033.md`
   - 崩れ: タイトルが `#` 無し／表が**罫線文字（┌─┬─┐）のASCIIアート**で markdown 表でない／全体字下げ／見出しが地の文。

### 修正方針（厳守）
- **本文の文言・意味は一字一句保持**。handover は歴史的記録なので内容は変えない。直すのは**書式のみ**。
- 具体的に行う変換:
  - 先頭の一律字下げを除去（コードブロック化の解除）。
  - 文書タイトルを `# Handover: YYYY-MM-DD_HHMM` 形式の H1 に。
  - 「作業」「確定した結論」「実装すべき内容」「検証」「次にやること」等の地の文ラベルを `##`/`###` 見出しに。
  - ASCII罫線表（`2026-06-19_2033.md` の2か所: ステータス表・既存issue再分類表）を**markdownパイプ表**に変換。セル文言は原文のまま。
  - 折り返しで分断された文（例: 「grep\n で…」）は意味単位で1行に再結合（語句は不変）。
  - 既存の番号付き手順（1./2./…）は正しいリスト記法に整える。
- 1ファイルずつ Edit し、変換後に整形結果を目視（見出し階層・表の列ズレ・本文欠落の有無）で検証。
- ファイル名 `2026-06-13-1632.md` のハイフン区切りはリネームしない（履歴の同一性を壊さない。本文H1のみ整形）。

---

## Part 2: 分家フォルダの集約（Claude が実施）
- `/home/node/.claude/handovers/`（5件・6/16〜6/19）が正本に存在するか `diff`/`md5sum` で照合。
- 正本に**無いものだけ**を `/workspace/.claude/handovers/` へコピー。重複は何もしない。
- 移送後、`~/.claude/handovers/` の扱い（残置 or 撤去）はユーザー確認のうえ決定。勝手に削除しない。
- ねらい: symlink 1本で全 handover を網羅できる状態にする。

---

## Part 3: Obsidian 閲覧（symlink方式・ユーザーが実行）
ホストの実パスは Claude から不可視のため、ユーザーが2つのパスを埋めて実行する。
- `<HOST_PROJECT>` = ホスト上の `/workspace` 相当パス
- `<VAULT>` = デフォルト vault のフォルダ

```sh
ln -s "<HOST_PROJECT>/.claude/handovers" "<VAULT>/Handovers"
```
- vault 内の symlink 名 `Handovers` は**ドット始まりにしない**（Obsidian が索引するため）。
- 任意拡張: `memory/`・`docs/` も同様に張ると `[[ ]]` リンク込みの知識ベースになる。

---

## 検証
- Part 1: 修正後、`grep -c '^#' <file>` で見出しが入ったこと、字下げ過多が解消したこと、表が `|...|` 形式になったことを確認。原文との意味差分が無いことを目視。
- Part 2: `ls /workspace/.claude/handovers/ | wc -l` で件数を確認。
- Part 3（ユーザー）: Obsidian に `Handovers/` が出現し、修正済み2件を含め整形表示・全文検索・`[[ ]]`リンクが効く。

## 注意・既知の制約
- **Obsidian Sync 利用時**: 同期は symlink 先の外部実体を辿らないことがある。複数端末同期が要件なら別途検討（今回は単一PC閲覧前提）。
- MCP は今回不採用。将来 Claude に Obsidian を能動操作させたくなったら「Local REST API プラグイン + mcp-obsidian」で後付け可（ホストゲートウェイ `169.254.1.2` 到達済）。

## モデル運用メモ
Part 1 は ASCII表→markdown表の逐語転記に多少の注意を要するが全体は機械的な書式変換、Part 2/3 は単純作業のため、承認後は **Sonnet** を推奨。
