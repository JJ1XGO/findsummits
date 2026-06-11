# 九州・四国削除 6 件の AZ/50m ゾーン分類分析

## Context

ISSUE-020「手動調査待ちピーク（key_col_resolved=false）の GeoJSON フィーチャ扱い」の議論で、ユーザーが「コル等高線ポリゴンを使った削除判定は範囲が広すぎる」と判断し、代替案として「AZ（Activation Zone, 25m）+ AZ 外で 50m ゾーン内」の二段階削除判定を検討中。

ただし数値（25m / 50m）の妥当性は本州・北海道の独立峰経験がないため不明。前プロジェクト findsummits4sotaja（九州・四国）の削除 6 件で「実際にどう分布していたか」を測ることで、判定ゾーンの構造（AZ 二段階の必要性）を判断したい。

## 分析の目的

6 件の削除ペアそれぞれについて、「統合先（高い側）標高 − minimax 鞍部標高」を計算し、以下のいずれに該当するか分類する:

| 分類 | 条件 | 解釈 |
|---|---|---|
| AZ 内 | `≤ 25m` | SOTA 公式の AZ 概念で吸収可能。matched 扱いで十分 |
| AZ 外 50m ゾーン内 | `25m < x ≤ 50m` | AZ 外 delete 判定ゾーンが必要なケース |
| ゾーン外 | `> 50m` | 50m では拾えない。閾値見直し or 別ロジック必要 |

3 分類の件数分布から、「AZ + AZ 外 50m ゾーン」二段階の必要性を判断する。

## 入力データ

- **削除一覧**: `/workspace/analysis/九州・四国サミット削除一覧.xlsx`
  - 6 ペア（削除サミット ↔ 統合先サミット）
  - 削除側座標: 備考欄の「SOTA緯度経度」から抽出
  - 統合先座標: 備考欄の緯度経度から抽出
  - JA5/TS-109 のみ統合先座標が空 → 別途 SOTA データベース（`/workspace/ref/summitslist.csv`）から JA5/TS-053 大山の座標を取得

- **DEM データ**: `/data/findsummits4sotaja/tiles/`
  - メッシュコード単位の結合 PNG（ファイル名規則: `<meshcode>_<z-x-y>_<z-x-y>.png`）
  - 標高デコード: `elev = (R*65536 + G*256 + B) / 100.0`、無効値 `(128,0,0) → -9999`
  - 各ペアを覆う PNG を特定して読み込む

## 計算アルゴリズム

**minimax 鞍部標高 H の計算**:

- DEM グリッドを無向グラフとして扱う（4 近傍 or 8 近傍）
- 辺の重み = 両端ピクセルの標高の最小値
- 始点（削除サミット位置）と終点（統合先サミット位置）の間の経路で「経路上の最低標高」を最大化する経路の値を求める
- これは Bottleneck Shortest Path 問題

**実装方式（推奨）**:

1. **Dijkstra 変種**（priority queue, max-heap）
   - ノード距離 = 「そのノードに到達するまでの経路の minimum 標高の maximum」
   - 始点から終点まで Dijkstra して終点の距離を取る
   - Python `heapq` + numpy で実装可能、1 ペアあたり 1 秒未満を想定

2. （別解）**Union-Find 段階的接続**
   - 標高の高い順にピクセルを追加、隣接が既存集合と統合
   - 始点と終点が同じ集合になった時点の標高が H
   - こちらの方が直感的だが Dijkstra で十分

**領域サイズ**:
- 各ペアを覆う矩形領域 + 適度なマージン（10%）で十分
- 通常は 1 メッシュ（約 80km × 80km）内に収まる
- ピクセル数で数十万 〜 数百万

## 出力

`analysis/keycol_threshold_analysis.csv`（または .md）:

| pair_id | 削除ID | 統合先ID | 削除側標高 | 統合先標高 | minimax鞍部標高H | 統合先 − H | 分類 |
|---|---|---|---|---|---|---|---|
| 1 | JA6/KG-157 | JA6/KG-197 | 484 | 478 | (計算結果) | (計算結果) | (分類結果) |
| ... | ... | ... | ... | ... | ... | ... | ... |

備考:
- 「統合先 − H」を主指標とするが、参考に「削除側 − H」（=低い方のプロミネンス相当）も出す
- 統合先標高が削除側より低いケース（KG-157, KM-093）の扱いは結果を見て判断

サマリーとして:
- 3 分類の件数集計
- 最大値・平均値・分布

## 実装計画

新規スクリプト: `analysis/keycol_threshold_analysis.py`

1. xlsx 読み込み → 6 ペア抽出（openpyxl）
2. 統合先空欄ケースは `ref/summitslist.csv` から座標補完
3. 座標 → タイル特定（`/data/findsummits4sotaja/tiles/` のファイル名から該当 PNG を選択）
4. 標高グリッド読み込み（PIL or 既存の C コードと同じデコード処理を Python で）
5. minimax 鞍部標高を Dijkstra 変種で計算
6. CSV 出力 + サマリー print

依存パッケージ:
- openpyxl（既存）
- Pillow（PNG デコード）
- numpy（グリッド演算）
- heapq（標準ライブラリ）

実装規模: 200-300 行程度。

## 検証方法

1. スクリプト実行: `venv/bin/python3 analysis/keycol_threshold_analysis.py`
2. 出力 CSV を確認:
   - 6 ペア全件の H と分類が出ているか
   - 視覚的確認のため、興味深いケース（JA6/KG-054 → JA6/KG-195、JA6/FO-043 → JA6/FO-092 など）の H を地理院地図で目視確認
3. 結果から判断:
   - 全件 AZ 内 → AZ 二段階は不要、AZ 一本で十分
   - AZ 外 50m ゾーン内が一定数 → AZ 外 delete 判定ゾーン導入を ISSUE-020 plan に組み込む
   - ゾーン外が出る → 50m では不足、閾値再検討 or 別ロジック検討

## ISSUE-020 議論への接続

本分析の結果を踏まえて:
- AZ 外 delete 判定ゾーンの必要性が確認できれば、ISSUE-020 plan の方向性として「広域モードはコル特定のみ（案 D''''の発展形）+ AZ/50m ゾーン判定」が固まる
- パラメータ化する閾値の妥当範囲（例: 25m / 50m が妥当か、別の値に調整するか）の根拠になる
- 本州・北海道で外挿可能かは別途検証必要（事実認識として plan に明記）

## 関連ファイル

### 入力
- `/workspace/analysis/九州・四国サミット削除一覧.xlsx`
- `/data/findsummits4sotaja/tiles/*.png`
- `/workspace/ref/summitslist.csv`

### 新規作成
- `/workspace/analysis/keycol_threshold_analysis.py` — 分析スクリプト
- `/workspace/analysis/keycol_threshold_analysis.csv` — 出力

### 参照
- `/data/findsummits4sotaja/findsummits/analyzePng.py` — PNG デコード処理の参考
- `/workspace/src/elevation.c` — 現プロジェクトの PNG デコード仕様
- `/workspace/mgmt/plan_2026-05-20_issue-020-discussion.md` — 前セッションの議論経過
