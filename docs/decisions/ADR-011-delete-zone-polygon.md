# ADR-011: delete判定ゾーンポリゴンの導入

| 状態 | 採用・未実装 |
| 決定日 | 2026-05-21 |

## Context

### 削除判定方式の課題（旧方針: コル等高線ポリゴン）

ピーク域ポリゴン生成（FR-016）で生成していた「コル等高線ポリゴン」（Flood Fill 閾値 = `col_elev` 以上）を削除判定に用いる旧方針には、以下の課題があった。

1. **独立峰問題**: プロミネンス 500m 超の独立峰（富士山・利尻岳・大雪山等）では、コル等高線が遠方の山域まで連続し、ポリゴンが日本全土規模に膨張する
2. **ガード条件の恣意性**: 上記回避のため「既存 SOTA プロミネンス > 500m なら削除除外」のガード条件が必要となるが、500m 値に技術的根拠がない
3. **コル未確定ピークの判定欠落**: `key_col_resolved=false`（広域再解析でも解消しない陸地最高峰級）のピークではコル等高線ポリゴン自体が生成されず、削除判定から漏れる
4. **判定方向の脆弱性**: 「自分より高い周辺ピーク」を基準とするとき、SOTA リスト登録標高と DEM 標高の前後関係が逆転すると判定が不安定になる

### 実データ検証（九州・四国前プロジェクト）

前プロジェクト `findsummits4sotaja` の削除 6 件で、削除側サミットと統合先（高い側）ピークの間の minimax 鞍部標高差（`main_Δ` = 統合先ピーク標高 − minimax 鞍部標高）を実測した結果:

- 6 件全件が `main_Δ` 100〜178m（最小 100.3m、最大 178.0m、平均 125.4m）に分布
- 「AZ 外 50m ゾーン」案では 1 件も拾えない
- 削除判定の本質は **新規ピーク発見によりプロミネンスが SOTA 閾値 150m 未満に再計算される現象** であり、`main_Δ` ≈ 150m に収束する構造的必然がある

詳細は [research/issue-020-keycol-threshold-analysis.md](research/issue-020-keycol-threshold-analysis.md) を参照。

## Decision

### delete判定ゾーンポリゴン の定義

各ピークについて、削除判定専用のポリゴンを生成する:

- **feature_type**: `delete_zone`
- **Flood Fill 閾値**: `max(col_elev, peak_elev - delete_zone_max_drop)` 以上
- **等価式**: ピーク標高から `min(prominence, delete_zone_max_drop)` 以内の連続エリア
- **パラメータ**: `delete_zone_max_drop`（`params/config.ini`）

`delete_zone_max_drop` の初期値は **250m** とする。最終値は SOTA 日本支部全サミット（約 1,500 件）の登録標高と DEM 標高の差分分布を調査し、以下の式で確定する:

```
delete_zone_max_drop = 150 + abs(SOTA標高 - DEM標高).max() + 余裕、切り良い整数
```

- 150m: SOTA プロミネンス閾値
- abs(SOTA-DEM): SOTA リスト登録標高と DEM 標高の差分（タイル更新により乖離する可能性）
- 余裕: 切り良い数値への丸め

### 既存サミット判定（FR-009）

ピークから生成された AZ ポリゴンおよび delete判定ゾーンポリゴンに対し、SOTA リストの各サミット座標で point-in-polygon 判定を行う。判定は**座標のみ**で行い、SOTA リスト登録標高と DEM 標高の前後関係には依存しない（標高無関係）。

| `summit.match_status` | 条件 |
|---|---|
| `matched` | いずれかのピークの AZ 内に存在 |
| `delete` | いずれかのピークの delete判定ゾーン内かつ AZ 外に存在 |
| `unmatched` | いずれにも該当しない（エラー、処理中止） |

`unmatched` は本来発生しないべき状態で、発生した場合は `delete_zone_max_drop` の値が小さすぎる等の不備を示す。merge.py は non-zero exit で停止し、後続の FR-013（GeoJSON/HTML 生成）はスキップする。

