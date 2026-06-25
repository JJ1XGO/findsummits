# Python 静的解析（Ruff）導入計画

## Context（背景・目的）

本リポジトリには Python が 12 ファイル・約 3,845 行ある（`mgmt/tracker/track.py` 1,013 行、
`scripts/` 本番系、`analysis/` 検証系）が、機械的チェックは Markdown（`make lint` → `lint-md`）
のみで **Python 用の静的解析はゼロ**。型ヒントも部分的で品質がばらついている。

コア原則 #7（機械的チェックの警告ゼロ）を Python にも適用するため、Python リンタを `make lint`
に組み込み、未使用 import・未定義名・import 順序・バグパターン等を恒常的に検出できるようにする。

**ツール選定: Ruff**（Astral 製・Rust 製の高速リンタ）。根拠:
pip 1 パッケージで既存 venv 運用に乗る／flake8・pyflakes・isort・pyupgrade 等を1本に統合／
段階導入（rule select・per-file-ignores）が容易／`ruff server` でエディタ LSP も兼ねる／
`target-version = py313` 対応。型チェックは Ruff の対象外のため**別軸として今回はスコープ外**。

## 確定した方針（ユーザー合意済み）

- **導入範囲: リント（`ruff check`）のみ**。`ruff format`（自動整形）は今回入れない（初回の大差分回避）
- **ルール強度: 標準セット** = `F, E, W, I, B, UP`
  （pyflakes / pycodestyle / isort / flake8-bugbear / pyupgrade）
- **型チェック（mypy / pyright / ty）は別途あとで判断**（今回スコープ外）

## 変更対象ファイル

### 1. `requirements.txt` — Ruff を追加（バージョン固定）

末尾の lint セクションに、pymarkdownlnt と同じ「バージョン固定」方針で 1 行追加する。

```text
# lint（開発ツール: python 静的解析用。ランタイム import はしない。
# 再構築でルール挙動が変わらないようバージョン固定）
ruff==<実装時の最新安定版を pin>
```

> 実装時に `venv/bin/pip install ruff` で入る版を確認して固定する（pymarkdownlnt の前例に倣う）。

### 2. `ruff.toml`（リポジトリ root 新規作成）

`pyproject.toml` が存在しないため、独立した `ruff.toml` を新規作成する（最小・明示的）。

```toml
# Ruff 設定。対象は本プロジェクトの Python 3.13。
target-version = "py313"
line-length = 100          # track.py 等の対話 UI を踏まえ E501 は緩めに（要実装時調整）

[lint]
select = ["F", "E", "W", "I", "B", "UP"]
# 既存コードの実情に応じて、ゼロ化が過大な規則のみ per-file-ignores / ignore で個別緩和する
# （黙って消さず、緩和は理由をコメントで明示。恒久判断が要るものは ADR 化を検討）

[lint.per-file-ignores]
# 例: 必要に応じて analysis/ や track.py に限定的な緩和を置く（実装時に違反実測後に確定）
```

### 3. `Makefile` — `lint-py` ターゲット追加 & `lint` 集約へ連結

`lint-md`（`Makefile:63-68`）と同じ構造で `lint-py` を追加し、`lint:`（`Makefile:57`）の依存に足す。
`.PHONY`（`Makefile:70`）にも `lint-py` を追記。

```makefile
lint: lint-md lint-py

# Python lint（チェックのみ・ファイルは書き換えない）。
# 既定対象: git 管理下の全 .py。LINT_PY_PATHS 指定時はそのパスを対象にする（override）。
LINT_PY_PATHS ?=
lint-py: venv
    @if [ -n "$(LINT_PY_PATHS)" ]; then targets="$(LINT_PY_PATHS)"; \
    else targets=$$(git ls-files '*.py' ':!:mgmt/archive/**'); fi; \
    venv/bin/python3 -m ruff check $$targets
```

> `ruff check` は設定を `ruff.toml` から自動参照する。出力既定は `full`。
> CI 連携が将来必要なら `--output-format github` を検討（今回は不要）。

### 4. `.claude/settings.local.json` — PostToolUse hook を `.py` にも拡張（任意・推奨）

