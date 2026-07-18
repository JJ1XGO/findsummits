# issue #30 対応: claude-container v3.5.0 リビルド後の実機確認・返信・force push ガード導入

## Context

dotclaude-ops から `findsummits#30` で claude-container v3.5.0（`SECRETS_DIR`/`GIT_PUSH_TOKEN` 対応版）
関連の知見 6 項目が共有された。ユーザーは最新版でのリビルドと `GIT_PUSH_TOKEN` の設定を完了済み。
issue の「お願い」（旧共有トークン依存の解消連絡）への返信と、知見の実機確認が必要。
参考として先行実施プロジェクトの `sotlas-frontend-ops#19`（実機確認結果＋追加知見 2 件）を読了済み。

### 計画フェーズで確認済みの現状（読み取り専用で確認した事実）

- `git config --get credential.helper` は空値を返す（v3.5.0 の実行時リセットが有効。知見 1 の修正込み）
- `~/.git-credentials` は存在しない
- `GIT_PUSH_TOKEN` は `/home/node/.config/claude-container/secrets/noexport/GIT_PUSH_TOKEN` に設定済み
- `gh-token-secondary` と `secrets/GITHUB_MCP_PAT` は同一内容（`diff -q` で確認、生値は非出力）。
  ユーザー確認により**ホスト側で一本化済み（旧共有トークンは新 fine-grained MCP PAT に置換済み）**と確定
- `.mcp.json` はツールセット限定 URL（`/mcp/x/issues`）を使用済み（知見 5 は対応済み）
- findsummits には force push の deny ルール・文書ルールとも未設定（プロジェクト/ローカル/グローバル全層確認済み）
- `env`/`printenv` の Bash 実行はグローバル deny 設定（秘密情報保護）で拒否される。異常ではない

### 知見 6 項目の採否判定

| 知見 | 判定 |
|---|---|
| 1. credential.helper=store バイパス | **実機確認する**（fetch 後の非生成確認が残） |
| 2. SECRETS_DIR と SECONDARY は別機構・一本化配線 | **確認済み・#30 へ回答**（一本化済み） |
| 3. noexport は env に出ないのが正常 | 既知（lessons.md 記録済み）。対応不要 |
| 4. env はランタイム設定・リビルド不要 | 既知（lessons.md 記録済み）。対応不要 |
| 5. MCP サーバー URL のツールセット限定 | 対応済み（`/mcp/x/issues`）。リビルド後の疎通のみ再確認 |
| 6. dotclaude 向け push 配線は不要 | 対応不要（情報として了解） |
| 追加知見 1（permissions.push は実効権限を保証しない） | 情報として了解。今回 PR 作成予定なし・対応不要 |
| 追加知見 2（force push deny の後置見逃し） | **採用**: deny＋文書ルール二重化（ユーザー承認済み） |

## タスク（実行モデル: 全タスク Sonnet 既定・上位モデル委譲なし）

### 1. 実機確認: credential.helper バイパス修正（知見 1）

```sh
git fetch origin
ls -la ~/.git-credentials   # 「存在しない」ままであることを確認
```

### 2. 実機確認: GIT_PUSH_TOKEN によるコンテナ内 push（知見 2 関連）

sotlas-frontend-ops の先行実績と同じ手順:

- テストブランチ `test/git-push-token-verify` を現 HEAD から作成（コミット追加なし）
- `git push origin test/git-push-token-verify` → 成功確認
- `git push origin --delete test/git-push-token-verify` → リモート掃除
- `ls -la ~/.git-credentials` → 非生成を再確認
- ローカルのテストブランチ削除

### 3. リビルド後の MCP 疎通確認（知見 5 関連）

`mcp__github__issue_read` で `jj1xgo/claude-container#24`（既知の open issue）を読み取り、
リビルド後も MCP 配線（`GITHUB_MCP_PAT` export → `.mcp.json`）が機能していることを確認する。
読み取りのみで書き込みテストは行わない（#28 対応時に書き込み済み実績あり）。

