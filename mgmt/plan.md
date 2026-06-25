# GeoJSON / HTML の lint 導入

## Context（なぜ行うか）

コア原則 #7（機械的チェックの警告ゼロ）を Markdown・Python に続き **GeoJSON・HTML** へ広げる。
現状 `make lint` は `lint-md`・`lint-py` のみで、リポジトリ内の GeoJSON（13ファイル）と
HTML（2ファイル）は機械チェックの対象外。手編集・生成スクリプト出力の構造崩れを未然に検出したい。

ツール選定は前々回セッション（b9ffa0ba）で一度合意しかけたが、ツール不調で混乱したため仕切り直す。
本計画で現状を実地確認した結果は以下:

- **Java は未インストール**（vnu を使うなら JRE 追加＝コンテナ再ビルドが必要）
- **HTML 対象は2ファイルのみ**（`docs/mockup/viewer_mockup.html`・`docs/decisions/research/dem_colormap.html`）
- GeoJSON 対象は git 管理下13ファイル（`ref/geojson_v31/` 10・`analysis/` 1・`docs/decisions/research/` 2）

## 確定方針

- **GeoJSON = `geojson-validator` 0.6.0**（Pure Python・追加ランタイム不要）。
  CLI が無い（Web版のみ）ため、`lint_docs.py` に倣ったラッパー `scripts/lint_geojson.py` を新規作成する。
  API: `validate_structure()`（RFC7946 構造）+ `validate_geometries()`（未閉合・巻き順・座標範囲外・自己交差等）。
  依存の shapely 系は既存 `requirements.txt` にあり。
- **HTML = 実データ検証してから最終決定**（ユーザー選択）。候補は `djlint` 1.39.4（Pure Python・`--lint`）。
  対象2ファイルのために Java JRE を足すコスト感が論点。検証結果次第で「djlint 採用 / vnu(Java) へ切替 / 見送り」を決める。

## 実装の段取り（2フェーズ）

### フェーズ1: 実データ検証（ExitPlanMode 承認後に最初に実行）

1. venv に試験インストール: `geojson-validator` と `djlint`（この時点では requirements.txt に固定しない）
2. **GeoJSON**: ラッパーの試作で全13ファイルを検証。
   - 特に `ref/geojson_v31/*.geojson`（SOTA 公式・外部由来＝自分で直せない）に違反が出るかを確認。
   - 違反ゼロ → 全13を対象に含める。違反が出る → 外部由来分を対象から除外する方針に切替（範囲を最終決定）。
3. **HTML**: `djlint --lint --profile html` で2ファイルを検証し、インライン `<style>`/`<script>` 由来の
   誤検知の量・種類を観察（H006/H021/H030 等）。
4. 検証結果（誤検知の実態・抑制で実用になるか）をユーザーに提示し、**AskUserQuestion で HTML ツールを最終決定**。
   - djlint で実用十分 → djlint 採用（`--ignore` / `pyproject.toml` で誤検知抑制）
   - 誤検知過多で抑制困難 → vnu(Java) へ切替（`docs/01_environment.md`・packages.txt に JRE 追加を別途相談）
   - 価値に見合わない → HTML lint は見送り（GeoJSON のみ導入）

### フェーズ2: 本実装（HTML ツール確定後）

既存の `lint-md`/`lint-py` と同一パターンで統合する。

1. **`requirements.txt`**: `geojson-validator==0.6.0`（＋ HTML 採用時 `djlint==1.39.4`）をバージョン固定で追記。
2. **`scripts/lint_geojson.py` 新規作成**: `scripts/lint_docs.py` の構造（argv でファイル受領・違反を stdout・
   exit 0/1）を踏襲。`validate_structure` + `validate_geometries` を呼び、違反を `path:行: 種別: 内容` 形式で出力。
3. **`Makefile`**:
   - `lint:` 依存に `lint-geojson`（＋採用時 `lint-html`）を追加。
   - `LINT_GEOJSON_PATHS ?=` / `lint-geojson:` ターゲットを `lint-md` と同形で定義
     （未指定時は `git ls-files '*.geojson' ':!:mgmt/archive/**'`、override 対応）。
   - HTML 採用時は `lint-html:` も同形で追加（`djlint --lint`）。
   - `.PHONY` に追記。
4. **`.claude/settings.local.json` の PostToolUse hook**: 現行 `case` 文に `*.geojson)` /（採用時）`*.html)`
   分岐を追加し、編集時に該当 linter を自動実行。
5. **既存違反ゼロ化**: `make lint` を実行し、検出された違反を修正 or（外部由来等で妥当なら）対象除外・抑制ルール化。
6. **ドキュメント・コミット**:
   - `docs/01_environment.md`（パッケージ表に追記、vnu 採用時のみ JRE）
   - `CLAUDE.md`「機械的チェック」節に `lint-geojson`（採用時 `lint-html`）を記載
   - Conventional Commits・本文日本語で一括コミット（docs はターン内 commit ルールに従う）

## 対象ファイル

- 新規: `scripts/lint_geojson.py`
- 変更: `requirements.txt`・`Makefile`・`.claude/settings.local.json`・`docs/01_environment.md`・`CLAUDE.md`
- 参考（流用元パターン）: `scripts/lint_docs.py`（自前チェッカー雛形）、`Makefile` の `lint-md`/`lint-py`

## 検証

1. `make lint`（または `make lint-geojson` / `make lint-html` 単体）が警告ゼロで終了（exit 0）。
2. 意図的に壊した GeoJSON / HTML を一時作成 → 各 linter が違反を検出し exit 1 になることを確認（検出力の確認）。
3. PostToolUse hook: `.geojson`/`.html` を編集して違反が自動提示されることを確認。
4. `make venv-rebuild` で venv == requirements.txt の一致を担保。

## 補足

- 実装フェーズはファイル編集中心（ラッパー＋Makefile＋hook＋docs）で設計判断は本計画でほぼ完了 → **Sonnet 推奨**。
  ただしフェーズ1の誤検知判断・HTML 最終決定はユーザーと相談しながら進める。
- `.claude/settings.local.json` はプロジェクトローカル設定。グローバル設定には触れない。
