# CLAUDE.md

## プロジェクト概要

国土地理院の標高タイル（DEM5/DEM10b）を解析し、SOTA 日本支部のサミットリスト更新申請に必要な成果物を生成する支援ツール。

主な成果物は3点:

- **申請書 XLSX**（最重要）: SOTA 日本支部指定フォーマット（追加/変更/削除）
- **エビデンス CSV**: サミット／Keyコルの座標・標高・プロミネンス
- **目視確認用 GeoJSON / HTML ビューア**: 地理院地図上での検証用

現行リストとの差分（新規・変更・削除）も識別する。要件の正は [`docs/10_URD.md`](docs/10_URD.md)。

標高値は dem5a → dem5b → dem5c → dem10b の順でフォールバックする（各座標で「タイルなし」または「無効値」なら次のソースへ）。

## 仕様優先原則（重要）

**現在のコードは全体像のイメージを掴むためにプロトタイプとして書いたものであり、正式な仕様ではない。**

- **URD/SRS/HLD/LLD が目標状態**。コードはその暫定的な副産物に過ぎない
- **コードと URD/SRS の乖離は意図的かつ正常**。コードを正にしてはならない
- **仕様を決めてからコードを書く**。SRS/HLD/LLD レビュー中は実装に手を入れない

※ 現在この原則は有効（HLD/LLD 未完成）。完成後は「ドキュメント・実装整合性原則」が主となる。

## ドキュメント・実装整合性原則

仕様書・設計書（URD/SRS/HLD/LLD/ADR）の内容とプログラムの実装は常に整合させること。

- 仕様変更時はドキュメントとコードを同期して更新する（片方だけの修正禁止）
- 実装が仕様と乖離した場合は、仕様優先原則に基づき仕様を正としてコードを修正する
- 乖離が意図的で許容される場合は ADR として記録する

## ビルド・テスト

```bash
make                    # build/findsummits をビルド
make test_mesh_analyze  # build/test_mesh_analyze をビルド
make test_analyze       # build/test_analyze をビルド
make clean              # build/ ディレクトリごと削除

./build/findsummits 4929                        # 1次メッシュコード指定で実行
./build/findsummits params/mesh_list_japan.txt  # メッシュリストファイル指定で実行
./build/test_mesh_analyze 4929                  # テスト（イメージ出力なし）
./build/test_mesh_analyze 4929 --save-image     # テスト（標高地形図 PNG も出力）
```

依存: `libpng`, `libm`, `pthread`（GCC / C99）

## 開発ドキュメント管理

docs/ 配下を編集するときは採番・フォーマット・ADR ルールを `docs/CLAUDE.md` で確認すること。

## 欠陥管理ルール（必須）

**バグ・欠陥を発見しても、すぐに修正を始めてはならない。必ず以下のフローを守ること。**

1. テスト結果を確認し、発見した欠陥を**すべて洗い出してから**まとめて一覧提示する
2. ユーザーに確認を取ってから `venv/bin/python3 mgmt/tracker/track.py bug add` で登録する
3. 登録完了後、修正作業の承認を得てから着手する

詳細な運用手順・コマンド一覧（close/verify 等）は `mgmt/tracker/CLAUDE.md` を参照。

## 課題管理ルール

**課題管理（`mgmt/tracker/` issue）はプロジェクトの仕様・設計・調査に特化する。**

- **issue type は 改善・調査・設計 の 3 種のみ**。「機能追加」は廃止済み（ISSUE-120）。FR 確定済みの実装タスクは `todo.md` で管理する

**判定基準: 残作業に文書・仕様の議論が必要か？（出自ではなく残作業で判定）**

- Yes → `issue`（SRS の FR 追加、ADR 作成、SRS と実装の乖離調査 等）
- No  → `todo.md`（関数名リネーム、ログ書式統一、コメント修正、実装追従 等）

