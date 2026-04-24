# findsummits アーキテクチャ刷新（3×3最小矩形 + 事前prefetch）

## Context

2026-04-21 のセッションで、以下のアーキテクチャ刷新がユーザーと合意済み（`.claude/handovers/2026-04-22_0015.md`）。

- 現行の `BORDER_TILES=8` 方式は過去の合意（3×3 フルメッシュ）違反として正式撤回
- Key Col 距離の実データ検証（`.claude/manage/keycol_analysis.md`）で、18km バッファでは 150 件（3.3%）取りこぼし・最遠 253km の独立峰あり → 3×3 必須
- 解析時の libcurl によるタイル取得は廃止し、事前 prefetch (Python) に一本化
- メモリ削減（`rank` int8・`peak_id` ハッシュマップ）で 3×3 を 62GB RAM 内で並列 2 まで実行可
- 座標決定論化（`cmp_elev_desc` の 2 次キー x→y）で「同一峰は異なる解析でも同一 global pixel に収束」させ、merge での重複判定を exact match で実施できるようにする
- 既存 9 メッシュ CSV (`/mnt/findsummits/results/csv/{5238..5440}.csv`) は削除し、新方式で再解析（ユーザー確認済）

目的: 全 176 メッシュを正確かつ効率的に解析し、SOTA 日本支部へ申請できる品質の「新規/変更/削除」候補 CSV を生成できるパイプラインに刷新する。

---

## 変更対象ファイル

### C エンジン

| ファイル | 役割 |
|---|---|
| `src/unionfind.h/.c` | `rank` int8 化、`peak_id` をハッシュマップ化 |
| `src/analyze.h/.c` | `cmp_elev_desc` 2 次キー（x→y）、プロミネンス閾値 130m、`col_margin_px` 計算、境界近接判定 |
| `src/mesh_analyze.h/.c` | 3×3 最小矩形、中心フィルタ撤廃、全ピーク出力、CSV 新列 |
| `src/mesh.h/.c` | 隣接メッシュコード計算・メッシュリスト Set 判定 |
| `src/fetch.h/.c` | `fetch_mesh`/`fetch_tile`/`fetch_dem10b_tile` 廃止、キャッシュ確認のみ残す |
| `src/elevation.c` | `elev_fill_nodata_dem10b` と `elev_load_with_overlap_8dir_with_dem10` から fetch 呼出を除去、未キャッシュ時は警告/エラー |
| `src/main.c` | メッシュリストを `MeshSet` に読み込み、各メッシュの隣接情報を `mesh_analyze` に渡す |

### Python スクリプト

| ファイル | 役割 |
|---|---|
| `scripts/prefetch_tiles.py` (新規) | If-Modified-Since 条件付き取得・並列4・100ms 間隔・UA 明示・429/503 backoff・Last-Modified→mtime 反映 |
| `scripts/merge.py` (改修) | 期待解析回数判定、`col_elev` 最小・`col_margin_px` 最大採用、`prominence≥150m` 最終判定 |

### 設定・データ

| ファイル | 役割 |
|---|---|
| `params/fetch_config.ini.example` (新規、commit) | UA テンプレート |
| `params/fetch_config.ini` (新規、gitignore) | 実 UA（メールアドレス含む） |
| `.gitignore` | `params/fetch_config.ini` 追記 |
| `/mnt/findsummits/results/csv/*.csv` | 削除（BORDER=8 方式の残骸） |

---

## 実装詳細

### 1. `src/unionfind.h/.c` — メモリ削減

**変更**:
- `int32_t *rank` → `int8_t *rank`（-8GB / 3×3 最大時）
  - 経路圧縮 union-by-rank で rank が int8 上限（127）に達するケースは実質ない。念のため `rank < 127` の増分ガードを入れる