### 4. force push ガード導入（deny＋文書ルール二重化）

**4a. `.claude/settings.json`（プロジェクト・git 管理対象）へ deny 追加:**

```json
"permissions": {
  "deny": [
    "Bash(git push --force:*)",
    "Bash(git push -f:*)"
  ]
}
```

- 注: プレフィックス一致のため後置形（`git push origin main --force`）や `git -C <path> push --force`、
  `+refspec` は素通りする（sotlas の発火テストで実証済み）。これを補完するのが 4b の文書ルール
- `--force-with-lease` は `git push --force` プレフィックスに包含されるため個別パターン不要

**4b. `CLAUDE.md`「ルールと制約」節の Git 項へ文書ルール追記:**

- force push（`--force` / `-f` / `--force-with-lease` / `+refspec`、フラグ位置を問わず）は
  実行前に必ずユーザー確認する旨
- deny ルールは前置形のみブロックする（後置形は素通り）ため文書ルールで補完する構造である旨
- 既存の「push は別途指示があるまで不要」運用は GIT_PUSH_TOKEN 設定後もそのまま維持
  （コンテナ内 push が可能になっただけで、push 実行は従来どおり指示ベース）と明記

**4c. deny 発火テスト:**

- settings.json の permissions 変更が同一セッション内で反映されるかをまず確認する
  （グローバル原則 15: 設定反映は再起動単位）
- 反映される場合: sotlas と同手法（`--dry-run`＋存在しないリモート名で無害化）で
  前置形ブロック・無関係コマンド誤検知なしを確認
- 同一セッションで反映されない場合: テスト手順を handover に記載し次セッションで実施
  （原則 5: 機構は発火まで見届ける）

### 5. issue #30 へ返信コメント（自リポジトリ → gh CLI・プライマリトークン）

内容:

- 実機確認結果（タスク 1〜3 の結果を事実ベースで記載）
- 「お願い」への回答: GitHub 公式 MCP サーバーの一本化配線へ移行済み
  （`GH_TOKEN_SECONDARY_FILE` と `SECRETS_DIR` 配下 `GITHUB_MCP_PAT` が同一実体を指す構成・
  ユーザー確認済み）。旧共有トークンへの依存なし
- force push ガード導入（追加知見 2 の採用）を一言報告
- 署名: `— <実装セッションのモデル名> (findsummits)`（経緯説明は書かない）

投稿直前に `--repo jj1xgo/findsummits` を引数レベルで再確認する（プロジェクト best_practices 17）。

### 6. lessons.md 追記（該当分）・コミット

- 新規の学びがあれば `.claude/lessons.md` へ追記（git 管理外）
- `make lint` 警告ゼロ確認 → `CLAUDE.md`・`.claude/settings.json` の変更を
  Conventional Commits でコミット（push はタスク 2 のテストブランチのみ。devel への push は行わない）

## 検証（end-to-end）

- タスク 1・2: `~/.git-credentials` 非生成を実物（`ls -la`）で確認
- タスク 2: `git ls-remote origin` でテストブランチの出現・消滅を実物確認
- タスク 3: MCP ツールの実応答（issue タイトル取得）で確認
- タスク 4: deny 発火テストの実挙動（ブロックメッセージ）で確認（反映単位に依存、上記 4c）
- タスク 5: `gh issue view 30 --repo jj1xgo/findsummits --comments` で投稿反映を実物確認
- `make lint` 警告ゼロ

## 完了時の扱い

全タスク完了後、本計画ファイルを `git rm` で削除しコミットする。
中断時は handover で引き継ぐ（特にタスク 4c が次セッション持ち越しになった場合）。

## スコープ外（今回やらないこと）

- `gh pr create` の実操作検証（追加知見 1 関連。PR 作成の運用が発生した時点で実施）
- findsummits#28 のクローズ（ユーザーの動作確認待ち・既存の持ち越し事項）
- 旧計画ファイル `.claude/plans/reactive-kindling-pumpkin.md` の削除（#28 クローズ後の既存持ち越し事項）
