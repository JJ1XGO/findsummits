# 計画: mesh_analyze.c 地理境界外ピークの除外（出力フィルタ）

## Context

検証の結果、combined 画像の端タイルが地理的メッシュ境界より若干（~17px ≒ 80m）
外側へはみ出しており、そのはみ出し部分で検出されたピークが CSV に混入している。

**設計上の制約**:
- フリンジデータ（端タイルの境界外ピクセル）は **Keyコル検出に必要** → 削除・マスク不可
- フリンジ内に存在するピーク → **出力対象外**

**修正方針**: Union-Find は full combined 画像で実行（変更なし）。
結果（AnalyzeResult）を CSV に書き出す C コードのループで、
mesh_set の地理的範囲外のピークをスキップする。
Python（merge.py）ではなく **C コードの出力段階**で完結させる。

merge.py の `analysis_count / expected_count / stability` ロジックは変更しない。

---

## 実行手順

### Step 0: 欠陥管理表への登録

```bash
python3 mgmt/tracker/track.py bug add \
  --title "combined 画像の地理境界外ピークが CSV に混入する" \
  --category "src/mesh_analyze.c" \
  --severity 中 \
  --resolution "CSV 出力ループで mesh_set 地理範囲外ピークをスキップ（C 側）"
```

### Step 1: `src/mesh_analyze.c` の修正

**変更箇所は2か所**:

#### 1-A. combined 計算ループ（行 261–275）に lat/lon コード追跡を追加

```c
MeshTileRange combined = center_range;
int lat_code_min = meshcode / 100;  /* 追加 */
int lat_code_max = meshcode / 100;  /* 追加 */
int lon_code_min = meshcode % 100;  /* 追加 */
int lon_code_max = meshcode % 100;  /* 追加 */

for (int dlat = -1; dlat <= 1; dlat++) {
    for (int dlon = -1; dlon <= 1; dlon++) {
        // ...（既存）
        /* lat/lon コードの追跡（追加） */
        int nb_lat = nb / 100, nb_lon = nb % 100;
        if (nb_lat < lat_code_min) lat_code_min = nb_lat;
        if (nb_lat > lat_code_max) lat_code_max = nb_lat;
        if (nb_lon < lon_code_min) lon_code_min = nb_lon;
        if (nb_lon > lon_code_max) lon_code_max = nb_lon;
    }
}
```

#### 1-B. CSV 出力ループ（行 367 付近）に地理範囲フィルタを追加

コメント更新:
```c
/* mesh_set の地理的範囲内のピークのみ CSV に保存 */
```

CSV ヘッダー出力の直前に地理的境界を計算:
```c
double geo_lat_south = lat_code_min       * 2.0 / 3.0;
double geo_lat_north = (lat_code_max + 1) * 2.0 / 3.0;
double geo_lon_west  =  lon_code_min + 100.0;
double geo_lon_east  =  lon_code_max + 101.0;
```

ループ内で `pixel_to_latlon` 直後にスキップ条件を追加:
```c
/* mesh_set 地理範囲外のピークはスキップ */
if (peak_lat <  geo_lat_south || peak_lat >= geo_lat_north ||
    peak_lon <  geo_lon_west  || peak_lon >= geo_lon_east)
    continue;
```

`out_cnt` はループ内でカウントする形に変更
（現在の `out_cnt = result->peak_cnt` から）。

---

### Step 2: 対応中に更新してから実装

```bash
python3 mgmt/tracker/track.py bug update <BUG-ID> --status 対応中 --actor "Claude"
```

その後 mesh_analyze.c を修正する。

### Step 3: `make` でビルド確認

```bash
cd /workspace && make
```

ビルドが通ったら対応完了にする（findsummits の再実行はユーザーの判断で別途実施）:

```bash
python3 mgmt/tracker/track.py bug close <BUG-ID> --actor "Claude" \
  --comment "CSV出力ループに mesh_set 地理範囲フィルタを追加。ビルド確認済み。findsummits 再実行による動作確認はユーザーが verify 時に実施"
```

---

## 修正対象ファイル

| ファイル | 変更内容 |
|---------|---------|
| `src/mesh_analyze.c` | combined ループに lat/lon コード追跡 + CSV 出力ループに地理範囲フィルタ |

**変更しないファイル**:
- `src/analyze.c` — Union-Find はフル combined 画像で変更なし
- `scripts/merge.py` — analysis_count/expected_count/stability はそのまま維持

---

## 期待される効果

| 項目 | 修正前 | 修正後 |
|------|--------|--------|
| 地理境界外ピーク | CSV に混入 | C 出力段階で除外 |
| Keyコル検出 | 正常 | 変更なし（フリンジデータ保持）|
| analysis_count 分布 | 4/6/9 + 少数の 2/3 | 4/6/9 のみ（見込み） |
| expected_count 一致率 | 428 件不一致 | 改善見込み |
