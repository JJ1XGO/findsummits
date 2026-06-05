# ISSUE-078 北方領土タイル分離検証

## Context

ADR-URD-005「北方領土タイル単位除外」は以下を主張するが、
**「本土を誤除外しない」根拠**が実データで未検証である。

> ズームレベル15タイル（辺長 ~1.2km）の単位であれば、
> 島内タイルと本土タイルが地理的に重複することはない。

FR-017 レビュー（2026-06-05）でユーザーから「ADR-URD-005 の主張が疑わしい」との指摘あり、
ISSUE-078 として実データ検証タスクが切り出された。

**今回のスコープ（ユーザー指定）:**
> 北方領土と北海道側が同じタイルに入らない事が確認できればOK

これは ISSUE-078 原文の「プロミネンス 150m 以上ピーク有無確認」より範囲が狭く、
**Z15 タイルレベルで 北方領土6村 と 北海道本土 が同一タイルを共有しない**ことの実証に絞る。

## Approach

### 検証方式

**Z15 タイル交差ベースの集合演算**（中心点判定より保守的）:

1. N03-20260101.geojson から 2 集合のポリゴンを抽出:
   - **N（北方領土）**: `N03_007 ∈ {01696, 01697, 01698, 01699, 01700, 01701}`
   - **H（北海道本土）**: `N03_001 == "北海道"` かつ N に含まれない全フィーチャ
     （※ 歯舞群島は根室市 = H 側に含まれる）
2. それぞれ `unary_union` で dissolve
3. Z15 タイル列挙:
   - N の bbox 内の全 Z15 タイルについて、ポリゴン交差テスト → タイル集合 `T_N`
   - H の bbox を `T_N` のカバー範囲に絞り込み、同様に → タイル集合 `T_H`
4. `T_N ∩ T_H` を計算 → これが「同じタイルに両方が含まれる」候補
5. 結果出力:
   - 件数 0 → ADR-URD-005 の主張を実証
   - 件数 > 0 → 衝突タイルを GeoJSON 出力し対策検討

タイル座標変換は `src/mesh.c` の `latlon_to_tile` を Python 移植
（Web Mercator 標準式、shapely の `STRtree` で交差を高速化）。

### 出力

| パス | 内容 |
|---|---|
| `analysis/verify_northern_territories_tile_isolation.py` | 検証スクリプト |
| `analysis/northern_territories_tile_isolation.txt` | サマリー（件数・最近接距離） |
| `analysis/northern_territories_overlapping_tiles.geojson` | 衝突タイル（あれば。地理院地図で目視確認用） |
| `analysis/northern_territories_excluded_tiles.geojson` | 除外候補タイル全体（参考） |

### 検証後の作業（本セッション範囲）

- **本セッションは検証実行 + 結果報告までで停止**
- ADR-URD-005 への追記・ISSUE-078 クローズはユーザー確認後に別途指示を受けて実施
- 検証結果（衝突件数・衝突タイルがあれば GeoJSON 添付）を報告する
- 副次的に判明した事実（例: ADR の N03_007 対応表のズレ）も結果報告に併記

## Critical Files

- `/data/ref/N03-20260101.geojson` — 入力 N03 行政区域 GeoJSON（765MB）
- `src/mesh.c` (L22-34) — `latlon_to_tile` 参考実装
- `scripts/preprocess_pref_boundaries.py` — N03 読み込みパターン参考
- 新規: `analysis/verify_northern_territories_tile_isolation.py`
- 更新候補: `docs/decisions/ADR-URD-005-northern-territories-exclusion.md`
- 更新: `mgmt/tracker/data/issues.json`（ISSUE-078 のステータス）

## Verification

```bash
# 検証実行
venv/bin/python3 analysis/verify_northern_territories_tile_isolation.py

# 期待出力（成功例）:
#   北方領土タイル数: NNNN
#   北海道本土タイル数: MMMM
#   衝突タイル数: 0
#   最近接距離: X.XX km
#   ✅ ADR-URD-005 の前提を実データで確認
```

- 衝突 0 件: ADR-URD-005 への追記 → コミット → ISSUE-078 close
- 衝突 > 0 件: 衝突タイル GeoJSON を地理院地図で目視確認 →
  ユーザーと対策（buffer 導入 or タイルベース除外方式の見直し）を協議
