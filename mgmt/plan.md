# 計画: docs/ 内 FR/UR/NFR 裸参照の一括リンク化と lint 検査追加

## Context

`docs/CLAUDE.md` L108 のルール: 本文中の FR/UR/NFR 参照は Markdown リンクで記述する。
現状 ~250-350 件の違反が存在し `make lint` で検出されていない。
対策は2段階: ①既存違反を一括修正、② lint 検査D 追加で以降の違反を機械的に防ぐ。
以後は既存の PostToolUse hook + `make lint` パイプラインが自動強制する。

## 調査で確認した事実

- FR 22件・NFR 10件: `docs/20_SRS.md` に `#### FR-NNN: タイトル` / `### NFR-NNN: タイトル` 形式
- UR 13件: `docs/10_URD.md` に HTML アンカー `<a id="ur-NNN">` 形式 → スラッグは単に `#ur-NNN`
- 既存リンク形式（53件確認済み）:
  - `docs/decisions/*.md` → SRS: `../20_SRS.md#fr-nnn-スラッグ` 形式
  - `docs/20_SRS.md` 内: 同一ファイルアンカー `#fr-nnn-スラッグ` 形式
  - UR 参照: `#ur-nnn` または `../10_URD.md#ur-nnn` 形式
- コードブロック（` ``` `）内の俯瞰図は除外、コードブロック外はリンク化対象とする

## GitHub slug 変換ルール

```python
s = heading_text.lower()
s = re.sub(r'[^\w\s-]', '', s, flags=re.UNICODE)  # 日本語は残す、:（）× 等を除去
s = re.sub(r'\s+', '-', s)
s = re.sub(r'-+', '-', s).strip('-')
# 例: "FR-004: 3×3メッシュ結合解析オーケストレーション" → "fr-004-33メッシュ結合解析オーケストレーション"
```

## タスク一覧

### T1: 一括修正スクリプト作成・実行（Sonnet）

新規: `scripts/fix_bare_refs.py`

処理フロー:

1. `docs/20_SRS.md` から FR/NFR 見出しをパースし slug マップを生成
2. `docs/10_URD.md` から `<a id="ur-NNN">` をパースし UR マップを生成
3. `docs/` 配下の全 .md を処理（`mgmt/archive/` 除外と同様に archive 系は除外）
4. 各ファイルで「リンクプレフィックス」を決定:
   - `docs/decisions/research/*.md` → SRS: `../../20_SRS.md`, URD: `../../10_URD.md`
   - `docs/decisions/*.md` → SRS: `../20_SRS.md`, URD: `../10_URD.md`
   - `docs/*.md`（SRS/URD 自身以外）→ SRS: `20_SRS.md`, URD: `10_URD.md`
   - `docs/20_SRS.md` 内の FR/NFR → プレフィックスなし（同一ファイルアンカー）
   - `docs/20_SRS.md` 内の UR 参照 → `10_URD.md#ur-nnn`
   - `docs/10_URD.md` 内の UR → プレフィックスなし（同一ファイルアンカー）
   - `docs/10_URD.md` 内の FR/NFR 参照 → `20_SRS.md#fr-nnn-スラッグ`
5. 行単位処理:
   - フェンスコードブロック（` ``` `）内はスキップ
   - 見出し行（`#` で始まる）はスキップ
   - 各行をトークン分割: `[TEXT](URL)` / `` `code` `` / それ以外 の3種
   - 「それ以外」部分のみ bare ref を置換
6. スラッシュ連鎖の処理:
   - `FR-NNN/MMM/PPP`（prefix なし後続）→ 各要素に prefix を補完してから個別リンク化
   - `FR-NNN/FR-MMM`（prefix あり）→ 各要素を個別にリンク化

dry-run オプション（`--dry-run`）で変更プレビュー後に本実行。

### T2: lint 検査D を `scripts/lint_docs.py` に追加（Sonnet）

`check_file()` に検査D を追加（T1 と同じトークン分割ロジックを共有関数化）:

```text
{file}:{line}: BARE-REF: {ID} はリンク化してください（docs/CLAUDE.md L108）
```

- 検査対象: docs/ 配下のみ（既存の `_apply_check_c` 条件と同じ）
- 除外: フェンスコードブロック内・見出し行・インラインコード内・既存リンク内
- スラッシュ連鎖も検出対象（分割後の各 ID が未リンクなら違反）

### T3: 検証・コミット（Sonnet）

1. `--dry-run` で変更サンプルを確認（10件程度）
2. 本実行
3. `make lint` 警告ゼロを確認
4. `git add -A && git commit`（docs/ と scripts/ を含む）

## 実行モデル

全タスク **Sonnet**（直線的なファイル処理・正規表現実装で設計判断不要）

## 検証

- `make lint` 0 violations
- git diff で変更ファイル数・行数を確認し意図外の変更がないこと
- 変更後の SRS・代表的 ADR を開いてリンクの見た目を確認
