# track.py: ID 引数を「数字だけ」でも受け付ける

## Context（なぜやるか）

課題管理は「1課題1登録」に整理した結果、日常操作は `show / update / close / verify` に
ID を渡す場面が多い。現状は `ISSUE-121` / `BUG-007` のようにプレフィックス＋3桁ゼロ埋めの
完全形を毎回タイプする必要があり、`121` や `7` で済ませたいというのが要望。
bug 側も同様。

ゴール: `track.py issue show 121` で `ISSUE-121` を、`track.py bug close 7` で `BUG-007` を
引けるようにする（完全形 `ISSUE-121` / `BUG-7` も従来どおり受け付ける＝後方互換）。

## 現状の把握（調査済み）

- ID 生成: `next_id()`（track.py:75-77）が `f"{prefix}-{counter:03d}"`。prefix は `BUG` / `ISSUE`、3桁ゼロ埋め
- ID 照合: `show/update/close/verify` の各関数が `next((x for x in data[...] if x["id"] == args.id), None)` で
  生文字列一致（bug: track.py:312,421,468 ほか / issue: 654,750,795 ほか）
- どのコマンドが bug か issue かは `args.kind`（`"bug"` / `"issue"`）で判別可能（main: track.py:983-985）
- ID を位置引数 `id` で取るのは `show / update / close / verify` の4コマンド（list/add/summary/export は対象外）

## 方針

照合箇所を個別に直さず、**`main()` で `args.id` を一度だけ正規化**する。注入点が1つで済み、
既存関数群（約12箇所の `args.id` 参照）には一切手を入れない。

### 変更1: 正規化ヘルパー追加（`next_id` の近く、track.py:77 直後あたり）

```python
def normalize_id(raw, kind):
    """ID引数を正規形に整える。'121'→'ISSUE-121'、'BUG-7'→'BUG-007' など。
    解釈できない入力はそのまま返し、後段の『見つかりません』処理に委ねる。"""
    prefix = {"bug": "BUG", "issue": "ISSUE"}.get(kind)
    if prefix is None or raw is None:
        return raw
    s = str(raw).strip()
    # 末尾の数字部分を取り出す（'ISSUE-121' / 'issue 121' / '121' いずれも 121 を得る）
    m = re.search(r'(\d+)\s*$', s)
    if not m:
        return raw
    return f"{prefix}-{int(m.group(1)):03d}"
```

- `import re` が未 import なら冒頭（`import argparse` 付近, track.py:29）に追加
- 完全形 `BUG-7` を渡しても `BUG-007` に正規化されるので、桁ズレ入力も救える副次効果あり

### 変更2: `main()` で適用（track.py:994 の dispatch 呼び出し直前）

```python
    fn = args.dispatch.get(args.cmd)
    if fn:
        if getattr(args, "id", None) is not None:
            args.id = normalize_id(args.id, args.kind)
        fn(args)
```

- `args.id` を持つのは ID 系4コマンドのみ。それ以外は `getattr` がデフォルトで素通り
- `args.kind` は `bug` / `issue` のどちらか（ここに来る時点で確定済み）

## 対象ファイル

- `mgmt/tracker/track.py`（唯一の変更対象。ヘルパー追加 + main 2行 + 必要なら import 1行）
- `mgmt/tracker/CLAUDE.md`（任意）: コマンド例に「ID は数字だけでも可（`issue show 121`）」の一文を追記

## 検証手順

実データ（issues.json に ISSUE-121 等が実在）で読み取り系を使い、JSON を壊さず確認する。

```bash
# 数字だけ → 正規形に解決されること
venv/bin/python3 mgmt/tracker/track.py issue show 121      # ISSUE-121 が表示される
venv/bin/python3 mgmt/tracker/track.py bug show 1          # BUG-001 が表示される（存在すれば）

# 完全形が従来どおり動くこと（後方互換）
venv/bin/python3 mgmt/tracker/track.py issue show ISSUE-121

# 桁省略の完全形も救えること
venv/bin/python3 mgmt/tracker/track.py issue show ISSUE-121   # = issue show issue-121 と同結果

# 存在しないIDで従来どおり『見つかりません』になること
venv/bin/python3 mgmt/tracker/track.py issue show 99999
```

`update/close/verify` は JSON を書き換えるため、検証は `show` で代表させる
（照合ロジックは4コマンド共通で `args.id` 一致のため、show が通れば他も同じ経路）。

## 補足

- 後方互換のため既存の handover / CLAUDE.md 内の `ISSUE-001` 形式の記述は修正不要（そのまま動く）
- 仕様議論を伴わない単独ファイルのオプション改善のため、issue 登録ではなく todo/直接実装の範疇
