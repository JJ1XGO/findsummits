# docs リンク切れ・レイアウト崩れの検出自動化＋実害3件の修正

## Context（なぜ / 何を解決するか）
ユーザーが kate（Marksman LSP）で 2 種類の不整合に気づいた:
1. **リンク切れ**: 存在しないファイルへの相対リンク（例: `ADR-SRS-004:180` が古い番号 `ADR-SRS-014-land-summit-highest-peak-handling.md` を参照。実体は番号が振り直され `ADR-SRS-019-land-summit-highest-peak-handling.md` に。014 番は別内容へ再利用済み）。
2. **レイアウト崩れ**: 箇条書き `- **概要**: …` の直後に空行なしで `**入力**:` が続き、`**入力**:` が概要項目に吸収されて同列に見える（FR-013=SRS:1036、FR-009=SRS:834 の 2 件）。

調査の結果、**どちらも PyMarkdown の標準ルールでは検出不可**（リンク実在検証ルールが無く、レイアウト崩れは markdown 構文上は正しいため）。Marksman を入れてもリンク切れしか拾えない。一方、軽量な自作 Python チェッカーで両方とも検出できることを実証済み（実際に計 3 件を検出）。ユーザー選択により **Marksman は導入せず、自作チェッカーを既存 `make lint-md`／PostToolUse フックに統合**する。

## 実装内容

### 1. 自作チェッカー `scripts/lint_docs.py` を新規作成
PyMarkdown を補完する 2 検査を行う read-only スクリプト。引数にファイル/ディレクトリを取り、違反を `path:line: 種別: メッセージ` 形式（PyMarkdown 風）で出力。違反ありで exit 1。

- **検査A（リンク実在）**: 各 `.md` 内の相対リンク `](xxx.md)` / `](xxx.md#frag)` を正規表現で抽出し、`http(s)` を除外、`os.path.normpath` で実在確認。存在しなければ違反。
- **検査B（レイアウト崩れ）**: `**入力**:` `**出力**:` `**説明**:` 等の太字ラベル単独行で、直前行が空行でない（＝箇条書き項目に吸収される）箇所を違反として報告。誤検出を避けるため「太字ラベル＋コロンのみの行」かつ「直前が非空行」に限定（今回の検証で誤検出 0・実害 2 件のみヒット）。
- 単一ファイル引数も受け付ける（フックから 1 ファイル検査するため）。

置き場所は `scripts/lint_docs.py`（開発支援ツール。本番パイプラインからは独立）。

### 2. `Makefile` の `lint-md` ターゲットを拡張
PyMarkdown と自作チェッカーの**両方**を実行し、いずれかが違反なら非ゼロ終了する。PyMarkdown が違反で止まっても自作チェッカーが走るよう、各 exit code を退避して最大値で終了:
```makefile
lint-md: venv
	@venv/bin/python3 -m pymarkdown -c .pymarkdown scan -r $(LINT_MD_PATHS); s1=$$?; \
	venv/bin/python3 scripts/lint_docs.py $(LINT_MD_PATHS); s2=$$?; \
	exit $$([ $$s1 -ge $$s2 ] && echo $$s1 || echo $$s2)
```

### 3. PostToolUse フック（`.claude/settings.local.json`）に自作チェッカーを追加
既存の `pymarkdown scan` に続けて `lint_docs.py <編集ファイル>` も実行し、両方の違反を `additionalContext` に結合して注入する。`.md` 以外は従来どおりスキップ。

### 4. 実害3件の修正（検出ロジックの妥当性確認も兼ねる）
- `docs/20_SRS.md`:1035→1036 間に空行挿入（FR-013 概要と入力を分離）
- `docs/20_SRS.md`:833→834 間に空行挿入（FR-009 概要と入力を分離）
- `docs/decisions/ADR-SRS-004-level14-max-pooling-isolated-peaks.md`:180 のリンクを
  `[ADR-SRS-014](ADR-SRS-014-land-summit-highest-peak-handling.md)`
  → `[ADR-SRS-019](ADR-SRS-019-land-summit-highest-peak-handling.md)` に修正

### 5. コミット
`scripts/lint_docs.py`・`Makefile`・`docs/` 変更を Conventional Commits・本文日本語でコミット。
`.claude/settings.local.json` は gitignore 対象のため対象外。

## 検証
1. `make lint-md` を実行 → PyMarkdown 0 件・自作チェッカー 0 件で **exit 0** を確認。
2. 修正前後で `scripts/lint_docs.py docs/` を単独実行し、修正前は 3 件検出・修正後は 0 件になることを確認。
3. フック動作確認: 既存の `.md` を 1 つ編集し、自作チェッカーの違反が（仕込んだ場合に）注入されることを確認。
4. `git diff` で実害3件が意図どおり（空行追加2・リンク番号1）であることを目視確認。

## モデル運用メモ
スクリプトの検出ロジックは調査で確定済み。実装はその Python 化＋ Makefile/フックへの組み込み＋3件修正で、ファイル編集中心。ExitPlanMode 承認後は **Sonnet** を推奨（フックのシェル組み込みのみ多少注意が必要）。
