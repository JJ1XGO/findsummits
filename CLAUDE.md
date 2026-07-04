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

※ HLD/LLD が未完成の間はこの原則が主となり、完成後は「ドキュメント・実装整合性原則」が主となる。

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
2. ユーザーに確認を取ってから `gh issue create --repo jj1xgo/findsummits --label bug --label priority:高`（優先度は高・中・低から選択）で登録する
3. 登録完了後、修正作業の承認を得てから着手する

詳細な運用手順・ラベル体系は「課題管理ルール」節を参照。

## 課題管理ルール

**課題管理は GitHub Issues（`jj1xgo/findsummits`）で行う。** 運用の共通定義はグローバル CLAUDE.md
「GitHub Issues による課題管理（opt-in）」を参照し、以下は findsummits 固有の定義。

**対象**: プロジェクトの仕様・設計・調査、および欠陥管理ルールで登録するバグ。

**ラベル体系**:

- **type**（issue のみ・3種）: `type:改善` / `type:調査` / `type:設計`。「機能追加」は使わない。
  FR 確定済みの実装タスクは `todo.md` で管理する
- **priority**（issue・bug 共通）: `priority:高` / `priority:中` / `priority:低`
- **bug**（バグのみ）: GitHub デフォルトの `bug` ラベル
- **on-hold**: 保留。再検討トリガーを本文に明記した上で付与する

**判定基準: 残作業に文書・仕様の議論が必要か？（出自ではなく残作業で判定）**

- Yes → `issue`（SRS の FR 追加、ADR 作成、SRS と実装の乖離調査 等）
- No  → `todo.md`（関数名リネーム、ログ書式統一、コメント修正、実装追従 等）

- **1 項目 1 課題**: 複数の課題を1件に詰め込まない
- **issue のスコープ**: 「問い＋決着（決定＋ADR/SRS への記録）」まで。記録完了 = 対応完了
- **impersonation 禁止**: 署名ルールはグローバル CLAUDE.md の定義に従う（findsummits 自身への投稿のため末尾にモデル名のみ、経緯説明は書かない）
- **クローズは `gh issue close` を正とする**（コミットの `fixes #N` は push まで閉じないため使わない）

**フロー**: `gh issue create` で登録 → 作業開始時に対応方針コメント → 実装 → 対応完了コメント →
**ユーザーが動作確認後に `gh issue close`**。調査の結果「仕様どおり・対応不要」と判明した場合は
対応側が説明コメント付きでクローズしてよい。

session-start hook が `jj1xgo/findsummits` の open issue 一覧を自動確認・注入する
（フェイルソフト。`gh` 不在・API 失敗時は一行メッセージのみでスキップする）。

## 環境課題の連携（claude-container への issue 起票）

findsummits 自体の仕様・実装ではなく、コンテナ環境（claude-container）に起因する問題・要望は、
`jj1xgo/findsummits` ではなく `jj1xgo/claude-container` への GitHub issue で起票する。

**判定基準（上記「課題管理ルール」の issue/todo 判定より前に適用）**: 問題の原因・対応先が
コンテナ環境側にある → claude-container への `gh issue`。findsummits 自身の仕様・実装の問題
→ 従来どおり `jj1xgo/findsummits` の issue または `todo.md`。

**フロー**: 起票 → （claude-container 側が調査・実装・対応完了コメント）→ 対応待ち →
**リビルド後**に動作確認 → 確認内容をコメントに付記してクローズ。

**ルール**:

1. クローズは原則起票側（findsummits）が動作確認後に行う。例外: 調査の結果「仕様どおり・
   対応不要」と判明した場合は、対応側（claude-container）が説明コメント付きでクローズすることがある
2. 動作確認は稼働中コンテナでは不十分になりうるため、リビルド（`-b`）後に行う
3. AI が起票・コメント・クローズする場合は、本文の**末尾に署名**として記入する。claude-container は
   findsummits 作業中のセッションから見て異なるリポジトリ（クロスリポジトリ連携）にあたるため、
   グローバル CLAUDE.md の署名ルールに従いモデル名に `(findsummits)` を付記する
   （例: `— Sonnet 5 (findsummits)`）。経緯の説明文（「findsummits の作業中に起票」等）は書かない。
   **この署名ルールはユーザー所有リポジトリへの投稿に限る**。外部プロジェクト（他者所有リポジトリ等）への
   issue・コメント投稿には署名を付けない
4. 起票先リポジトリ名・仕様は推測せず、不明な場合はユーザーに確認してから起票する
5. 1 issue 1 論点（上記「課題管理ルール」の「1 項目 1 課題」と同じ）

**注記**: `gh` はコンテナ内セッションのみ利用可能（ホストセッションには無い）。挙動は「課題管理ルール」
節の hook 説明と同じ（対象リポジトリが `jj1xgo/claude-container` である点のみ異なる）。

## ToDo リスト運用ルール

**作業リスト（仕様議論を伴わない実装タスク等）は `.claude/todo.md` で管理する。**

- **用途**: 文書・仕様の議論を伴わない作業の保管庫（実装タスク・ファイル名追従・ログ整備・運用作業・チェックリスト等）
- **管理方法**: 完了したものは消す（履歴は git で追える）。長期保留中のものは「保留」セクションへ
- **新規発生時**: 「課題管理ルール」の判定基準（残作業に文書・仕様の議論が必要か）で判定し、No なら todo.md へ追記（issue 登録不要）

issue を todo.md へ降格する場合は `issue close` の理由欄に「todo.md に移行」と記載し、todo.md 側に転記する。

