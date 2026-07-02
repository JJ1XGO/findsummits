# best_practices.md 自動注入の実装

## Context

`.claude/lessons.md`（蓄積）→ `/update-best-practices`（更新・蒸留）→ `.claude/best_practices.md`（原則集）というサイクルのうち、「注入」だけが機能していなかった。`session-start.sh` は handover と lessons.md の全文は毎回 cat するが、蒸留成果である best_practices.md 自体は cat しておらず、「lessons.md が+10件増えたら更新推奨」というメッセージにファイル名が出るだけだった。結果として、せっかく蒸留した原則集がセッション冒頭で確実に読まれる保証がなく、CLAUDE.md「根拠主義」節からの参照も Claude 側の明示 Read 任せになっていた。

Fable（設計判断サブエージェント）に相談し、以下の方針で合意した：

- best_practices.md を CLAUDE.md の `@path` インポート構文（Claude Code 公式機能、起動時に読み込まれる）で毎セッション確実に注入する
- 引き換えに lessons.md の全文注入は廃止する（重複注入によるコンテキスト浪費を避ける）。lessons.md は「学び転記の重複チェック」等、必要な場面で明示 Read する運用に切り替える
- ユーザー確認の結果、この方針（lessons全文注入の廃止）とグローバル CLAUDE.md のセッション開始ルーティン文言修正の両方を今回のスコープに含める

## 変更対象

### 1. `/workspace/CLAUDE.md`（プロジェクト）

「Best Practices（教訓蒸留）運用ルール」節に `@.claude/best_practices.md` のインポート行を追加し、best_practices.md が毎セッション自動的にコンテキストへ読み込まれることを明記する。

### 2. `/workspace/.claude/hooks/session-start.sh`

- `echo '## .claude/lessons.md'` と `cat "$ROOT"/.claude/lessons.md` の2行（lessons.md 全文注入）を削除する
- `<<<BEGIN/END AUTO-INJECTED REFERENCE>>>` の対象を handover のみに縮小する（囲みの意図＝プロンプトインジェクション対策のデータ境界表示は維持）
- 「## handover → lessons.md 転記（自律実行）」の指示文を、lessons.md がもう事前注入されない前提に修正する（「上記 handover の…lessons.md と突き合わせ」→「.claude/lessons.md を Read し、上記 handover の…と突き合わせ」）
- watermark 差分による `/update-best-practices` 推奨ロジック（+10件閾値）はそのまま維持する

### 3. `/workspace/.claude/README.md`

hooks 一覧表（51行目）の `session-start.sh` 説明を実態に合わせて更新する：「handover + lessons を注入」→「handover を注入（best_practices.md は CLAUDE.md の @import で常時注入、lessons.md は都度 Read）」。

### 4. `/home/node/.claude/CLAUDE.md`（グローバル、全プロジェクト共通）

「セッション開始時のルーティン」の一文を実態に合わせて修正する：

- 変更前: 「注入された handover と lessons を確認し、今回のタスクに関連するレッスンをユーザーへ共有する」
- 変更後: 「注入された handover と best_practices を確認し、必要に応じて lessons.md を参照した上で、今回のタスクに関連する原則・レッスンをユーザーへ共有する」

このファイルは lint 対象外（`/workspace` の git 管理下ではないため `make lint` は通らない、手動で誤字のみ確認）。

## 検証

1. `bash /workspace/.claude/hooks/session-start.sh` を手動実行し、出力に lessons.md の全文が含まれず、handover のみが BEGIN/END ブロック内に収まっていることを確認する
2. `make lint`（lint-md が `/workspace/CLAUDE.md` と `/workspace/.claude/README.md` を対象に含む）で警告ゼロを確認する
3. `git diff` で各ファイルの変更内容を実物照合する
4. 新規セッション（次回起動時）で best_practices.md の内容がコンテキストに載っているか、ユーザー自身に確認してもらう（このセッション内では起動時注入は再現できないため）

## コミット

ドキュメント更新ルールに従い、このターン内で lint ゼロ確認後にコミットする。対象: `CLAUDE.md`（プロジェクト）、`.claude/hooks/session-start.sh`、`.claude/README.md`（3ファイルは1コミットにまとめる）。グローバル `~/.claude/CLAUDE.md` は git 管理外・別リポジトリ相当のため、変更のみ行いコミットはしない（対象外）。

## 実行モデル

全タスクとも Sonnet 直接対応（機械的な hook・ドキュメント編集であり、アーキテクチャ判断は既に Fable 相談で完了済みのため）。
