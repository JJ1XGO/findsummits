# 計画: SRS セクション3・FR-009・FR-013 の設計変更（アクティベーションエリア導入）

**ステータス: 実装完了**（docs/02_SRS.md に反映済み）

## Context

SRS レビュー中に以下の設計変更が決定した。

主な変更点:
1. セクション3 アーキテクチャ図: 3ステップ運用フロー・フェーズ番号追記
2. FR-009 マッチング方式: tolerance_px 廃止 → アクティベーションエリア Flood Fill 方式
3. FR-013 GeoJSON フィーチャ構成: アクティベーションエリアポリゴン追加（TBD-03 解消）、フェーズ4 → フェーズ3 へ移動（merge.py が担当）
4. FR-016（新規）: アクティベーションエリア計算（findsummits C エンジンの新機能）

---

## 変更 A: セクション3 アーキテクチャ図（3ステップ運用フロー・フェーズ番号追記）

```
【ステップ1: タイル取得】
prefetch_tiles.py    タイル事前取得（フェーズ1）
       ↓
【ステップ2: 解析・突合・確認】（GeoJSON/HTML で結果を確認してから次ステップへ）
findsummits (C)      山頂・コル検出・アクティベーションエリア計算（フェーズ2）
       ├─ per-mesh CSV              ($DATA_DIR/results/csv/<meshcode>.csv)
       ├─ per-peak activation area  ($DATA_DIR/results/csv/<meshcode>_activation.geojson)
       └─ 標高地形図                ($DATA_DIR/images/<meshcode>_terrain.png)
merge.py (Python)    SOTA 突合・差分分類（フェーズ3）
       ├─ merged.csv         ($DATA_DIR/results/merged.csv)
       ├─ merged.geojson     ← 目視確認用 GeoJSON
       └─ merged_viewer.html ← 目視確認用 静的 HTML ビューア
【ステップ3: 申請書生成】（確認済みの場合のみ実行）
output.py (Python)   申請書 XLSX 生成（フェーズ4）
       └─ submission.xlsx
```

---

## 変更 B: FR-009 マッチング方式変更（tolerance_px 廃止）

### 旧方式（廃止）
Chebyshev 距離 `tolerance_px` による pixel座標マッチング。
`tolerance_px = 0` はピーク検出精度確認を優先するための意図的な設定だったが、
精度確認が完了したためより高精度なアクティベーションエリア方式に刷新。

### 新方式: アクティベーションエリア Flood Fill
- findsummits（C）: Flood Fill で `elev ≥ peak_elev − 25.0m` の連続エリアをポリゴン出力
- merge.py（Python）: point-in-polygon 判定で SOTA サミットと照合
- 一意性: プロミネンス≥150m の制約により 1 アクティベーションエリア内に複数サミットは不在

---

## 変更 C: FR-013 GeoJSON フィーチャ構成（TBD-03 解消）

フェーズ4 から フェーズ3（merge.py 担当）に移動。アクティベーションエリアポリゴン追加。

| match_status | フィーチャ |
|---|---|
| matched | Point（ピーク）+ Polygon（アクティベーションエリア）+ Point（Keyコル）+ Point（SOTA サミット）+ LineString×2 |
| new | Point（ピーク）+ Polygon（アクティベーションエリア）+ Point（Keyコル）+ LineString（ピーク→Keyコル） |
| deleted | Point（SOTA サミット）のみ |

---

## 次のタスク（保留中）

- **BUG-014 verify**（後回し）: findsummits 再実行 → analysis_count 分布確認（4/6/9 のみ） → `python3 mgmt/tracker/track.py bug verify BUG-014 --actor "JJ1XGO"`
