# 環境異常時のセッション隔離プロトコル＋異常時 lint 省略

## Context（なぜやるか）

前回セッションで「Write/Edit が"成功"表示なのに実ファイル未反映」という汚染が発生し、汚染された可能性のあるセッションでそのまま作業を続けるリスクが顕在化した。ユーザーから2つの運用改善提案があり、両方妥当と判断した：

1. **セッション隔離**: 環境異常を感知したら ①即時インシデント記録 → ②作業再開できるよう handover → ③そのセッションは一旦終了。次セッションで環境確認し、異常がなければ作業継続する。
   - 制約: Claude は自分でセッションを終了できない。ルール化できるのは「異常起因 handover を書いたら**それ以上の実作業をせず、ユーザーにセッション終了を促す**」まで。
2. **異常時 lint 省略**: インシデント記録・handover では lint をかけず、整形は環境が安定した次セッションで行う。異常下で整形に手間をかけるのは本末転倒。

調査で判明した重要事実：
- `make lint` は `git ls-files` ベース＝**git 管理下のみ**。`.claude/incidents/` `.claude/handovers/` は `.gitignore` 済みで**元から対象外**。
- 異常時に lint が「余計な作業」に見えた正体は **`.claude/settings.local.json` の PostToolUse hook**。`*.md` 編集のたびに pymarkdown + lint_docs.py を走らせ違反を additionalContext で自動提示しており、git 管理外でも発火する。

意図する結果: 異常時はノイズなく安全に状態保全して離脱でき、次セッションで健全性を確認してから継続できる。

## 決定事項（ユーザー承認済み）

- **環境確認**: 軽量チェックリストを明文化（新スキルは作らない。毎回手で確認）。
- **lint 抑制**: PostToolUse hook を `.claude/incidents/` `.claude/handovers/` で発火させない（settings.local.json 変更）＋ CLAUDE.md/スキルにルール明記の併用。

## 変更対象ファイルと内容

### 1. `.claude/settings.local.json`（hook 除外）

PostToolUse `Write|Edit` hook の `*.md` 分岐の先頭で、ファイルパスが `.claude/incidents/` または `.claude/handovers/` 配下なら lint をスキップ（`exit 0`）する。

- 現状: `case "$f" in *.md) out1=$(... pymarkdown ...); out2=$(... lint_docs.py ...); ...`
- 変更方針: `*.md)` ブロックに入った直後で `case "$f" in */.claude/incidents/*|*/.claude/handovers/*) exit 0;; esac` を追加（絶対パス・相対パス両対応のため `*/.claude/...` でマッチ）。
- 他の分岐（`*.py` `*.geojson` `*.html`）は変更しない。
- 編集は **1行 case ガードの挿入のみ**。既存の lint コマンド本体は触らない。
- 検証: 変更後、`.claude/incidents/test.md` 相当のパスで hook コマンド断片を手動評価し `exit 0` になること、`docs/foo.md` では従来どおり lint が走ることを確認。

### 2. `/home/node/.claude/CLAUDE.md`（グローバル・「環境異常・インシデント記録」節）

現行は「感知したら即座に記録する」のみ。以下のセッション隔離フローを追記：

- 異常感知 → ①即時インシデント記録（`/log-incident`）→ ②handover で作業状態を保全 → ③**それ以上の実作業をせずユーザーにセッション終了を促す**（Claude は自分で終了できないため）。
- 次セッション冒頭で**環境確認**を行い、異常がなければ作業継続。確認手順は `/log-incident` を参照、と誘導。
- 異常起因のインシデント記録・handover では **lint をかけない**。整形は環境が安定した次セッションで継続作業が終わってから `make lint` で行う、と明記。

### 3. `/home/node/.claude/commands/log-incident.md`（スキル）

- **段階3 完了後** の後ろに新セクション **「段階4 — セッション隔離」** を追加：
  - handover を書いて作業状態を保全（`/handover` 参照）。
  - それ以上の実作業をせず、ユーザーにセッション終了（`/clear` または新規セッション）を促す。
  - 同一セッションでの作業継続はしない（汚染が残っている可能性があるため）。
- 新セクション **「次セッションでの環境確認チェックリスト」** を追加（軽量・手動）：
  1. **書込み反映の照合**: 小さなファイルを Write → `cat` で実体一致を確認（前回症状の再現チェック）。
  2. `git status` で想定外の差分がないか。
  3. 想定外のプロセス／外部コマンド／設定の混入がないか（`env_foreign_orchestrator_commands` の教訓）。
  4. 最新 `.claude/incidents/` を再読し、未解決事項を把握。
  - 全項目クリアなら作業継続。異常が残れば再度インシデント記録。
- **lint 不要の明記**: 段階0〜3 の記録時は lint をかけない。正式フォーマットへの整形（段階3）も「環境が安定した次セッションで行う」と補足（既存の「環境が安定したら」を強化）。

### 4. `/home/node/.claude/commands/handover.md`（スキル）

- 「実行前の準備」または冒頭ルールに **異常起因 handover の特例** を追記：
  - 環境異常を受けて書く handover は、**lint をかけない・整形に時間をかけない**。事実の保全を最優先し、整形は次セッションで継続作業完了後に行う。
  - 通常 handover でも `.claude/handovers/` は lint 対象外（git 管理外・hook 除外済み）であることを明記。

## 影響範囲・非対象

- `make lint` 本体・Makefile は変更不要（元から `.claude/` 配下は対象外）。
- 通常運用の「ドキュメント更新後は即時 commit ＋ lint ゼロ」ルール（tracked な `docs/` `mgmt/`）は維持。本変更は `.claude/incidents/` `.claude/handovers/` のみに作用。
- プロジェクト `/workspace/CLAUDE.md` は変更不要（隔離プロトコルはグローバル運用ルールのため `/home/node/.claude/CLAUDE.md` に集約）。

## 検証

1. **hook 除外**: `.claude/settings.local.json` 編集後、`jq` で JSON 妥当性を確認。`.claude/incidents/xxx.md` 相当のパスを与えて hook の `*.md` 分岐が `exit 0` で抜けること、`docs/xxx.md` では従来どおり lint 出力が出ることを、コマンド断片の手動実行で確認。
2. **ドキュメント整合**: 編集した CLAUDE.md（グローバル）・log-incident.md・handover.md を読み返し、フロー記述に矛盾がないこと、相互参照（`/log-incident`⇄`/handover`）が一致することを確認。
3. **lint**: グローバル `~/.claude/` 配下のファイルはプロジェクト `make lint` 対象外。プロジェクト側で編集するファイルはないため `make lint` 影響なし（念のため `make lint` 警告ゼロを最終確認）。
4. コミット: グローバル `~/.claude/` 配下はプロジェクト git 管理外のためコミット不要。`.claude/settings.local.json` も git 管理外（local 設定）。**本変更でプロジェクトの commit は発生しない**見込み（最終 `git status` で確認）。
