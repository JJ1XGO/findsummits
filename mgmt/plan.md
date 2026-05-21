# ISSUE-020: delete判定ゾーンポリゴン導入

## Context

### 経緯

ISSUE-020 元タイトル「FR-013/FR-014: 手動調査待ちピーク GeoJSON フィーチャ扱い」の議論が、九州・四国削除 6 件の minimax 鞍部標高分析（main_Δ 100〜178m）の結果を受けて以下の根本設計変更に発展した:

- **旧方針**: コル等高線ポリゴン（`col_elev` 以上の Flood Fill）で削除判定
  - 課題: 独立峰（プロミネンス 500m+）でポリゴンが日本全土を覆う問題
  - 対症療法: ガード条件「既存 SOTA プロミネンス > 500m なら削除除外」が必要

- **新方針**: **delete判定ゾーンポリゴン**（プロミネンス連動 + 250m 上限キャップ）
  - 等価式: `max(col_elev, peak_elev - 250m)` 以上の Flood Fill
  - 独立峰問題が 250m キャップで自然解消
  - ガード条件（恣意的な 500m 値）が不要になる
  - 250m 上限により delete判定ゾーンは常に 3×3 メッシュで完結する

### 議論の結論

| 論点 | 結論 |
|---|---|
| ポリゴン名 | **delete判定ゾーン** (delete-determination zone) |
| Flood Fill 閾値 | `max(col_elev, peak_elev - 250m)` 以上 |
| 250m の根拠 | 暫定（感覚値）。実データ検証で確定: `150m + abs(SOTA標高 - DEM標高).max() + 余裕、切り良い整数` |
| 250m の管理場所 | `params/config.ini` でパラメータ化 |
| 判定方向 | 既存サミット座標がいずれかのピークの delete判定ゾーン内にあるか（**座標のみ・標高無関係**） |
| 既存サミットの状態 | matched (AZ 内) / delete (delete判定ゾーン内 + AZ 外) / **unmatched (どちらでもない)** |
| unmatched 発生時 | **merge.py を non-zero exit で停止、FR-013 (GeoJSON/HTML) は実行しない** |
| 不備の表現 | merged.csv に**不備種別ごとの別列**を追加 |
| 主ピーク特定 | ADR-008 維持（プロミネンス最小ピーク） |
| 広域解析の役割 | コル特定のみ。delete判定ゾーンは 3×3 で完結するため広域不要 |
| FR-014 トリガー | `key_col_resolved=false` は維持。`area_complete=false` は **AZ のみに限定** |

---

## 新ロジック概要

### peak.match_status（ADR-007 維持、コル等高線→delete判定ゾーンに置換）

| 状態 | 条件 |
|---|---|
| matched | ピークの AZ 内に既存 SOTA サミットが存在 |
| dominant | ピークの delete判定ゾーン内に既存 SOTA サミットが存在（AZ 外） |
| new | いずれにも該当せず、プロミネンス ≥ 150m |

### summit.match_status

| 状態 | 条件 |
|---|---|
| matched | いずれかのピークの AZ 内 |
| delete | いずれかのピークの delete判定ゾーン内（AZ 外） |
| **unmatched** | いずれにも該当しない（**エラー、処理中止**） |

### 主ピーク特定（ADR-008 維持）

delete 候補サミットを含む delete判定ゾーンのピーク候補が複数の場合、**プロミネンス最小のピークを主ピーク**とする。

---

## 進め方: ハイブリッド（大筋先 + 250m だけ実データ検証先）

仕様優先原則と「実データ検証を上流に組み込む」のバランスを取る。

```
Step 1: 大筋を SRS / ADR に書く（Phase A 実行）
        - 250m は "delete_zone_max_drop" パラメータ名だけ書く
        - 値は "TBD（Phase B で確定）" と明記
        - 不備フラグ列はリスト化（実装で追加発見されたら追記）

Step 2: ユーザーレビュー（SRS / ADR 構造の合意）

Step 3: 250m パラメータ値の実データ検証（Phase B 実行）
        - analysis/sota_dem_elevation_diff.py 作成・実行
        - SOTA 日本支部全サミットで abs(SOTA-DEM) 分布調査
        - 値確定（150 + max(abs差) + 余裕、切り良い整数）

Step 4: SRS の TBD 部分だけ確定値に更新

Step 5: Phase C 以降の実装（C エンジン → merge.py → output_geojson.py）
```

