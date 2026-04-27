# 課題管理システム - Claude Code 運用ガイド

## ファイル構成

```
issue-system/
├── issue.py        # CLIツール（メインスクリプト）
├── data/
│   └── issues.json   # 課題データ（唯一のデータソース）
├── reports/
│   └── issues_export.xlsx  # エクスポート先
└── CLAUDE.md       # このファイル（Claude Codeへの指示）
```

---

## ステータス遷移

```
未対応 → 対応中 → 対応完了 → 解決済
                              ↑ユーザーが verify コマンドで実施
         ↑Claude が         ↑Claude が
          実装開始時          実装完了時（close コマンド）
```

`却下` はユーザーが対応不要と判断した場合のみ使用。

---

## よく使うコマンド

```bash
# 未解決課題一覧
python3 issue.py list --open

# ステージで絞り込み（例: ITステージの課題）
python3 issue.py list --stage IT

# 課題の詳細（履歴つき）
python3 issue.py show ISSUE-001

# 課題を登録（非対話）
python3 issue.py add --title "..." --priority 高 --type 機能追加 \
  --stage COD --category merge.py \
  --description "詳細説明" --resolution "対応方針"

# ステータスを対応中に
python3 issue.py update ISSUE-001 --status 対応中 --actor "Claude"

# 実装完了（対応完了 + resolved_date 自動記録）
python3 issue.py close ISSUE-001 --actor "Claude" --comment "実装内容"

# ユーザーが解決確認（解決済 + verified_date 自動記録）
python3 issue.py verify ISSUE-001 --actor "JJ1XGO" --comment "確認OK"

# サマリー
python3 issue.py summary

# Excel出力
python3 issue.py export
```

## 自然言語での依頼例

| ユーザーの言葉 | 実行すること |
|---|---|
| 「未解決課題を見せて」 | `python3 issue.py list --open` |
| 「ISSUE-001の詳細を見せて」 | `python3 issue.py show ISSUE-001` |
| 「課題のサマリーを出して」 | `python3 issue.py summary` |
| 「Excelに書き出して」 | `python3 issue.py export` |
| 「ISSUE-001を対応中にして」 | `python3 issue.py update ISSUE-001 --status 対応中 --actor "Claude"` |
| 「ISSUE-001の実装が完了した」 | `python3 issue.py close ISSUE-001 --actor "Claude" --comment "..."` |

## 有効な値

| フィールド | 有効な値 |
|---|---|
| status | 未対応, 対応中, 対応完了, 解決済, 却下 |
| priority | 高, 中, 低 |
| type | 機能追加, 改善, 調査, 設計 |
| stage | URD, SRS, HLD, LLD, COD, UT, IT, ST, OPS |
| category | 解析エンジン, merge.py, GeoJSON出力, prefetch, 設定, その他 |

ステージの意味：URD=ユーザー要求定義 / SRS=システム要件定義 / HLD=基本設計 /
LLD=詳細設計 / COD=コーディング / UT=単体テスト / IT=結合テスト / ST=総合テスト / OPS=本番運用
