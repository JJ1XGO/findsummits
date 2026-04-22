# TODO

承認済みアーキテクチャ刷新計画: `/home/tsu/.claude/plans/sequential-humming-hickey.md`

## 方針（2026-04-22 承認）

- `BORDER_TILES=8` 廃止 → 3×3 最小矩形方式に
- タイル取得は C から切り離し、`scripts/prefetch_tiles.py` の事前 prefetch に一本化
- `rank` int8 化 + `peak_id` ハッシュマップ化でメモリ削減
- `cmp_elev_desc` に 2 次キー (x→y) を追加して座標を決定論化
- 中心メッシュフィルタ廃止、combined 内の全ピークを CSV 出力
- CSV に `col_margin_px`, `center_mesh` を追加
- `prominence` は C 側 130m、Python 側で 150m 最終判定
- 既存 9 メッシュ CSV は削除し新方式で再解析

## 実装ステップ

- [ ] 既存 CSV 退避・削除（`/mnt/findsummits/results/csv/*.csv`）
- [ ] `src/mesh.c/h`: `MeshSet` と隣接計算ヘルパ追加
- [ ] `src/unionfind.c/h`: `rank` int8 化、`peak_id` ハッシュマップ化
- [ ] `src/analyze.c/h`: 2 次キー x→y、プロミネンス 130m、`col_margin_px` 計算、`PeakResult` 拡張
- [ ] `src/mesh_analyze.c/h`: 3×3 最小矩形、中心フィルタ撤廃、全ピーク出力、CSV 新列
- [ ] `src/fetch.c/h` 完全削除、`src/elevation.c` の fetch 呼出除去
- [ ] `Makefile`: `-lcurl` 除去、`test_fetch` ターゲット撤去
- [ ] `src/main.c`: MeshSet 読み込み、`min_prominence=130.0f`
- [ ] ビルド・単体テスト・小規模動作確認
- [ ] `params/fetch_config.ini.example` 作成、`.gitignore` に実設定を追加
- [ ] `scripts/prefetch_tiles.py` 新規作成（If-Modified-Since、並列4、backoff）
- [ ] `scripts/merge.py` 改修（期待解析回数、col_elev 最小、col_margin_px、150m 最終判定）
- [ ] 小規模 9 メッシュでフルパイプ検証
- [ ] コミット（論理単位で分割）

## 保留・将来対応

- タイル取得「最後の 1 枚」ハング問題は prefetch に移しての再現性確認
- Union-Find の CPU 並列化
- README.md「使用する標高データについて」セクションの移動
- `.claude/settings.json` / `.mcp.json` の扱い（空ファイル、放置中）
