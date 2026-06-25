# lint ツールの最新版監視（make lint-latest + GitHub Actions）

## Context（なぜ行うか）

lint ツール（ruff/djlint/geojson-validator/pymarkdownlnt）は `requirements.txt` で
バージョン固定している（再構築でルール挙動が変わらないよう＝再現性のため）。
ユーザーは「常に最新版でチェックしたい（セキュリティ意識）」と希望。

技術的整理として、lint ツールは静的解析でネットワークに触れず攻撃面が小さく、固定の主目的は
セキュリティではなく再現性（新ルール追加で「コード未変更でも lint が突然落ちる」を防ぐ）である。
そこで **「ローカル＝固定で安定／CI＝最新版で定期監視」のハイブリッド** を採る。
最新版で新ルールが増えて落ちる時だけ GitHub Actions が検知し、メール通知で気づける。

確定事項（ユーザー合意済み）: 定期実行は月1、workflow は main に置く運用、リポジトリは public。

## 確定方針

- **`requirements.txt` は変更しない**（固定維持。ローカル `make lint` は従来どおり固定版で安定）。
- **`make lint-latest`（新 target）**: lint ツールだけ最新版へ上げてから `make lint` を実行。
  ローカル手動でも CI でも同一コマンド（DRY）。
- **GitHub Actions** がこの `make lint-latest` を月1（cron）＋手動ボタン（workflow_dispatch）で実行。
- Python は 3.13 を明示しローカル（Debian trixie / 3.13）と揃える。

## 実装内容

### 1. `Makefile` に `lint-latest` ターゲット追加

```makefile
# lint ツールを最新版へ上げてからチェック（手動更新確認 + CI 用）。
# requirements.txt の固定は変えない＝ローカルの再現性は維持。
# ローカルで実行すると venv が固定版とズレるため、確認後は make venv-rebuild で復元すること。
lint-latest: venv
    venv/bin/pip install --upgrade ruff djlint geojson-validator pymarkdownlnt
    @$(MAKE) lint
```

`.PHONY` に `lint-latest` を追記。

### 2. `.github/workflows/lint-latest.yml` 新規作成

- トリガー: `schedule`（cron `0 0 1 * *` = 毎月1日 00:00 UTC / JST 9:00）+ `workflow_dispatch`（手動ボタン）
- ジョブ（ubuntu-latest）:
  1. `actions/checkout@v4`
  2. `actions/setup-python@v5`（python-version: '3.13'）
  3. `make lint-latest` を実行（make venv → lint ツール最新化 → make lint）
- C ビルド系（gcc/libpng）は lint に不要なので導入しない。

### 3. ドキュメント更新

- `CLAUDE.md`「機械的チェック」節: 「lint ツールは固定維持。最新版での通過は `make lint-latest`（手動）と
  GitHub Actions（月1自動・main の workflow）で監視する」を追記。
- `docs/01_environment.md`: lint パッケージ表の近くに `make lint-latest` と CI 監視の一文を追記。

### 4. ブランチ運用の段取り

- workflow と Makefile/docs は **devel で作成・コミット**。
- `schedule`・`workflow_dispatch` は GitHub 仕様で **デフォルトブランチ（main）の workflow** が対象。
  既存運用（devel → release/* で mgmt/ 除外 → main マージ）に従い、workflow を main へ反映して初めて
  定期実行・手動ボタンが有効化される（`.github/` は mgmt/ ではないので release 時に除外されない）。
- push / 反映タイミングはユーザー操作（CLAUDE ルール: push は別途指示まで行わない）。

## 対象ファイル

- 新規: `.github/workflows/lint-latest.yml`
- 変更: `Makefile`・`CLAUDE.md`・`docs/01_environment.md`
- 変更しない: `requirements.txt`（固定維持が方針の核）

## 検証

1. ローカルで `make lint-latest` を実行 → 最新版の lint ツールで `make lint` が警告ゼロを確認
   （実行後 `make venv-rebuild` で venv を固定版へ復元し、requirements.txt との一致を担保）。
2. workflow YAML を目視確認（actionlint は未導入のため構文を手検証）。
3. main 反映後、GitHub の Actions 画面から手動ボタン（workflow_dispatch）で初回実行し、成功を確認。

## 補足

- 実装はファイル編集中心（target＋YAML＋docs）で設計判断は本計画で完了 → **Sonnet 推奨**。
- README への Actions バッジ追加は任意。README はユーザー承認のうえでのみ変更（CLAUDE 禁止事項に配慮）。
- `make lint-latest` をローカルで使うと venv が固定版とズレる点に注意（target コメントと docs に明記）。
