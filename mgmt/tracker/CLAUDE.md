# 欠陥・課題 統合管理システム - Claude Code 運用ガイド

## Issue と Bug の概念定義

### Issue（課題）

**プロジェクトの仕様・設計・調査・新機能に関するもの**を登録する。
文書（URD / SRS / HLD / LLD / ADR / GLOSSARY 等）と紐づく議論を伴うものが対象。

`type` の使い分け:

| type | 意味 | 例 |
|---|---|---|
| 機能追加 | 新規 FR や新規スクリプトの追加 | FR-XXX 新設、新コマンドの追加 |
| 改善 | 既存仕様の変更・拡張・整理 | SRS の章立て見直し、既存 FR の挙動変更 |
| 調査 | 何かを決めるための情報収集・分析 | UR 対応漏れ調査、データ分布の実測 |
| 設計 | 実装方針・アーキテクチャの判断 | フェーズ配置、アルゴリズム選定 |

### Bug（バグ）

**動作不良・欠陥**を登録する。仕様通りに動かない・期待と異なる挙動を示すもの。

`severity` の目安:
- **高**: 結果が信用できなくなる（誤検出・欠落）/ 完走しない
- **中**: 結果は正しいが効率や使い勝手に支障
- **低**: ログ・コメント・命名等の品質改善余地

### 登録すべきでない例（→ `mgmt/todo.md` へ）

以下は **issue にも bug にも登録しない**。`mgmt/todo.md` で管理する:
- 単独のファイル修正（関数名リネーム・コメント整理 等）
- ログ書式の統一・ログ追記・ファイル名追従（仕様議論を伴わないもの）
- スクリプトのオプション追加（仕様議論を伴わないもの）
- セットアップ手順整備・運用作業・チェックリスト

**判定基準: 文書・仕様の議論を伴うか？**
- Yes → `issue` / `bug`
- No  → `mgmt/todo.md`

詳細は `/workspace/CLAUDE.md` の「課題管理ルール」「ToDo リスト運用ルール」を参照。

---

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

# Excel出力（無条件）
python3 track.py bug export

# Excel出力（bugs.json が xlsx より新しい場合のみ再生成）
python3 track.py bug export --if-changed
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

# Excel出力（無条件）
python3 track.py issue export

# Excel出力（issues.json が xlsx より新しい場合のみ再生成）
python3 track.py issue export --if-changed
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
| 「Excelに書き出して（強制）」 | `python3 track.py bug export && python3 track.py issue export` |
| 「Excelに書き出して（変更時のみ）」 | `python3 track.py bug export --if-changed && python3 track.py issue export --if-changed` |

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
