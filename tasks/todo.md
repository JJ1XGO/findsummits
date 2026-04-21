# TODO

## 実装前に確認が必要な事項

- [x] SOTA 申請用 Excel テンプレートの列定義・シート構成を確認する（`/mnt/findsummits/ref/` にテンプレートがあるか確認）
- [x] 解析対象の 1 次メッシュコードリストを確定する（tasks/mesh_list_japan.txt, 176コード）
- [x] 新規サミットの SummitCode 採番ルール → 申請前は `ZZ/ZZ-001` 形式の連番ダミーコードを使用（旧プロジェクト踏襲）

## 実装フェーズ

### Phase 1: C エンジン修正

- [x] **fetch.c**: ファイルサイズ判断削除 → HTTP200成功判定に変更。tile_dir をbase(zoom含まず)に変更。dem10b専用fetch追加。パス形式: `{base}/{z}/{x}/{y}_{dem}.png`
- [x] **elevation.c**: dem5 pixel-levelフォールバック(a→b→c)実装。NODATA保持→dem10b補完→SEA変換に変更。dem10bは自動fetch対応。
- [x] **mesh_analyze.c**: 3×3メッシュ方式（NW/SE隣接コードからcombined range計算、中心メッシュのみCSV出力）
- [x] **main.c**: メッシュリストファイル読み込み対応（数字ならmeshcode、それ以外はファイルパスと判断）
- [x] **test_mesh_analyze.c**: イメージ出力をTerrain-RGB形式に変更（Terrain-RGB=国土地理院標高タイル互換エンコード）

### Phase 2: 並列実行

- [x] **run_all.sh**: `xargs -P 8` で176メッシュを並列処理するシェルスクリプト（ログは `/mnt/findsummits/logs/`）

### Phase 3: Python後処理

- [x] summitslist.csv を `/mnt/findsummits/ref/` に再取得（2026-04-20付 181001行 JA:7211件）
- [ ] **scripts/merge.py**: 176CSV統合・is_tile_topフラグ処理・summitslist.csv突き合わせ・ZZ/ZZ-XXXダミーコード付与
- [ ] **scripts/output.py**: XLSX申請書・GeoJSON出力

## 保留・将来対応

- [ ] README.md の「使用する標高データについて」セクション（DEM1a 不採用理由）を、より適切な場所（設計判断を記録するドキュメント）が見つかり次第移動する