### 判断根拠

| 観点 | ハイブリッドでの扱い |
|---|---|
| 構造の手戻りリスク | 議論で固めたので小。先行可能 |
| 数値の手戻り | 250m 値が変わっても SRS の文章は不変。`config.ini.example` 数値更新のみ |
| 仕様優先原則 | 守れる（実装着手は SRS 構造合意後） |
| 実データ検証の上流組み込み | 値だけは検証後に書くので満たす |

---

## 実装フェーズ分割

### Phase A: SRS 改修（250m は TBD で記述）

| 対象 | 改修内容 |
|---|---|
| `docs/02_SRS.md` FR-009 | summit.match_status に `unmatched` 追加、エラー停止仕様明記。AZ 内優先（match と delete 衝突時は match）を明記 |
| `docs/02_SRS.md` FR-013 | dominant/new フィーチャ構成の「コル等高線ポリゴン」→「delete判定ゾーンポリゴン」。merge.py 異常時は実行しない（exit code 経由） |
| `docs/02_SRS.md` FR-014 | `area_complete=false` トリガーを AZ のみに限定。広域モードで delete判定ゾーンは生成しない |
| `docs/02_SRS.md` FR-016 | 「コル等高線ポリゴン」を削除し「delete判定ゾーンポリゴン」を追加。Flood Fill 閾値 `max(col_elev, peak_elev - delete_zone_max_drop)` に変更 |
| `docs/decisions/ADR-007` | 影響確認（terminology 変更が必要かレビュー） |
| `docs/decisions/ADR-008` | 影響確認（主ピーク特定アルゴリズムは維持できるか） |
| `docs/decisions/ADR-NNN` | 新規 ADR: 「delete判定ゾーンポリゴン採用」決定 |
| `docs/00_GLOSSARY.md` | 「delete判定ゾーン」用語追加、「コル等高線ポリゴン」削除 |

### Phase B: 250m パラメータ値の実データ検証

| 内容 | ファイル |
|---|---|
| 検証スクリプト作成（read-only 解析） | `analysis/sota_dem_elevation_diff.py`（新規） |
| SOTA 日本支部全サミット（JA で約 1,500 件）の座標から DEM 標高取得 | 同上 |
| `abs(SOTA標高 - DEM標高)` の分布調査 | 同上 |
| 250m の最終確定（暫定式: `150 + max(abs差) + 余裕、切り良い整数`） | 結果を ADR に追記 |

### Phase C: C エンジン側の Flood Fill 閾値変更

| 対象 | 改修内容 |
|---|---|
| `src/analyze.c` または `src/mesh_analyze.c` | Flood Fill 閾値を `max(col_elev, peak_elev - delete_zone_max_drop)` に変更 |
| `src/mesh_analyze.c` | コル等高線ポリゴン生成削除、delete判定ゾーンポリゴン生成追加 |
| `params/config.ini.example` | `delete_zone_max_drop = 250` パラメータ追加 |
| `src/*.c` の config 読み込み | `delete_zone_max_drop` 値を C 側に渡す経路追加（既存の config 読み込み経路に合流） |

### Phase D: merge.py 改修

| 対象 | 改修内容 |
|---|---|
| `scripts/merge.py` | AZ ポリゴン読み込み + point-in-polygon 実装（shapely 利用） |
| 同上 | delete判定ゾーンポリゴン読み込み + point-in-polygon 実装 |
| 同上 | 不備フラグ列追加（種別ごとに別列） |
| 同上 | unmatched サミット検出時に non-zero exit |
| 同上 | dominant 判定の追加（peak.match_status） |
| 同上 | 既存の `match_status` 値拡張: `matched` / `dominant` / `new` / `deleted` → `delete` |

### Phase E: FR-013 (output_geojson.py) 改修

