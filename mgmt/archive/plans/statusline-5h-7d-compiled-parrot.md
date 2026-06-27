# 環境異常・インシデントの即時記録ルール化

## Context

ユーザーから「環境異常や異変を感知したら、その詳細を記録に残してほしい」という要望。過去に claudebox 由来の外部コマンド混入やモデルの一過性自走・暴走といったインシデントが発生しており（memory: `incident_model_runaway_not_env.md` / `env_foreign_orchestrator_commands.md`）、発生時の生情報が原因究明の決め手だった。

handover まで待つ方式は、暴走系の異常でセッションが正常終了せず handover を書けない／長セッションでコンテキスト圧縮により詳細が薄れる、というリスクがある。そこで **感知した瞬間に専用 Markdown へ即時記録し、handover には要約を参照として残すハイブリッド方式**を採用する（ユーザー合意済み）。

記録場所は `.claude/incidents/`（handover と同列・gitignore 運用。ホストの bind mount 実ディスクに残り Obsidian でも閲覧可）。

## 変更対象（3ファイルへの追記）

1. **`/workspace/.gitignore`** — `.claude/handovers/`（89行目）の直後に `.claude/incidents/` を追加
2. **`~/.claude/CLAUDE.md`**（グローバル横断ルール）— 新セクション「環境異常・インシデント記録（即時）」を追加
3. **`~/.claude/commands/handover.md`** — 引き継ぎノート構成に「環境異常・インシデント」セクションを追加

## 変更内容

### 1. `.gitignore`
```
.claude/handovers/
.claude/incidents/      ← 追加
.claude/bash_history
```

### 2. `~/.claude/CLAUDE.md` に追加するルール（新セクション）

**トリガー（「異常・異変」とみなす例）:**
- 指示なく自走・暴走した（勝手にコミット／handover／再実行などへ走る）
- 想定外の外部コマンド・プロセス・設定の混入を検知
- ツール／コマンドの挙動が不安定・再現性なく失敗する
- コンテナ／権限／ファイルシステムの予期せぬ変化
- その他「これは普通ではない」と感じた挙動

**対応手順:**
1. 感知した時点で**即座に** `.claude/incidents/` に `YYYY-MM-DD_HHmm_<slug>.md` を作成（日時は必ず `date '+%Y-%m-%d_%H%M'` で取得。ディレクトリが無ければ作成）
2. 記載内容: **セッションID（`$CLAUDE_CODE_SESSION_ID` のフル UUID。handover との突合用）** / 発生時刻 / 症状（何が起きたか） / 観測した具体的な出力・エラー / 推測される原因（不明なら「不明」と明記） / とった対応・暫定回避 / 状態（未解決 or 解決済）
3. 記録後、ユーザーに一行で「インシデントを記録しました（ファイル名）」と報告
4. handover 時に「環境異常・インシデント」節へファイル名＋1行要約を転記
5. 原因が判明し恒久知識化できる場合は `memory/incident_*.md`（index 更新）または `mgmt/lessons.md` へ昇華

### 3. `~/.claude/commands/handover.md` の構成に追加

「学び」セクションの後あたりに追加:
```
### 環境異常・インシデント
- 今セッションで `.claude/incidents/` に記録した異常のファイル名＋1行要約。なければ「なし」
```

## 検証

1. `.gitignore` 追記後: `git check-ignore .claude/incidents/` がパスを出力する（＝ignore対象になった）こと
2. 動作確認: `.claude/incidents/` にダミー `.md` を1つ置き `git status` に出ないことを確認 → ダミー削除
3. `~/.claude/CLAUDE.md` と `~/.claude/commands/handover.md` に追記が反映されていること（目視）

## コミットについて

`.gitignore` は git 管理対象なので、変更後 Conventional Commits 形式（本文日本語）でコミットする。`~/.claude/` 配下の2ファイルはリポジトリ外（グローバル設定）のためコミット対象外。