### コル等高線ポリゴンの廃止

旧方針の「コル等高線ポリゴン」（`feature_type="key_col_boundary"`、Flood Fill 閾値 = `col_elev`）は廃止する。判定構造（peak.match_status の matched / dominant / new、summit.match_status の matched / delete）と主ピーク特定アルゴリズム（最小プロミネンス）は維持する。

### 副次効果

- **独立峰問題の自然解消**: 250m キャップによりポリゴンが日本全土規模に膨張しない
- **ガード条件不要**: 500m のような恣意的な閾値が不要に
- **広域再解析の役割縮小**: delete判定ゾーンは常に 3×3 メッシュ範囲内で完結するため、FR-014（広域再解析）のトリガーは `key_col_resolved=false` および AZ の `area_complete=false` のみに限定できる
- **削除判定のシンプル化**: 座標のみで判定するため、SOTA 登録標高と DEM 標高の前後関係に依存しない

## Alternatives

### 案 A: AZ + AZ 外 50m ゾーン（不採用）

ピーク標高 −25m（AZ）/−50m（追加ゾーン）の固定閾値で 2 段階判定する案。九州・四国実測で 6 件全件が `main_Δ` ≥ 100m なため、50m ゾーンでは 1 件も拾えない。固定閾値ではプロミネンスの大小に追従できず、判定が機能しない。

### 案 B: コル等高線ポリゴン継続 + ガード条件（不採用）

旧方針を維持し「既存 SOTA プロミネンス > 500m なら削除除外」をガード条件として追加する案。500m 値が恣意的で、独立峰問題への根本対処にならない。delete判定ゾーン方式では 250m キャップにより独立峰問題が自然解消するため、ガード条件自体が不要になる。

### 案 C: コル等高線ポリゴン継続 + 暫定ポリゴン union（不採用）

Union-Find を拡張し、暫定 `col_elev` で暫定ポリゴンを生成する案。コル等高線方針自体が独立峰問題を抱えているため、複雑化するだけで根本解決にならない。

### 案 D: 固定閾値 300m ポリゴン（不採用）

`peak_elev - 300m` 以上の固定閾値で削除判定する案。プロミネンスに連動しないため、プロミネンス 150〜300m のサミットでポリゴンが実プロミネンス（コル）を超えて拡張し、判定が過剰になる。delete判定ゾーン方式（`max(col_elev, peak_elev - 250m)`）はプロミネンス連動でこの問題を回避する。

## Consequences

### 影響を受ける文書・実装

- **ADR-007** (peak-match-status-terminology): 「コル等高線内」表記を「delete判定ゾーン内」に更新
- **ADR-008** (dominant-peak-identification): 「コル等高線ポリゴン」「feature_type=key_col_boundary」表記を delete判定ゾーン関連に更新（アルゴリズム本体は維持）
- **FR-016**: コル等高線ポリゴン仕様を削除し、delete判定ゾーンポリゴン仕様を追加
- **FR-009**: `summit.match_status` に `unmatched` 追加、エラー停止仕様追加
- **FR-013**: dominant/new フィーチャ構成のポリゴン種別を変更
- **FR-014**: `area_complete=false` トリガーを AZ のみに限定、広域モードで delete判定ゾーンは生成しない
- **GLOSSARY**: 「delete判定ゾーン」用語追加、「コル等高線ポリゴン」用語削除
- **C エンジン** (`src/analyze.c`, `src/mesh_analyze.c`): Flood Fill 閾値とポリゴン種別の変更
- **merge.py**: AZ / delete判定ゾーンの point-in-polygon 実装、不備フラグ列追加、exit code 制御
- **output_geojson.py**: dominant フィーチャ構成変更
- **params/config.ini.example**: `delete_zone_max_drop` パラメータ追加

### 未確定事項

- `delete_zone_max_drop` の最終値（SOTA 全サミットの abs(SOTA-DEM) 分布調査後に確定）
- 全国解析実行時に `summit.match_status="unmatched"` が発生しないことの実証
