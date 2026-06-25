# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## プロジェクト概要

国土地理院の標高タイル（レベル15）を使ってSOTA日本支部の対象サミットを最新化する為の支援ツール。
最新化するに当たって必要なO/Pは、下記3点。
・SOTA日本支部に更新申請をするに当たって申請用ファイル（excel。新規・変更・削除）があるので、それに値を入れたexcelファイル。最重要。
・対象サミットの一覧（座標と標高、そのサミットのKeyコル座標と標高。対象サミットとそのKeyコルの標高がわかればプロミネンスがわかるが一応データとしてはプロミネンスも欲しい。csvで可）。申請書の内容を証明するためのエビデンスとして。
・国土地理院地図上で目視確認するためのGEOJSONファイル。申請書の内容を証明するためのエビデンスとして。
当然、現行の対象サミットに変更・削除があるケースが考えられるので、それらを考慮して申請先の人が見てわかる形のO/Pにする必要がある。

解析に使う基本データはdem5aをベースとし、dem5aがない/もしくは有効値がない座標はdem5b(の該当座標)から、dem5bがない/もしくは有効値がない座標はdem5c(の該当座標)から、dem5cがない/もしくは有効値がない座標はdem10b(の該当座標)から標高値を取得する。

## 仕様優先原則（重要）

**現在のコードは仕様が曖昧な状態で Claude が意図を推測して書いたものであり、正式な仕様ではない。**

- **URD/SRS/HLD/LLD が目標状態**。コードはその暫定的な副産物に過ぎない
- **コードと URD/SRS の乖離は意図的かつ正常**。コードを正にしてはならない
- **仕様を決めてからコードを書く**。SRS/HLD/LLD レビュー中は実装に手を入れない
- この原則は HLD/LLD フェーズでも同様に適用する

Claude へ: SRS/HLD/LLD の記述を確認・レビューするときに「でも実装ではこうなっている」という視点でコードを参照してはならない。仕様書の内容が正であり、コードは後で仕様に合わせて書き直す。

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
3. 原因がわかっている場合は `--resolution` に対応方針まで記入してから登録する
4. 登録完了後、修正作業の承認を得てから着手する
5. 修正完了後は `bug close` コマンドでステータスを「対応完了」にする（解決済にしない）
6. ユーザーが確認完了後、`bug verify` コマンドでステータスを「解決済」にする

詳細な運用手順・コマンド一覧は `mgmt/tracker/CLAUDE.md` を参照。

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
- **粒度**: 数行で書ける作業単位。実装ステップは箇条書きでチェックボックス化可
- **管理方法**: 完了したものは消す（履歴は git で追える）。長期保留中のものは「保留」セクションへ
- **新規発生時**: 「文書・仕様の議論を伴うか？」で判定し、No なら todo.md へ追記（issue 登録不要）

handover との関係:

- todo.md と issue は **原本**（永続的な作業リスト）
- handover はセッション終了時点の **スナップショット**。todo.md と issue の未対応分を抜粋して載せる

Issue から todo.md への降格判定:

- 文書・仕様の議論が不要 / 単独のファイル修正で完結 / 純粋な実装タスクのみ → todo.md へ移行可
- 移行時は `issue close` の理由欄に「todo.md に移行」と記載し、todo.md 側に転記する

## 計画ファイル・handover の扱い

- **plan.md の置き場**: プロジェクトの `mgmt/plan.md` とする。`/plan` コマンドはシステムの都合でグローバルの `.claude/plans/` に自動生成するため、plan モードが終わっていれば直ちに所定の場所に移動する事。
- **handover ファイル名の日時**: ファイル名に使う日時は必ず `date '+%Y-%m-%d_%H%M'` コマンドで実時刻を取得すること。会話履歴や記憶から日付を推測してはならない（同日別セッションとの衝突を防ぐため）。

## handover 実行時のルール

`/handover` を実行するとき（または手動で handover ドキュメントを作成するとき）は、
**A → B → C の順**で実行する。コミットを先に済ませ、git をクリーンにしてから handover を書く。

### A. handover ファイル作成前: それまでの作業をコミット

1. Excel レポートの条件付き更新:

   ```bash
   venv/bin/python3 mgmt/tracker/track.py bug export --if-changed
   venv/bin/python3 mgmt/tracker/track.py issue export --if-changed
   ```

2. Conventional Commits 形式・本文日本語でコミットし、`git status` がクリーンになったことを確認する。
   ※ `.claude/handovers/` は `.gitignore` 済みのため、handover ファイル自体はここでもコミットされない。

### B. handover ファイル作成

1. 構成は `~/.claude/commands/handover.md` に従う。
2. 末尾に「未対応バグ・課題サマリー」を追記する。件数は `track.py bug/issue list --open`、todo.md 残件は `mgmt/todo.md` を直接カウント。

### C. 後始末（フォールバック）

1. `git status` を確認し、管理対象の差分が残っていればコミットしてクリーンにする。

## ドキュメント更新時のルール

ユーザーはドキュメントを GitHub 上で確認しているため、ローカル編集だけでは確認できない。
**ドキュメントの更新が完了した直後（その作業ターン内）に必ず commit する**こと。push は別途指示があるまで不要。

対象ドキュメント:

- `docs/` 配下のすべてのファイル（URD/SRS/HLD/LLD/UT/IT/ST/OPS/GLOSSARY/environment）
- `docs/decisions/` 配下の ADR と research 資料
- `ref/SOURCES.md` などの参照資料
- `mgmt/plan.md`・`mgmt/lessons.md`（devel ブランチ運用ファイル）

手順:

1. 更新作業が一段落したら `git status` で対象を確認
2. `git status` で想定外の野良ファイルがないことを確認のうえ `git add -A`（意図しない変更が見える場合のみ個別指定）
3. Conventional Commits 形式・本文日本語でコミット
4. push は別途指示があるまで行わない（ユーザーが任意のタイミングで push する）

例外: 同一作業内でコードと一緒に更新したドキュメントは、コードのコミットに含めて構わない（ドキュメント単独でのコミット分割は不要）。

**markdown lint**: `.md` を編集したらコミット前に `make lint-md` を実行する。検出された違反は編集対象外のファイルのものでも全て修正してからコミットする。lint 対象は `mgmt/archive/` を除く現用 md 全体。

## ブランチ運用ルール

- `devel → main` の直接マージ禁止
- リリース時は必ず `release/*` ブランチを介す:
  1. `git checkout -b release/vX.X`
  2. `git rm -r mgmt/`
  3. `git commit -m "chore: リリース用にmgmt/除外"`
  4. `git checkout main && git merge release/vX.X`

## その他

他のプロジェクトの参考コードは以下の場所にあります：
@../findsummits4sotaja/ # 以前、pythonで開発した時のプロジェクト。九州・四国を解析してSOTA日本支部に申請した時のもの。

### SOTA関連資料

- [SOTAの山岳リスト](https://www.sotadata.org.uk/summitslist.csv)（JAで始まるものが日本支部のサミット）
- [SOTA日本支部への山岳リスト更新申請書](https://www.kawauchi.homeip.mydns.jp/sotajp/wp-content/uploads/2024/03/SOTA-Summit-list-revision-request.xlsx)