- `int32_t *peak_id[N]` → 開放オープンアドレス法ハッシュマップ `PeakMap`
  - キー: root 画素インデックス（int32_t）、値: peak_id（int32_t）
  - 実装は `src/unionfind.c` にインラインで持つ（別ファイル化は不要）
  - 初期容量は全ピーク見込み 200K に対し load factor 0.5 を切るサイズ（262144）から動的成長
  - `uf_find` 後に root 値を参照する箇所をすべて `peakmap_get(pm, root)` に置換
  - `uf_union` で loser 側はエントリ削除（任意、放置でも正しさは壊れない）
  - 非 root への `peak_id` 代入（`analyze.c` 内の 2 箇所）は廃止（もともと未使用）

**公開 API の変更**:
- 内部の `peak_id` 配列を使う呼出が無いか要確認（grep 済、`analyze.c` のみ）
- `uf_find` は変更なし
- `uf_new_peak` は内部で `peakmap_put(root=i, pid)` を呼ぶ

### 2. `src/analyze.h/.c`

**`cmp_elev_desc` の 2 次キー追加**（決定論化）:
```c
static int cmp_elev_desc(const void *a, const void *b) {
    const Pixel *pa = a, *pb = b;
    if (pa->elev > pb->elev) return -1;
    if (pa->elev < pb->elev) return  1;
    if (pa->x   < pb->x   ) return -1;  /* x 小=西 優先 */
    if (pa->x   > pb->x   ) return  1;
    if (pa->y   < pb->y   ) return -1;  /* y 小=北 優先 */
    if (pa->y   > pb->y   ) return  1;
    return 0;
}
```
並行解析間の変換は不要（translation は ordering を保存する）。

**プロミネンス閾値を 130m に**:
- 現在は呼出側 (`MeshAnalyzeConfig.min_prominence = 150.0f`) から来る
- merge.py 側で 150m 最終判定するため、C 側は 130m で出力（境界近接で過小評価された候補を取りこぼさないマージン）
- `main.c` の `DEFAULT_CFG.min_prominence = 130.0f` に変更

**`col_margin_px` 計算**:
- `analyze_tile_data` に解析領域の境界情報を引数追加、または「エッジまでの最短距離」を各ピーク結果に含める
- 実装: PeakResult に `int32_t col_margin_px` を追加。col 座標 (col_x, col_y) から上下左右の画像外周までの 4 方向の最小距離（ピクセル）
- is_tile_top=1 の場合は `col_margin_px = -1`（無意味）

**`PeakResult` 新メンバ**:
```c
typedef struct {
    int32_t peak_x, peak_y;
    float   peak_elev;
    int32_t col_x, col_y;
    float   col_elev;
    float   prominence;
    int     is_tile_top;
    int32_t col_margin_px;   /* 追加: col から combined 境界までの最短距離(px)、is_tile_top=1なら-1 */
} PeakResult;
```

### 3. `src/mesh.h/.c` — 隣接判定ヘルパ

```c
/* メッシュリストを保持する軽量 Set（整数配列 + 線形/二分探索） */
typedef struct {
    int *codes;   /* ソート済み */
    int  count;
} MeshSet;

int  mesh_set_load(MeshSet *set, const char *list_path);
int  mesh_set_contains(const MeshSet *set, int code);
void mesh_set_destroy(MeshSet *set);

/* 隣接メッシュコードを計算（存在チェックなし） */
int  mesh_neighbor(int code, int dlat, int dlon);  /* dlat, dlon ∈ {-1, 0, 1} */
```

1次メッシュコード形式（例: 4929 = lat 32.67-33.33°、lon 129-130°）:
- `lat_code = code / 100`、`lon_code = code % 100`
- N 隣接: `lat_code+1`、S 隣接: `lat_code-1`、E: `lon_code+1`、W: `lon_code-1`
- 日本の対象範囲内では桁あふれなし

### 4. `src/mesh_analyze.h/.c` — 3×3 最小矩形 + 全ピーク出力

**`MeshAnalyzeConfig` 拡張**:
```c
typedef struct {
    const char    *tile_dir;
    const char    *result_dir;
    const char    *img_dir;            /* 追加: Terrain-RGB PNG 出力先（NULL=出力しない） */
    float          min_prominence;     /* 130.0f */
    const MeshSet *mesh_set;           /* 追加: 隣接判定用 */
} MeshAnalyzeConfig;
```