| 対象 | 改修内容 |
|---|---|
| `scripts/output_geojson.py` | merged.csv の不備フラグ確認、不備行があれば実行スキップ（merge.py の exit code で前段で止まるが二重チェック） |
| 同上 | dominant フィーチャ追加（コル等高線 → delete判定ゾーン） |
| 同上 | `merged_viewer.html` 出力追加（SRS の FR-013 で要求済みだが現状未実装） |

---

## merged.csv 不備フラグ列（暫定スキーマ）

既存の `match_status` / `stability` 列に加えて以下の bool 列を追加:

| 列名 | 意味 |
|---|---|
| `is_unmatched_summit` | 既存サミットが AZ にも delete判定ゾーンにも入らない |
| `is_area_incomplete` | ピークの AZ が解析範囲外で途切れ（FR-014 広域でも解消せず） |
| `is_key_col_unresolved` | ピークの Keyコルが未確定（FR-014 広域でも解消せず） |
| `is_out_of_range_summit` | 既存サミット座標が解析対象メッシュ範囲外 |
| `is_dem_invalid_summit` | 既存サミット座標の DEM が NODATA / 海面 |

merge.py 終了時に上記いずれかが true の行があれば exit code 1 で終了する。

---

## 修正対象ファイル（絶対パス）

### SRS / ADR / GLOSSARY
- `/workspace/docs/02_SRS.md`（FR-009 / FR-013 / FR-014 / FR-016）
- `/workspace/docs/decisions/ADR-007-peak-match-status-terminology.md`（影響確認）
- `/workspace/docs/decisions/ADR-008-*.md`（影響確認）
- `/workspace/docs/decisions/ADR-NNN-delete-zone-polygon.md`（新規）
- `/workspace/docs/00_GLOSSARY.md`

### C エンジン
- `/workspace/src/analyze.c`
- `/workspace/src/mesh_analyze.c`
- `/workspace/params/config.ini.example`

### Python スクリプト
- `/workspace/scripts/merge.py`
- `/workspace/scripts/output_geojson.py`

### 検証スクリプト（新規）
- `/workspace/analysis/sota_dem_elevation_diff.py`

---

## 再利用する既存資産

- `shapely`（既に Python 環境に存在、`merge.py` の `--regions-file` で利用例あり）
- merge.py 既存の point-in-polygon 経路（都道府県境界判定）
- C エンジン既存の Flood Fill 実装（FR-016 アクティベーションゾーン生成）
- merge.py 既存のログ出力（`merge_YYYYMMDD_HHMMSS.log` 形式は既に統一済み）

---

## 検証セクション

### 単体検証

1. **C エンジン**: 既知のテストメッシュで delete判定ゾーンポリゴンを生成 → 想定外形と一致するか目視確認
2. **merge.py**: 九州・四国の既存解析結果に新ロジック適用 → 削除 6 件すべて delete 判定されることを確認

### 統合検証

3. **北海道独立峰**: 利尻岳・羊蹄山・大雪山周辺で 3×3 解析実行 → delete判定ゾーンが想定範囲（250m キャップ）に収まることを確認
4. **全国解析**: 日本全土の解析実行 → unmatched サミット 0 件を確認（出れば 250m 値見直し）

### 失敗パターン検証

5. **強制 unmatched**: 250m を意図的に小さい値（例: 50m）に設定して解析 → unmatched サミット検出 → merge.py が non-zero exit → output_geojson.py が実行されないことを確認

---

## 別 ISSUE 切り出し候補（本 ISSUE 範囲外）

- area_complete=false ピークの隣接メッシュ合体問題（前セッションから繰り越し）
- area_complete スコープ明確化（AZ 用 / delete判定ゾーン用の分離）
- HTML ビューア（merged_viewer.html）の実装範囲（FR-013 既存仕様で要求済みだが現状未実装）

---

## ステータス

- 議論完了論点: 基本設計（delete判定ゾーン定義 / 判定ロジック / 不備処理方針）
- **未確定**: 250m パラメータ最終値（Phase B で実データ検証後に確定）
- **未着手**: SRS / ADR / 実装（Phase A 以降）
