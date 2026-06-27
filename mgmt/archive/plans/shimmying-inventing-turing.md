# ISSUE-079 todo 移行・ISSUE-062 以降のステージ情報補正

## Context

前回コミット（`fc9cb8c chore(mgmt): 課題管理と ToDo リストの管理を分離`）で課題管理と todo.md の分離整備を実施したが、以下の補正が必要：

1. **ISSUE-079** (preprocess_pref_boundaries.py FR-017 追従) を境界線として「Issue 残し」にしたが、内容を再確認すると **仕様は確定済みで実装作業のみ**。todo.md に移行する。
2. 現在 **SRS ステージ中** に登録した ISSUE-062 以降の未対応 Issue について、発生ステージが未設定または別ステージになっているものがある。**発生ステージを SRS に統一**する。
3. 各 Issue の **「対応予定ステージ」（どこまでに対応すれば良いか）** が未設定。個別に設定する。

## 修正対象

### 1. ISSUE-079 を todo.md へ移行

- `track.py issue update ISSUE-079 --status 却下 --actor "Claude" --comment "todo.md に移行（運用ルール改訂: 仕様議論を伴わない実装タスクは todo.md 管理）"`
- `mgmt/todo.md` の高優先セクションに以下を追記:

```
- [ ] (元 ISSUE-079) scripts/preprocess_pref_boundaries.py を FR-017 改訂版仕様に追従
  - ZIP 自動検出方式（$DATA_DIR/ref/ を N03-(\d{8})_GML\.zip で走査、YYYYMMDD 最大を採用）
  - ZIP 内市区町村版 GeoJSON のみを一時ディレクトリに展開して処理、処理後削除
  - dissolve 出力を 60 地域（46 都府県 + 14 振興局）に修正
  - params/config.ini の n03_year 設定参照を廃止
  - 採用 ZIP 名・YYYYMMDD・サイズをログに出力
  - 北方領土除外タイルリスト生成（ADR-SRS-018: NORTHERN_CODES = {01695..01700}）
  - 仕様詳細: docs/02_SRS.md FR-017・7.1.3・7.2.1・ADR-URD-005
```

### 2. ISSUE-062 以降の未対応 Issue の発生ステージを SRS に統一

対象 10 件（ISSUE-079 を除く未対応分）:
- ISSUE-062, ISSUE-063, ISSUE-066, ISSUE-070, ISSUE-071, ISSUE-072, ISSUE-073, ISSUE-074, ISSUE-075, ISSUE-076

実行: `track.py issue update ISSUE-XXX --stage SRS --actor "Claude" --comment "発生ステージを現行 SRS ステージに統一"`

### 3. 対応予定ステージの個別設定

**判定基準**（ユーザー指示）: SRS に書かれており HLD/LLD に影響せずプログラム修正だけで良ければ COD、HLD/LLD で記述事項があるなら該当ステージ。

| ID | タイトル要約 | 対応予定 | 判定理由 |
|---|---|---|---|
| ISSUE-062 | FR-012 実装: merged_summit_revised.xlsx | **HLD** | ブラウザ内生成方針に確定（SRS 修正済み）。HTML ビューア側の設計が HLD 領域 |
| ISSUE-063 | FR-021 実装: 申請エビデンス ZIP 生成 | **HLD** | ブラウザ内 JSZip 実装。HTML ビューアのボタン・統合設計が HLD 領域 |
| ISSUE-066 | FR-019 検索機能実装 | **HLD** | HTML ビューア UI 機能。検索方式・対象プロパティ・サジェストの設計が HLD 領域 |
| ISSUE-070 | 等高線レイヤー描画パラメータ確定（HLD） | **HLD** | HLD 段階で方針確定する設計判断 |
| ISSUE-071 | 標高タイル選択・フォールバック実装方針（HLD） | **HLD** | HLD 段階で方針確定する設計判断 |
| ISSUE-072 | Leaflet pane 構成と重ね順の整理（HLD） | **HLD** | HLD 段階で構成決定 |
| ISSUE-073 | attribution 制御方式の確定（HLD） | **HLD** | HLD 段階で方式確定 |
| ISSUE-074 | Phase 構造の再編 | **SRS** | SRS 改訂で完結 |
| ISSUE-075 | FR-004/FR-014 責務分担変更 + ADR-SRS-004 更新 | **SRS** | SRS と ADR 改訂で完結 |
| ISSUE-076 | FR-022 出力明示 + フェーズ遷移制御の所在 | **SRS** | SRS 改訂で完結 |

実行: `track.py issue update ISSUE-XXX --planned_stage XXX --actor "Claude" --comment "対応予定ステージを設定"`

### 4. Excel レポート再生成

`venv/bin/python3 mgmt/tracker/track.py issue export --if-changed`

### 5. コミット

対象ファイル: `mgmt/todo.md` / `mgmt/tracker/data/issues.json` / `mgmt/tracker/reports/issues_export.xlsx` / `mgmt/plan.md`

メッセージ案:
```
chore(tracker): ISSUE-079 todo 移行・ISSUE-062 以降のステージ情報補正

- ISSUE-079 (preprocess_pref_boundaries.py FR-017 追従) を todo.md へ移行
- ISSUE-062 以降の未対応 Issue 10 件の発生ステージを SRS に統一
- 各 Issue に対応予定ステージを設定（HTML ビューア系/HLD 確定系=HLD、SRS 改訂系=SRS）
```

## 検証

- `track.py issue list --open` で未対応 Issue が **22 件**（23 - ISSUE-079）になる
- `track.py issue show ISSUE-XXX` で発生ステージ・対応予定ステージが反映されていることを確認（サンプル: ISSUE-062 / 070 / 074 で各カテゴリ 1 件ずつ）
- `mgmt/todo.md` の高優先セクションに ISSUE-079 由来項目が追加されている
- `git status` でクリーン状態

## 影響範囲

- ドキュメント・トラッカーデータのみ（コード変更なし）
- 既存 ID は維持
- 棚卸し済み Issue / バグ件数は変動するが、判定基準（文書議論の有無）は前回コミットで明文化済み