- **1 項目 1 課題**: 複数の課題を1件に詰め込まない
- **issue のスコープ**: 「問い＋決着（決定＋ADR/SRS への記録）」まで。記録完了 = 対応完了
- **impersonation 禁止**: AI 登録の課題・バグは `報告者`・`--actor` ともにモデル名（Sonnet/Opus 等）。ユーザー名を充ててはならない

登録フロー: `issue add` → 作業開始時 `issue update --status 対応中` → `issue close` → ユーザーが `issue verify`

詳細な運用手順・判定基準・コマンド一覧は `mgmt/tracker/CLAUDE.md` を参照。

## ToDo リスト運用ルール

**作業リスト（仕様議論を伴わない実装タスク等）は `mgmt/todo.md` で管理する。**

- **用途**: 文書・仕様の議論を伴わない作業の保管庫（実装タスク・ファイル名追従・ログ整備・運用作業・チェックリスト等）
- **管理方法**: 完了したものは消す（履歴は git で追える）。長期保留中のものは「保留」セクションへ
- **新規発生時**: 「文書・仕様の議論を伴うか？」で判定し、No なら todo.md へ追記（issue 登録不要）

issue を todo.md へ降格する場合は `issue close` の理由欄に「todo.md に移行」と記載し、todo.md 側に転記する。

## 計画の自動レビュー（plan self-review）

`ExitPlanMode` でユーザーへ計画を提示する**前**（承認を得る前）に、計画が `docs/` 配下のファイル（URD/SRS/HLD/LLD/ADR 等）の作成・更新を含む場合のみ `/spec-panel mgmt/plan.md` を実行してセルフレビューを行う（視点の詳細は `.claude/commands/spec-panel.md` 参照）。`docs/` を伴わない計画（コード修正・ツール整備・運用作業等）はスキップする。

**省略・短縮は禁止**。以下を必ず守ること:

- `/spec-panel` の実行をスキップしない。理由・規模・確信度によらず省略しない
- `/spec-panel` 実行後、指摘記録ファイル（`mgmt/spec-findings/` 配下）が実際に作成されたことを確認してから `ExitPlanMode` を出す。ファイルの存在確認なしに次へ進まない
- 指摘がある場合は計画に反映するか、確認事項としてユーザーに明示してから `ExitPlanMode` を出す

## 計画ファイル・handover の扱い

- **plan.md の置き場**: プロジェクトの `mgmt/plan.md` とする。`/plan` コマンドはシステムの都合でグローバルの `.claude/plans/` に自動生成するため、ExitPlanMode 承認後・ファイル編集を始める前に `mv` で移動する。
- **plan.md 内のファイル参照はコード表記にする**: `mgmt/plan.md` 内で `docs/...` 等のリポジトリ内ファイルを参照するときは、Markdown リンク `[..](..)` ではなく**コード表記（バッククォート）**で書く。`mv` 元（`.claude/plans/`）でも移動先（`mgmt/`）でも相対リンクが解決せず broken-link になるため、リンクにしないことで構造的に回避する。`mgmt/plan.md` は lint 対象に含めたまま運用する（隠さない）。
- **計画の各タスクに実行モデルを明記する**: グローバル CLAUDE.md「モデルを使い分ける」節の3条件に該当し Opus/Fable サブエージェントへの委譲が想定されるタスクには理由を付記する（既定は Sonnet 直接対応のため無印でよい）。
- **handover ファイル名の日時**: ファイル名に使う日時は必ず `date '+%Y-%m-%d_%H%M'` コマンドで実時刻を取得すること。会話履歴や記憶から日付を推測してはならない（同日別セッションとの衝突を防ぐため）。

## handover 実行時のルール

handover を書く前に git をクリーンにする（コミットを先に済ませる）。詳細手順は `/handover` スキル参照。

## ドキュメント更新時のルール

