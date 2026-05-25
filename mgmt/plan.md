# FR-001 取得条件を FR-002 と一致させる + 404 時のキャッシュ削除対応

## Context

SRS FR-001（標高タイル事前取得）と FR-002（DEM 階層フォールバック）の整合性確認の議論で、以下 2 点の乖離が判明した。

### 乖離 1: 取得粒度

- **FR-002 の設計意図**: 「上位 DEM のピクセルが無効値（-9999m）または該当タイルが存在しない場合、同座標の次の DEM 種別のピクセル値を使用する」（`docs/02_SRS.md:193`）。ピクセル単位で `dem5a → dem5b → dem5c → DEM10b` をカスケードする。
- **現状 prefetch_tiles.py 実装**（`scripts/prefetch_tiles.py:185-199` `fetch_dem5_with_fallback()`）: 座標 (x,y) ごとに `dem5a` を試し、200/304 なら `dem5b/c` を取りに行かない「タイル単位フォールバック取得」になっている。
- **帰結**: 「dem5a タイルはあるが特定ピクセルが NODATA」の座標で、`elevation.c:113-148` のピクセル単位フォールバックが dem5b/c のキャッシュにアクセスできず（ファイル自体が無いため `load_png_rgb()` が NULL）、`elev_fill_nodata_dem10b()` の DEM10b 補完に直接カスケードする。dem5b/c に有効値が存在しても拾えない。

### 乖離 2: キャッシュ削除欠如

- `prefetch_tiles.py` には削除処理が一切ない（grep 結果 `delete|unlink|remove` 該当なし）。
- 地理院が古い dem5b/c を将来削除した場合、HTTP 404 → スキップで終わり、ローカルキャッシュの古いファイルは残り続ける。
- `elevation.c` は残った古いファイルを正常な dem5b/c として読み続け、地理院最新状態と乖離した解析になる。

### ユーザー方針（2026-05-25）

> 「FR-001 の取得条件を FR-002 と同じにして欲しい。でなければ意味がない。HTTP 404 が地理院タイルが存在しないという意味ならキャッシュから削除して地理院側と同じ状態にして欲しい。」

仕様優先原則に従い、SRS（FR-001）を正として改訂し、実装は SRS に追従する。

---

## 方針

### Phase 1: SRS FR-001 改訂（本セッションで対応・SRS ステージ）

`docs/02_SRS.md` の FR-001 を以下の方向で書き換える:

1. **取得粒度の明示**: 「DEM5a / DEM5b / DEM5c / DEM10b の各種別について、対象タイル範囲の全タイルを独立に取得する（取得時点で種別による条件分岐は行わず、すべて取得する）。FR-002 のピクセル単位フォールバックが機能するための前提として、全種別を並列取得する」
2. **404 時のキャッシュ削除**: 「HTTP 404 が返った場合、同パスのローカルキャッシュが存在すれば削除し、地理院側の状態と一致させる」を追記
3. **FR-002 との整合**: 取得方針が FR-002 のフォールバック動作と表裏一体であることを補足

### Phase 2: ADR-002 確認（本セッションで対応）

`docs/decisions/ADR-002-dem-hierarchy-fallback.md` を読み、取得段階の記述が新方針と矛盾していれば更新。Decision/Consequences に「prefetch は全種別取得・404 時はキャッシュ削除」を反映。

### Phase 3: ISSUE 登録（本セッションで対応）

`prefetch_tiles.py` の実装追従を新規 ISSUE として登録（対応予定ステージ = COD）。本セッションでは実装変更には踏み込まない（SRS ステージ）。

ISSUE 内容:
- `fetch_dem5_with_fallback()` の早期 break を撤廃し、dem5a/b/c を独立ジョブとして列挙
- `enumerate_jobs()` で dem5a 以外（dem5b/dem5c）も列挙対象に追加
- `fetch_one()` で HTTP 404 を受けたとき `path` 存在チェックの上で `os.unlink(path)` を実行
- サマリー出力の「フォールバック試行分」注記を「全種別取得」に修正

---

## 修正対象ファイル（本セッション）

| ファイル | 修正内容 |
|---|---|
| `docs/02_SRS.md` | FR-001（168-181 行付近）の文言修正：全種別並列取得・404 時キャッシュ削除 |
| `docs/decisions/ADR-002-dem-hierarchy-fallback.md` | 取得段階の記述を新方針と整合させる（必要に応じて） |
| `mgmt/tracker/data/issues.json` | prefetch_tiles.py 実装追従 ISSUE を新規登録 |
| `mgmt/tracker/reports/issues_export.xlsx` | 再生成 |

## 修正対象ファイル（後続 ISSUE で対応・本セッション対象外）

| ファイル | 修正内容 |
|---|---|
| `scripts/prefetch_tiles.py` | 全種別取得への変更・404 時キャッシュ削除 |
| `docs/03_HLD.md` / `docs/04_LLD.md` | 該当記述があれば整合 |

---

## 検証方法

### Phase 1-2（ドキュメント整合性）

- FR-001 と FR-002 を併読し、ピクセル単位フォールバックが機能するための取得前提が読み取れること
- ADR-002 の Decision / Consequences が新方針と整合していること

### Phase 3（実装フェーズ・別 ISSUE）

実装着手時に以下で動作確認:
1. テスト用メッシュで prefetch を実行し、dem5a 取得成功座標でも dem5b/c が独立に取得されることを確認（`$DATA_DIR/tiles/15/{x}/{y}_b.png` の存在）
2. ダミーキャッシュファイルを置き、HTTP 404 を返すモック取得で削除されることを単体テストで確認
3. dem5a に NODATA が含まれる実データで elevation.c のピクセル単位フォールバックが dem5b/c の有効値を拾えることを確認（解析結果の terrain PNG 比較）
