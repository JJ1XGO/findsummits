# 欠陥・課題 統合管理システム - Claude Code 運用ガイド

## ファイル構成

```
tracker/
├── track.py          # 統合CLIスクリプト
├── data/
│   ├── bugs.json     # バグデータ
│   └── issues.json   # 課題データ
├── reports/
│   ├── bugs_export.xlsx    # バグExcelエクスポート
│   └── issues_export.xlsx  # 課題Excelエクスポート
└── CLAUDE.md         # このファイル
```

## ステータス遷移（Bug・Issue 共通）

```
未対応 → 対応中 → 対応完了 → 解決済
                              ↑ユーザーが verify コマンドで実施
         ↑Claude が         ↑Claude が
          作業開始時          作業完了時（close コマンド）
```

`却下` はユーザーが対応不要と判断した場合のみ使用。

---

## バグ管理コマンド

```bash
cd /workspace/.claude/mgmt/tracker

# 未解決バグ一覧
python3 track.py bug list --open

# バグの詳細（履歴つき）
python3 track.py bug show BUG-001

# バグを登録（非対話）
python3 track.py bug add --title "..." --severity 高 --stage COD \
  --category 解析エンジン --found_in "src/xxx.c" --cause "src/xxx.c" \
  --found_stage COD --repro "毎回" --description "詳細説明" --resolution "対応方針"

# ステータスを対応中に
python3 track.py bug update BUG-001 --status 対応中 --actor "Claude"

# 修正完了（対応完了 + resolved_date 自動記録）
python3 track.py bug close BUG-001 --actor "Claude" --comment "修正内容"

# ユーザーが解決確認（解決済 + verified_date 自動記録）
python3 track.py bug verify BUG-001 --actor "JJ1XGO" --comment "確認OK"

# サマリー
python3 track.py bug summary

# Excel出力
python3 track.py bug export
```

---

## 課題管理コマンド

```bash
# 未解決課題一覧
python3 track.py issue list --open

# ステージで絞り込み（例: ITステージの課題）
python3 track.py issue list --stage IT

# 課題の詳細（履歴つき）
python3 track.py issue show ISSUE-001

# 課題を登録（非対話）
python3 track.py issue add --title "..." --priority 高 --type 機能追加 \
  --stage COD --category merge.py \
  --description "詳細説明" --resolution "対応方針"

# ステータスを対応中に
python3 track.py issue update ISSUE-001 --status 対応中 --actor "Claude"

# 実装完了（対応完了 + resolved_date 自動記録）
python3 track.py issue close ISSUE-001 --actor "Claude" --comment "実装内容"

# ユーザーが解決確認（解決済 + verified_date 自動記録）
python3 track.py issue verify ISSUE-001 --actor "JJ1XGO" --comment "確認OK"

# サマリー
python3 track.py issue summary

# Excel出力
python3 track.py issue export
```

---

## 自然言語での依頼例

| ユーザーの言葉 | 実行すること |
|---|---|
| 「未解決バグを見せて」 | `python3 track.py bug list --open` |
| 「BUG-001の詳細を見せて」 | `python3 track.py bug show BUG-001` |
| 「未解決課題を見せて」 | `python3 track.py issue list --open` |
| 「ISSUE-001の詳細を見せて」 | `python3 track.py issue show ISSUE-001` |
| 「バグのサマリーを出して」 | `python3 track.py bug summary` |
| 「課題のサマリーを出して」 | `python3 track.py issue summary` |
| 「Excelに書き出して」 | `python3 track.py bug export && python3 track.py issue export` |

---

## 有効な値

| フィールド | 有効な値 |
|---|---|
| status | 未対応, 対応中, 対応完了, 解決済, 却下 |
| severity (Bug) | 高, 中, 低 |
| priority (Issue) | 高, 中, 低 |
| type (Issue) | 機能追加, 改善, 調査, 設計 |
| stage | URD, SRS, HLD, LLD, COD, UT, IT, ST, OPS |
| category | 解析エンジン, merge.py, GeoJSON出力, prefetch, 設定, その他 |

ステージの意味：URD=ユーザー要求定義 / SRS=システム要件定義 / HLD=基本設計 /
LLD=詳細設計 / COD=コーディング / UT=単体テスト / IT=結合テスト / ST=総合テスト / OPS=本番運用
