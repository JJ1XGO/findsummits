# Opus/Sonnet 使い分けルールの明文化

## Context

「計画は Opus、実装は Sonnet」のルールは現状 auto memory（`feedback_model_switching.md`）にしか書かれておらず、グローバル/プロジェクト `CLAUDE.md` には記載がない。memory は関連性が高いと判断されたときに参照される位置付けで、常時コンテキストに入る `CLAUDE.md` と違って参照漏れが発生しやすい。これが直近「ルールが守られていない」原因と推定される。

前セッション（15:36 作成のplan `opus-sonnet-prancy-feather.md`）でユーザー選択により **グローバル `CLAUDE.md` (`/home/node/.claude/CLAUDE.md`) の「コア原則」に新項目として明文化** する方針が確定済み。全プロジェクト共通で常時コンテキストに載るため、最も強制力が高い。ホスト再起動でセッションが切れたため、本セッションでその plan を承認・実装に進める。

ユーザーからの追加指示（前セッションでの合意事項）:
1. グローバル CLAUDE.md に書くなら、memory にある重複は **削除する**（情報源の二重化を避ける）
2. 実装は必ず Sonnet ではなく、**品質確保上 Opus の方が良いと判断した場合はその旨を申し出る** 運用にする

本セッションでの追加確認事項:
- `/workspace/mgmt/tracker/CLAUDE.md` を実物確認 → モデル名記入欄・担当者関連の記述は **無し**
- よって memory 末尾の「バージョン番号なし表記」ルールの移管先は `/workspace/CLAUDE.md` の「タスク管理の流れ」に追記する方法で確定

## 変更内容

### 1. グローバル `CLAUDE.md` への追記

ファイル: `/home/node/.claude/CLAUDE.md`

「コア原則」セクションの末尾（既存 `### 4. 学びを活かす` ブロックの直後）に以下を追加。既存 1〜4 の番号は変更しない。

```markdown
### 5. モデルを使い分ける
Plan Mode で計画が承認・確定したら、**実装に着手する前にユーザーに `/model` で Sonnet への切り替えを促す** ことを原則とする。
Opus は調査・設計・判断フェーズ用。ファイル編集中心の単純な実装で Opus を使うのは無駄。

ただし、実装内容が以下のいずれかに該当し **品質確保のため Opus 継続が望ましい** と判断した場合は、その旨を理由付きで申し出る：
- 実装中に追加の設計判断・トレードオフ評価が頻繁に発生しそうな場合
- 複雑なアルゴリズム実装・難解なデバッグが想定される場合
- 仕様の解釈に揺れがあり、コーディング中も随時判断が必要な場合

`ExitPlanMode` が承認された直後、ファイル編集を始める前に以下のいずれかを 1 行で伝える：
- 通常: 「実装は Sonnet に切り替えますか？」
- Opus 継続推奨時: 「この実装は <理由> のため Opus 継続を推奨しますが、いかがしますか？」

計画ファイルが引き継ぎに足る詳細度（対象ファイルパス・コード/設定ドラフト・検証手順）になっていることを、提案前に確認しておく。
```

### 2. memory ファイルの削除

ファイル: `/home/node/.claude/projects/-workspace/memory/feedback_model_switching.md`

**ファイル本体を削除** し、`/home/node/.claude/projects/-workspace/memory/MEMORY.md` の index 行（`- [計画は Opus・実装は Sonnet に切り替える](feedback_model_switching.md) — Plan 承認後、実装着手前に /model 切り替えを促す`）も削除する。

### 3. memory に残っていた運用補足の移管

memory には以下の細則も書かれていた:

> トラッカーで担当者にモデル名を記入する場合は **バージョン番号なしで「Sonnet」「Opus」とだけ** 書く（バージョンアップのタイミングをユーザー側で追えないため。2026-05-20 指示）

確認結果: `/workspace/mgmt/tracker/CLAUDE.md` にはモデル名記入欄・担当者関連の記述が存在しない（本セッションで `grep` 確認済み）。

→ `/workspace/CLAUDE.md` の「タスク管理の流れ」セクションの末尾に 1 行追記する。文面案:

```
- トラッカーで担当者にモデル名を記入する場合はバージョン番号なしで「Sonnet」「Opus」とだけ書く（バージョンアップ追従の手間を避けるため）
```

## 修正対象ファイル

| ファイル | 変更内容 | リポジトリ |
|---|---|---|
| `/home/node/.claude/CLAUDE.md` | 「コア原則」末尾に「5. モデルを使い分ける」を追加 | 外（コミット不要） |
| `/home/node/.claude/projects/-workspace/memory/feedback_model_switching.md` | ファイル削除 | 外 |
| `/home/node/.claude/projects/-workspace/memory/MEMORY.md` | 対応する index 行を削除 | 外 |
| `/workspace/CLAUDE.md` | 「タスク管理の流れ」末尾にモデル名表記ルールを 1 行追記 | 内（要コミット） |

## 検証

- `/home/node/.claude/CLAUDE.md` を Read し、追加内容が「コア原則」直下に入っていること（5. のブロックが存在）
- `/home/node/.claude/projects/-workspace/memory/MEMORY.md` から `feedback_model_switching` 行が消えていること
- `/home/node/.claude/projects/-workspace/memory/feedback_model_switching.md` ファイル本体が消えていること（`ls` で確認）
- `/workspace/CLAUDE.md` の「タスク管理の流れ」末尾にバージョン番号なし表記ルールが追記されていること
- 次回 Plan Mode を使うセッションで、`ExitPlanMode` 直後に Claude 側から自発的に「Sonnet 切替を促す or Opus 継続を理由付きで申し出る」発話が出るかを観察する
- プロジェクト `CLAUDE.md` 変更分のコミット（Conventional Commits 形式・本文日本語）

## 旧 plan ファイルとの関係

前セッションで書かれた `/home/node/.claude/plans/opus-sonnet-prancy-feather.md` と内容はほぼ同等。本セッションでは実物確認による移管先確定（tracker CLAUDE.md 不採用 → プロジェクト CLAUDE.md 採用）の差分のみ。旧 plan ファイルは履歴として残置で問題なし（自動生成名のため命名上の衝突なし）。