現状 hook（`settings.local.json:26`）は `.md` 限定。`.md` は従来どおり、`.py` 編集時は
`ruff check` を走らせて違反を Claude に提示する分岐を追加する（Markdown と同じ即時フィードバック体験）。
※ 既存 `.md` 分岐の挙動は壊さないこと。実装時に拡張子で分岐する形へ書き換える。

### 5. 文書更新（実装と同一作業ターン内で更新・コミット）

ドキュメント・実装整合性原則に従い、ツール構成変更を文書に反映する。

- **`CLAUDE.md`（プロジェクト, 「機械的チェック」節）**:
  「現在の構成: lint-md（pymarkdown + scripts/lint_docs.py、mgmt/archive/ 除外）」を
  **lint-md + lint-py（ruff、mgmt/archive/ 除外）** に追記更新する（既存運用＝ lint 構成は ADR でなく
  本節へインライン記録、に倣う）。
- **`docs/01_environment.md`（「Python パッケージ」表）**:
  `ruff`（用途: python 静的解析 / `make lint`）の行追加を検討。
  ※ 現在この表は requests/openpyxl/shapely/numpy の4行のみで、**既存の pillow・pymarkdownlnt が未掲載**
  （runtime 中心で dev ツール未記載という既存の不整合）。実装時にユーザーへ「dev ツール（pymarkdownlnt
  含む）も表に載せる方針か」を確認し、方針に合わせて ruff（＋必要なら pymarkdownlnt/pillow）を追記する。
- **ADR は作成しない**: lint ツール構成は本プロジェクトでは CLAUDE.md にインライン記録する運用のため。
  Ruff は事実上の標準で重い設計判断を伴わない。
- **`mgmt/lessons.md`**: 事前更新不要。実装中に学び（規則の誤検知・緩和判断等）が出たら追記する。

## 実装手順（承認後）

1. `venv/bin/pip install ruff` → 入った版を確認し `requirements.txt` に pin、`ruff.toml` を作成
2. `venv/bin/python3 -m ruff check $(git ls-files '*.py')` で **現状の違反を実測**
   - `--statistics` で規則別の件数を集計し、件数・内訳をユーザーに一覧提示する
3. 自動修正可能なもの（主に `I`/`UP`/一部 `E`）は `ruff check --fix` で解消、
   手修正分は内容を確認しながら対応。**警告ゼロまで**持っていく（コア原則 #7）
   - ゼロ化が過大／意図的に残す規則があれば、`ignore`・`per-file-ignores` に理由コメント付きで設定。
     恒久的な設計判断を伴う緩和は ADR 化を検討（docs/CLAUDE.md 準拠）
4. `Makefile` に `lint-py` を追加し `lint` へ連結、`.PHONY` 更新
5. PostToolUse hook を `.py` 対応に拡張
6. **文書更新**: `CLAUDE.md`「機械的チェック」節を更新、`docs/01_environment.md` の Python パッケージ表に
   ruff を追記（dev ツール掲載方針をユーザー確認のうえ）。`make lint` 警告ゼロを確認し、
   コード・設定・文書をまとめてコミット（ドキュメント更新後は即時 commit のルール準拠）
7. 違反件数が多く一度に潰しきれない場合は、ユーザーと相談のうえスコープ分割
   （`select` を `F, E9` から段階拡張する等）を再検討

## 検証

- `make lint` が **lint-md と lint-py の両方を実行**し、警告ゼロで `exit 0` になること
- 故意に違反（未使用 import 等）を入れたファイルで `make lint` が非ゼロ終了することを確認
- `.py` を Edit/Write したとき PostToolUse hook が違反を提示し、`.md` 編集時の従来挙動が不変であること
- `make venv-rebuild` 後も `ruff` が requirements.txt から復元され `make lint` が通ること

## 留意点

- 標準セットは初回修正がやや多め。手順 2 の実測で件数を見てから本格修正に入る（無理なら段階導入へ）
- `ruff format` と型チェッカ（pyright / mypy / ty）は今回スコープ外。必要になれば別計画で追加
- バージョン固定運用・新規パッケージは同コミットで requirements.txt 追記（既存ルール踏襲）
