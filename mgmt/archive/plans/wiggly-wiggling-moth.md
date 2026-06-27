# SessionStart フック導入: セッション開始ルーティンの自動実行

## Context（なぜやるか）

グローバル CLAUDE.md の「セッション開始時のルーティン（必須）」は、毎セッション開始時に
(1) 最新 handover を読む (2) lessons.md を読む (3) 関連レッスンを共有、と無条件で定めている。
しかし今セッション冒頭で、私（Claude）はこのルーティンを実行せず clarification に飛んでしまった。

メモリ（feedback 型）に記録する手段は「私が読んで思い出して従う」soft な仕組みで、私の裁量に依存し
再発の余地が残る。そこでハーネス（Claude Code 本体）が実行する **SessionStart フック**を導入し、
セッション開始時に handover + lessons を**自動でコンテキストへ注入**する。これにより「読み忘れる」
という選択肢自体が消える（私を経由しない強制）。ユーザーは「始めます」だけで済むようになる。

ユーザーの選択（AskUserQuestion 回答）: **SessionStart フック（推奨）**。

## 変更対象ファイル

- `/workspace/.claude/settings.local.json`（**新規キー追加のみ**。既存の permissions / sandbox は保持）

### なぜ settings.local.json か（settings.json ではなく）

- handover 置き場 `.claude/handovers/` は `.gitignore` 登録済み（machine-local）
- lessons は `mgmt/lessons.md`。`mgmt/` は **devel ブランチのみ**（main では存在しない）
- これらを参照するフックを共有の `settings.json`（リポジトリにコミットされ main にも乗る）へ置くと、
  main ブランチや他環境でパスが存在せず無意味に走る。`settings.local.json`（gitignore 対象）が正しい置き場。

## 追加する内容（設計確定済み）

`settings.local.json` に以下の `hooks` キーを追加する:

```json
"hooks": {
  "SessionStart": [
    {
      "matcher": "startup|resume|clear",
      "hooks": [
        {
          "type": "command",
          "command": "H=$(ls -t /workspace/.claude/handovers/*.md 2>/dev/null | head -1); { echo '# セッション開始ルーティン（自動注入: handover + lessons）'; echo '※ 開始ルーティンを満たすため自動注入。関連レッスンがあれば作業前にユーザーへ共有すること。'; echo; echo \"## 最新 handover: ${H##*/}\"; cat \"$H\" 2>/dev/null; echo; echo '## mgmt/lessons.md'; cat /workspace/mgmt/lessons.md 2>/dev/null; }"
        }
      ]
    }
  ]
}
```

設計上のポイント:
- **matcher**: `startup`（新規）・`resume`（再開）・`clear`（/clear 後）で発火。compact は除外（要約継続中は不要）
- **出力**: SessionStart フックの stdout はセッションコンテキストに追加される。最新 handover 全文 + lessons.md 全文（計 ~17KB）を注入
- **冒頭の指示行**: 注入物が「開始ルーティンの自動実行」であること・関連レッスンの共有義務を私に明示する
- **堅牢性**: handover が無い／lessons が無い場合も `2>/dev/null` で静かに空出力（エラーにしない）

## 実装手順（Plan 承認後）

1. `update-config` skill を使って `settings.local.json` に上記 `hooks` キーを追加する
   （hook スキーマの検証・JSON エスケープを skill に任せる。直接 Edit でも可だが skill が安全）
2. 既存の `permissions` / `sandbox` キーが保持されていることを確認する

## 検証方法

1. `cat /workspace/.claude/settings.local.json` で JSON が壊れていない・既存キー保持を目視確認
2. `venv/bin/python3 -c "import json; json.load(open('/workspace/.claude/settings.local.json'))"` で JSON 妥当性チェック
3. フックの command 部分を手動実行し、最新 handover + lessons が出力されることを確認:
   `bash -c "H=\$(ls -t /workspace/.claude/handovers/*.md | head -1); cat \"\$H\"; cat /workspace/mgmt/lessons.md"`
4. **最終確認は次セッション**: 新しいセッションを開始し、handover + lessons が自動でコンテキストに
   現れる（私が「始めます」だけで handover を把握済みになる）ことを確認する

## このタスク後の予定（本計画には含めない）

- **FR-018 レビュー**（ユーザーの本来の主タスク）を再開する。Opus 視点の指摘と、ユーザー提起の
  「入力に merged_peak.csv が無くて問題ないか」論点を 1 論点ずつ相談形式で進める。
  ※ 暫定確認では ADR-SRS-004 / FR-014 が「広域 L14 でもピーク座標は L15 座標を保持」と明記しており、
    座標突き合わせは設計上保証される見込み（要・本レビューで精査）。