## 計画の自動レビュー（plan self-review）

`ExitPlanMode` でユーザーへ計画を提示する**前**（承認を得る前）に、計画が `docs/` 配下のファイル（URD/SRS/HLD/LLD/ADR 等）の作成・更新を含む場合のみ、自セッションの計画ファイルに対して `/spec-panel` を実行してセルフレビューを行う（視点の詳細は `.claude/commands/spec-panel.md` 参照）。`docs/` を伴わない計画（コード修正・ツール整備・運用作業等）はスキップする。

**対象パス**: `/spec-panel` の対象は `.claude/plans/<slug>.md`（下記「計画ファイル・handover の扱い」節参照）。

**省略・短縮は禁止**。以下を必ず守ること:

- `/spec-panel` の実行をスキップしない。理由・規模・確信度によらず省略しない
- `/spec-panel` 実行後、指摘記録ファイル（`mgmt/spec-findings/` 配下）が実際に作成されたことを確認してから `ExitPlanMode` を出す。ファイルの存在確認なしに次へ進まない
- 指摘がある場合は計画に反映するか、確認事項としてユーザーに明示してから `ExitPlanMode` を出す

## 計画ファイル・handover の扱い

- **置き場・命名**: `.claude/plans/<slug>.md`。`.claude/settings.json` の `plansDirectory: ".claude/plans"` により plan ファイルは最初からリポジトリ内に生成されるため、**承認後の `mv` は不要**。万一 `~/.claude/plans/`（ホーム配下・グローバル）に生成された場合は設定が効いていないサインなので、異常として報告した上で `mv` で `.claude/plans/<slug>.md` へ移動する。`<slug>` はセッションごとに異なるため、複数セッションが同時に Plan Mode を使っても衝突しない。承認に至らず放棄された下書きが未追跡ファイルとして残っていたら、気づいた時点で削除してよい
- **plan ファイル内のファイル参照はコード表記（バッククォート）にする**: plan ファイルの位置からの相対リンクは閲覧環境によって解決されず broken-link になるため。plan ファイルは lint 対象に含めたまま運用する（隠さない）
- **計画の各タスクに実行モデルを明記する**: グローバル CLAUDE.md「モデルを使い分ける」の3条件に該当し上位モデル（Fable、不可時 Opus）委譲が想定されるタスクに理由を付記する（既定は Sonnet 直接対応のため無印でよい）
- **handover ファイル名の日時**: 必ず `date '+%Y-%m-%d_%H%M'` で実時刻を取得する（推測しない。同日別セッションとの衝突防止）
- **完了時の扱い**: 実装が完了し区切りがついたら `.claude/plans/<slug>.md` を `git rm` で削除しコミットする（履歴は git で追える）。中断・持ち越しで handover を書く場合は残し、次セッションは `<slug>` を含むファイル名とタイムスタンプで対象を特定して再開する

## handover 実行時のルール

handover を書く前に git をクリーンにする（コミットを先に済ませる）。詳細手順は `/handover` スキル参照。

## ドキュメント更新時のルール

ユーザーはドキュメントを GitHub 上で確認しているため、ローカル編集だけでは確認できない。
**ドキュメントの更新が完了した直後（その作業ターン内）に必ず commit する**こと。push は別途指示があるまで不要。

対象ドキュメント:

- `docs/` 配下のすべてのファイル
- `docs/decisions/` 配下の ADR と research 資料
- `ref/SOURCES.md` などの参照資料
- `.claude/plans/*.md`（devel ブランチ運用ファイル。命名規則は「計画ファイル・handover の扱い」節参照）

※ `.claude/best_practices.md` は例外: 上記の手動手順ではなく `/update-best-practices` 実行時にコマンド内で完結する（詳細は「Best Practices（教訓蒸留）運用ルール」参照）。

更新が完了したターン内に: ① `make lint` 警告ゼロを確認 → ② 意図した変更ファイルを個別に `git add`（全対象を確認済みなら `git add -A` 可）→ ③ Conventional Commits でコミット → ④ push は別途指示まで行わない。

例外: 同一作業内でコードと一緒に更新したドキュメントは、コードのコミットに含めて構わない。

## Best Practices（教訓蒸留）運用ルール

@.claude/best_practices.md

上記は `@` インポートによりセッション開始時に毎回自動でコンテキストへ読み込まれる。lessons.md 側は全文注入せず、必要な場面（学び転記の重複チェック等）で都度 Read する運用とする。

- 学びは `.claude/lessons.md` に随時記録する（git 管理外・コミット不要）
- `/update-best-practices`（グローバルコマンド、Fable 実行・利用不可時は Opus）が `.claude/lessons.md` を再分析し、
  `.claude/best_practices.md`（git 管理対象）を再合成する。蒸留観点・原則数の既定と
  watermark 更新・コミットはコマンド側で完結する
  - 本プロジェクトの除外例: dem10b 解像度・openpyxl API 等の技術詳細は原則に含めない
- lessons.md が一定量増えるとセッション開始時に実行が自動的に推奨される（hooks 側で検知）

## 機械的チェック（lint / LSP 等）

`make lint` で全警告ゼロを保つ。

- target 構成（対象拡張子・使用ツール・設定ファイル・除外）は `Makefile` を参照
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

- SOTA 関連資料（山岳リスト summitslist.csv・申請書テンプレート）の出典 URL・備考は [`ref/SOURCES.md`](ref/SOURCES.md) を参照
