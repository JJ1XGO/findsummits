# バグ管理システム - Claude Code 運用ガイド

## ファイル構成

```
bug-system/
├── bug.py          # CLIツール（メインスクリプト）
├── data/
│   └── bugs.json   # バグデータ（唯一のデータソース）
├── reports/
│   └── bugs_export.xlsx  # エクスポート先
└── CLAUDE.md       # このファイル（Claude Codeへの指示）
```

---

## 【最重要】バグ発見時の運用ルール

**バグ・欠陥を発見しても、すぐに修正してはならない。必ず以下のフローに従うこと。**

### ステータス遷移

```
未対応 → 対応中 → 対応完了 → 解決済
                              ↑ユーザーが verify コマンドで実施
         ↑Claude が         ↑Claude が
          修正開始時          修正完了時（close コマンド）
```

`却下` はユーザーが対応不要と判断した場合のみ使用。

### Claudeが守るべき手順

1. **テスト実行・結果確認**
   - 複数のバグが見つかっても、1件ずつ都度登録しない
   - 確認が完了してから、発見したバグを**まとめて一覧提示**
   - ユーザーに確認を取ってから `add` コマンドで登録

2. **バグ登録**（`add` コマンド）
   - 原因がわかっている場合は `--resolution` に対応方針まで記入してから登録
   - 登録後にユーザーへ報告し、修正作業開始の承認を得る

3. **修正作業開始**
   - `python3 bug.py update BUG-XXX --status 対応中 --actor "Claude"` でステータス更新

4. **修正完了**（`close` コマンド）
   - `python3 bug.py close BUG-XXX --actor "Claude" --comment "修正内容の説明"` を実行
   - → ステータスが「対応完了」、`resolved_date` が自動記録される
   - ユーザーに確認依頼のメッセージを送る

5. **ユーザーが確認完了**（`verify` コマンド）
   - ユーザーが実施：`python3 bug.py verify BUG-XXX --actor "JJ1XGO" --comment "確認OK"`
   - → ステータスが「解決済」、`verified_date` が自動記録される

---

## よく使うコマンド

```bash
# 未解決バグ一覧
python3 bug.py list --open

# バグの詳細（履歴つき）
python3 bug.py show BUG-001

# バグを登録（非対話）
python3 bug.py add --title "..." --severity 中 --category 解析エンジン \
  --found_in "src/xxx.c" --cause "src/xxx.c" --repro "毎回" \
  --description "詳細説明" --resolution "対応方針"

# ステータスを対応中に
python3 bug.py update BUG-001 --status 対応中 --actor "Claude"

# 修正完了（対応完了 + resolved_date 自動記録）
python3 bug.py close BUG-001 --actor "Claude" --comment "修正内容"

# ユーザーが解決確認（解決済 + verified_date 自動記録）
python3 bug.py verify BUG-001 --actor "JJ1XGO" --comment "確認OK"

# サマリー
python3 bug.py summary

# Excel出力
python3 bug.py export
```

## 自然言語での依頼例

| ユーザーの言葉 | 実行すること |
|---|---|
| 「未解決バグを見せて」 | `python3 bug.py list --open` |
| 「BUG-001の詳細を見せて」 | `python3 bug.py show BUG-001` |
| 「バグのサマリーを出して」 | `python3 bug.py summary` |
| 「Excelに書き出して」 | `python3 bug.py export` |
| 「BUG-001を対応中にして」 | `python3 bug.py update BUG-001 --status 対応中 --actor "Claude"` |
| 「BUG-001の修正が完了した」 | `python3 bug.py close BUG-001 --actor "Claude" --comment "..."` |

## バグをJSONに直接追加するときのテンプレート

```json
{
  "id": "BUG-NNN",
  "title": "",
  "description": "",
  "reporter": null,
  "found_date": "YYYY-MM-DD",
  "category": null,
  "found_in": null,
  "repro": null,
  "severity": "中",
  "status": "未対応",
  "assignee": null,
  "stage": null,
  "cause": null,
  "resolution": null,
  "notes": null,
  "resolved_date": null,
  "verified_date": null,
  "history": []
}
```

## 有効な値

| フィールド | 有効な値 |
|---|---|
| status | 未対応, 対応中, 対応完了, 解決済, 却下 |
| severity | 高, 中, 低 |
| stage | URD, SRS, HLD, LLD, COD, UT, IT, ST, OPS |
| category | 解析エンジン, merge.py, GeoJSON出力, prefetch, 設定, その他 |

ステージの意味：URD=ユーザー要求定義 / SRS=システム要件定義 / HLD=基本設計 /
LLD=詳細設計 / COD=コーディング / UT=単体テスト / IT=結合テスト / ST=総合テスト / OPS=本番運用
