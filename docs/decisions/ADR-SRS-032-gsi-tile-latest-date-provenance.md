# ADR-SRS-032: gsi_tile_latest_date の生成責務と集約スコープ

| 状態 | 採用・未実装 |
| 決定日 | 2026-06-19 |

## Context

SRS FR-013 の metadata 一覧（ISSUE-116 レビューで発覚）に `gsi_tile_latest_date`（地理院タイル更新日）が参照されているが、SRS L1110 で「生成実装は別 ISSUE 管理」として棚上げされており、どの FR が生成するか未定義だった（ISSUE-117 で解決）。

### 問題の核心

`gsi_tile_latest_date` は「**地理院側が当該タイルを最後に更新した日付**」を意味する。これを正確に知る手段は、タイル取得時の HTTP レスポンスヘッダー `Last-Modified` のみである。

現状の `prefetch_tiles.py`（FR-001）はタイルをローカルに PNG 保存するが、`Last-Modified` 値は記録しておらず、ファイルの mtime は「ダウンロードした日時」になってしまう。mtime をそのまま使うと `gsi_tile_latest_date` は「最終 prefetch 実行日時」になり、「提供元更新日」の意味にならない。

### 集約スコープの選択肢

| 案 | 方法 | 精度 | 実装コスト |
|---|---|---|---|
| 案① | 解析で**実際に使ったタイル**の mtime 最大 | 高（解析範囲と完全一致） | 高（C エンジン → per-mesh CSV 列追加 → FR-008 持ち回り → FR-009） |
| 案② | **キャッシュ全体**の mtime 最大（パイプライン末尾で1回スキャン） | 実用上十分 | 低（FR-009 末尾で `$DATA_DIR/tiles/` を1回スキャン） |

## Decision

### 1. FR-001 で HTTP `Last-Modified` をキャッシュファイルの mtime に焼き込む

タイル取得成功時（HTTP 200）に、レスポンスヘッダー `Last-Modified` の日時を `os.utime()` でキャッシュ PNG ファイルの mtime に設定する。

効果:

- ファイル mtime が「提供元更新日」を正確に表すようになる
- 304（Not Modified）レスポンス時はファイルをダウンロードしないため mtime は変わらず、前回取得時の `Last-Modified` 値が保持される（正しい動作）
- 404（地理院未整備）でキャッシュ削除する場合は mtime 操作不要

### 2. 集約スコープは案②（キャッシュ全体の mtime 最大）を採用

**採用理由**:

- 基本運用（日本全土一括 prefetch）では「使ったタイル」≒「キャッシュ全体」になり、案①と案②は実質同値
- `gsi_tile_latest_date` は表示専用（ビューアの情報欄）であり、ピーク検出・突合・採番などいかなる判定にも使用しない。精度の差が実害になる局面がない
- 案①は4コンポーネント（C エンジン・per-mesh CSV・FR-008・FR-009）を跨ぐ修正が必要で、表示専用フィールド1個のためのコストとして不釣り合い

**既知の制約**: 部分解析時（特定メッシュのみ prefetch）は、解析範囲外の古いタイルが混入してスコープより新しい日付が返る可能性がある。この制約は SRS に「既知の制約」として注記する。

### 3. FR-009 が metadata に格納

FR-009（SOTA リスト突合・match_status 判定）の処理末尾で `$DATA_DIR/tiles/` 配下の全 PNG ファイルの mtime 最大値を取得し、`gsi_tile_latest_date`（ISO 8601 UTC 形式）として `merged_summit.geojson` の top-level `metadata` に格納する。`Last-Modified` は HTTP の RFC 7231 準拠（UTC）であるため、タイムゾーン変換不要。

### 4. FR-013/FR-019 は表示のみ

`metadata.gsi_tile_latest_date` を読んでビューア上に表示する（表示時は `(UTC)` を付記）。生成には関与しない。

## Alternatives

### 案①：使用タイルのみ集計（不採用）

C エンジンが per-mesh 解析時に使用タイルの mtime を CSV に出力し、FR-008 が統合 CSV に最大値を持ち回り、FR-009 が参照する方式。精度は高いが、表示専用フィールド1個のために4コンポーネントを修正するコストが見合わない。全国一括運用で案②と結果が一致するため、精度向上の実益がない。→ **却下**

### `Last-Modified` を別ファイルに記録する方式（不採用）

タイル PNG とは別に mtime ファイル（JSON 等）を管理する方式。`os.utime()` による mtime 焼き込みと比べてファイル管理が複雑になる。mtime 焼き込みで同等の効果が得られるため不採用。→ **却下**

## Consequences

- `scripts/prefetch_tiles.py`: タイル保存処理に `os.utime()` による mtime 焼き込みを追加（HTTP 200 時のみ。404 でキャッシュ削除する処理は変更不要）
- `scripts/merge.py`（FR-009 実装）: 処理末尾に `$DATA_DIR/tiles/` 全 PNG の mtime 最大取得・`gsi_tile_latest_date` 格納を追加
- SRS FR-001 の説明に mtime 焼き込みを追記
- SRS FR-009 の説明に gsi_tile_latest_date 格納を追記
- SRS FR-013 の metadata 一覧に `gsi_tile_latest_date` を追記（既知の制約注記付き）