ユーザーはドキュメントを GitHub 上で確認しているため、ローカル編集だけでは確認できない。
**ドキュメントの更新が完了した直後（その作業ターン内）に必ず commit する**こと。push は別途指示があるまで不要。

対象ドキュメント:

- `docs/` 配下のすべてのファイル（URD/SRS/HLD/LLD/UT/IT/ST/OPS/GLOSSARY/environment）
- `docs/decisions/` 配下の ADR と research 資料
- `ref/SOURCES.md` などの参照資料
- `mgmt/plan.md`（devel ブランチ運用ファイル）

※ `.claude/best_practices.md` は例外: 上記の手動手順ではなく `/update-best-practices` 実行時にコマンド内で完結する（詳細は「Best Practices（教訓蒸留）運用ルール」参照）。

更新が完了したターン内に: ① `make lint` 警告ゼロを確認 → ② 意図した変更ファイルを個別に `git add`（全対象を確認済みなら `git add -A` 可）→ ③ Conventional Commits でコミット → ④ push は別途指示まで行わない。

例外: 同一作業内でコードと一緒に更新したドキュメントは、コードのコミットに含めて構わない。

## Best Practices（教訓蒸留）運用ルール

- 学びは `.claude/lessons.md` に随時記録する（git 管理外・コミット不要）
- `/update-best-practices`（グローバルコマンド、Opus 実行）が `.claude/lessons.md` を再分析し、
  `.claude/best_practices.md`（git 管理対象）を再合成する
  - 蒸留観点: 手戻り防止 / 判断コスト削減 / 信頼性の担保 / コンテキスト継続 / 仕様と実装の整合
  - 原則数目安: 14〜18件（増えすぎたら統合する）
  - 除外: プロジェクト固有の技術詳細（dem10b 解像度・openpyxl API 等）は原則に含めない
  - 実行後、`.claude/best_practices.md` と `.claude/best_practices_watermark` はコマンド内でコミットまで完結する
- lessons.md が一定量増えるとセッション開始時に実行が自動的に推奨される（hooks 側で検知）

## 機械的チェック（lint / LSP 等）

`make lint` で全警告ゼロを保つ。

- 現在の構成:
  - `lint-md`: pymarkdown + `scripts/lint_docs.py`、`mgmt/archive/` 除外（対象: `*.md`）
  - `lint-py`: ruff（設定: `ruff.toml`）、`mgmt/archive/` 除外（対象: `*.py`）
  - `lint-geojson`: `scripts/lint_geojson.py`（geojson-validator ラッパー）、`mgmt/archive/` 除外（対象: `*.geojson`）
  - `lint-html`: djlint（設定: `.djlintrc`）、`mgmt/archive/` 除外（対象: `*.html`）
- チェック対象を追加するとき（LSP の CLI チェック・C コンパイラ警告・Python 型チェック等）は、CLAUDE.md に個別ルールを増やさず `make lint` の依存へ target を足す
- 編集時は PostToolUse hook が該当ファイルの違反を自動提示する。提示された違反はそのターン内で解消する
- lint ツールのバージョンは `requirements.txt` で固定（ローカルの再現性維持）。最新版での通過確認は `make lint-latest`（手動）と GitHub Actions（月1自動・main の workflow）で監視する

## ブランチ運用ルール

- `devel → main` の直接マージ禁止
- リリース時は必ず `release/*` ブランチを介す:
  1. `git checkout -b release/vX.X`
  2. `git rm -r mgmt/`
  3. `git commit -m "chore: リリース用にmgmt/除外"`
  4. `git checkout main && git merge release/vX.X`

## その他

### SOTA関連資料

- [SOTAの山岳リスト](https://www.sotadata.org.uk/summitslist.csv)（JAで始まるものが日本支部のサミット）
- [SOTA日本支部への山岳リスト更新申請書](https://www.kawauchi.homeip.mydns.jp/sotajp/wp-content/uploads/2024/03/SOTA-Summit-list-revision-request.xlsx)