**3×3 最小矩形の計算**:
```c
/* 中心メッシュ + 隣接メッシュが存在する方向だけ拡張 */
MeshTileRange combined = center_range;
for (int dlat = -1; dlat <= 1; dlat++) {
    for (int dlon = -1; dlon <= 1; dlon++) {
        if (dlat == 0 && dlon == 0) continue;
        int nb = mesh_neighbor(meshcode, dlat, dlon);
        if (!mesh_set_contains(cfg->mesh_set, nb)) continue;
        MeshTileRange nb_range;
        if (mesh_to_tile_range(nb, 15, &nb_range) != 0) continue;
        if (nb_range.x_min < combined.x_min) combined.x_min = nb_range.x_min;
        if (nb_range.x_max > combined.x_max) combined.x_max = nb_range.x_max;
        if (nb_range.y_min < combined.y_min) combined.y_min = nb_range.y_min;
        if (nb_range.y_max > combined.y_max) combined.y_max = nb_range.y_max;
    }
}
combined.tile_w = combined.x_max - combined.x_min + 1;
combined.tile_h = combined.y_max - combined.y_min + 1;
```

**fetch 廃止**:
- `fetch_mesh(&fetch_cfg, &combined);` を削除
- 代わりに「prefetch_tiles.py で事前取得済みのはず」というコメント
- `load_mesh_tile` 内で未キャッシュタイル検出時は明確にエラー終了

**中心メッシュフィルタ廃止・全ピーク出力**:
- 現行の `if (p->peak_x < cx_min || p->peak_x > cx_max || ...)` チェックを削除
- combined 内全ピークをそのまま CSV 出力
- マージは Python 側で行う

**CSV 出力フォーマット変更**（新列追加）:
```
peak_lat,peak_lon,peak_elev,col_lat,col_lon,col_elev,prominence,is_tile_top,col_margin_px,center_mesh
```
- `center_mesh`: この解析の中心メッシュコード（混在 CSV でも由来がわかる）

### 5. `src/fetch.h/.c`・`src/elevation.c` — キャッシュ専用化

**`fetch.h`**:
- `fetch_tile`、`fetch_dem10b_tile`、`fetch_mesh` を削除
- `tile_is_cached`、`tile_dem10b_is_cached`、`make_tile_cache_path` は残す（elevation.c が使う）
- `FetchConfig` 型は削除

**`fetch.c`**:
- 削除した関数定義を削除
- libcurl 依存が残る場合は elevation.c も見直す（fetch 呼出除去）

**`elevation.c`**:
- `elev_load_with_overlap_8dir_with_dem10` 内の `fetch_dem10b_tile(&fc, z14_x, z14_y);` を削除（テスト用コードなので警告ログに置き換え可）
- `elev_fill_nodata_dem10b` 内の同 fetch 呼出を削除
- 未キャッシュのままロード試行し、失敗時は `stderr` に 1 行ログを残して NODATA 継続（解析は続行）

**Makefile**:
- 現状 `LIBS = -lcurl -lpng -lm`。fetch.c を残したまま libcurl 未使用にするか、fetch.c 完全削除して libcurl を外すか
- **選択**: fetch.c 完全削除し、`LIBS = -lpng -lm` に変更。キャッシュチェックは elevation.c 内の `stat()` で十分

### 6. `src/main.c`

- 引数がファイルパスなら `MeshSet` を読み込み
- 引数が単一メッシュコードなら `MeshSet` に 1 件だけ入れて `mesh_analyze` に渡す（隣接は存在しないので center のみ解析）
- `min_prominence = 130.0f` に変更

### 7. `scripts/prefetch_tiles.py` (新規)

**仕様**:
- `params/mesh_list_japan.txt` を読み、全メッシュ（＋ 3×3 分の外周タイル）の z15 dem5 と z14 dem10b を事前取得
- 初回: 各 URL を GET、Last-Modified を保存（ファイル mtime に反映）
- 2 回目以降: `If-Modified-Since: <mtime>` で条件付き GET
  - 304 → スキップ（ログ出力のみ）
  - 200 → 保存・mtime 更新
  - 404 → 次の DEM 候補（dem5 は a→b→c、それ以外は SEA 扱いなのでスキップ可）
  - 429/503 → `Retry-After` or 指数バックオフ（初期 60s）
- 並列: 4（`params/fetch_config.ini` で変更可）
- 間隔: 各リクエスト間 100ms sleep
- User-Agent: `findsummits/1.0 (mailto:<config email>)`
- ログ: `取得 / 304 / 404 / 既キャッシュ` の内訳を集計表示

**CLI**:
```
python scripts/prefetch_tiles.py \
  --mesh-list params/mesh_list_japan.txt \
  --config params/fetch_config.ini \
  --tile-dir /mnt/findsummits/tiles
```

**依存**: `urllib.request` のみで書く（requests/pandas は入れない。前回 pandas 未インストールで詰まった前例あり）

### 8. `params/fetch_config.ini.example` (新規、commit する)

```ini
[fetch]
# If-Modified-Since 方式で地理院サーバへ問い合わせる際の連絡先
# 実運用では params/fetch_config.ini にコピーしてメールを記入
user_agent_email = your.email@example.com

# 並列数（地理院サーバ負荷軽減のため 4 推奨）
max_parallel = 4

# リクエスト間インターバル（ミリ秒）
interval_ms = 100

# 429/503 時の初期バックオフ秒数
backoff_initial_sec = 60
```

### 9. `.gitignore` 追記

```
params/fetch_config.ini
```

### 10. `scripts/merge.py` 改修

**期待解析回数の計算**:
- 各メッシュ M について、`expected_count(M) = 1 + 隣接メッシュのうち mesh_list に含まれる数`
- 解析結果 CSV 群を読み込み、peak を (px, py) = `latlon_to_pixel(peak_lat, peak_lon)` で group
- group 内のレコード数 == expected_count(peak 所属メッシュ) なら「確定」

**確定判定**:
- 「全件 `is_tile_top=0`」かつ「レコード数 == expected_count」→ 確定
- 不足・過剰は `status = "unstable"` で別途出力（調査用）

**重複採用規則**:
- 重複ピーク群から 1 件を選ぶ:
  - `col_elev` が最小のレコードを採用（保守的評価）
  - `col_margin_px` が最大のレコードを優先キーにしても良いが、handover 決定は「col_elev 最小」

**CLI 拡張**:
- 既存 `--tolerance` は「SOTA リスト突合」用のまま残す（解析結果間は exact match、0 固定）
- 新規 `--csv-dir` はそのまま

**出力 CSV 列追加**:
- `col_margin_px`（マージ後の採用値）
- `analysis_count` / `expected_count`（デバッグ用）

### 11. 既存 CSV の削除

```bash
rm -rf /mnt/findsummits/results/csv/*.csv
```

---

## 影響範囲の注意

- **テスト**: `tests/test_mesh_analyze.c` / `tests/test_fetch.c` は fetch API 変更の影響を受ける。fetch.c を完全削除する場合、test_fetch.c も削除する。
- **Makefile**: `CORE_SRCS` は `src/*.c` をワイルドカード取得しているので fetch.c 削除で自動反映。`test_fetch` ターゲットのみ手で外す。
- **ビルド依存**: `-lcurl` を外すなら、後述の prefetch 実装を先に終え、C エンジンだけでフル解析できないことを受け入れる。

---

## 実装順序（1 PR 分、C → Python の順）

1. **準備**: 既存 CSV を退避 `mv /mnt/findsummits/results/csv/{5238..5440}.csv /tmp/` → 動作確認後削除
2. **C エンジン**: `mesh.c` 隣接ヘルパ → `unionfind.c` メモリ最適化 → `analyze.c` cmp+col_margin → `mesh_analyze.c` 3×3+CSV → `elevation.c`/`fetch.c` 削除 → `main.c` MeshSet → ビルド・`test_mesh_analyze` 単体確認
3. **Config**: `params/fetch_config.ini.example` + `.gitignore`
4. **Prefetch**: `scripts/prefetch_tiles.py` 書いて小規模（1〜3 メッシュ分）で動作確認
5. **本番 prefetch**: 全 176 メッシュを事前取得（長時間タスク、別セッションでも可）
6. **再解析**: 1 メッシュでフル回す → 9 メッシュ分再解析して結果検証 → 全 176
7. **Merge**: `scripts/merge.py` 改修・確定判定テスト
8. **コミット**: 論理単位で分割コミット（PR は 1 本）

---

## 検証手順

### ビルド・単体
```bash
cd /home/tsu/sota/findsummits
make clean && make
./build/findsummits 4929   # 単一メッシュ（隣接は含まれない）で動作確認
```

### 3×3 動作確認（隣接込み）
```bash
# 関東近辺の 9 メッシュを一時リスト化（実地理では周囲全てにメッシュが存在するが、
# 本テストでは「mesh_list_japan に相当するセット＝この 9 件だけ」と仮定して挙動確認する）
cat > /tmp/test9.txt <<EOF
5338
5339
5340
5438
5439
5440
5538
5539
5540
EOF
./build/findsummits /tmp/test9.txt
# 中心位置 (5439) の combined 範囲が N/S/E/W/NE/NW/SE/SW 全て拡張されることをログで確認
# 角位置 (5538) は、このテスト用 MeshSet の中では隣接が NE/E/N の 3 方向だけ存在する
# →拡張方向が 3 つだけになることを確認（角メッシュ＝最小矩形が縮退するケースの検証）
```

### Prefetch 動作確認
```bash
cp params/fetch_config.ini.example params/fetch_config.ini
# user_agent_email を自分のアドレスに書き換える
python scripts/prefetch_tiles.py --mesh-list /tmp/test9.txt --config params/fetch_config.ini \
    --tile-dir /mnt/findsummits/tiles
# 2 回目実行で全て 304 になることを確認
python scripts/prefetch_tiles.py --mesh-list /tmp/test9.txt --config params/fetch_config.ini \
    --tile-dir /mnt/findsummits/tiles
```

### 再現性・決定論の確認
```bash
# 同一メッシュを 2 回解析して diff が空になることを確認
./build/findsummits 5339
cp /mnt/findsummits/results/csv/5339.csv /tmp/5339_a.csv
./build/findsummits 5339
diff /tmp/5339_a.csv /mnt/findsummits/results/csv/5339.csv   # 空差分
```

### Merge 検証
```bash
./build/findsummits /tmp/test9.txt
python scripts/merge.py --csv-dir /mnt/findsummits/results/csv \
    --summitslist /mnt/findsummits/ref/summitslist.csv \
    --output /mnt/findsummits/results/merged_test9.csv
# 中心メッシュ（5439 等）のピークが expected_count = 9 で確定しているか目視
# 角メッシュ（5538 等）のピークが expected_count = 4 になっているか目視
```

### 既存 CSV の最終削除（検証 OK 後）
```bash
rm -rf /tmp/5238*.csv /tmp/52*.csv  # 退避分
```

---

## リスク・残課題

- **タイル取得「最後の 1 枚」ハング問題**（lessons.md 既記）は prefetch に移して発生するか別途確認
- **peak_id ハッシュマップ**の実装バグが出やすい（int32_t キーで -1 を空セルマーカーに使わない・ロードファクタ・リサイズ）。ユニットテスト `test_unionfind.c` を拡張して回す
- **Union-Find の CPU 並列化**は本刷新では扱わない（未解決）
- **prefetch 全 176 メッシュ**は数時間〜半日かかる可能性あり。別セッション・バックグラウンド推奨
