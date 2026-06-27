# FR-013 レビュー決着と SRS/ADR 反映計画

## Context

FR-013（HTML ビューア生成）のレビューを実施し、FR-009（生成元）・FR-019/020/021（消費側）との整合を突き合わせた結果、7件の指摘＋用語の違和感（「中心成果物」）が出て、1論点ずつユーザーと決着した。本計画はその決着を SRS/ADR/todo に反映するもの。

**仕様優先原則によりコードは一切触らない。docs/（SRS・ADR）と mgmt/todo.md のみ更新する。** 実装（C/Python）の追従は別途。

---

## 決着事項

### finding 1: metadata の責務分離（→ issue + ADR 改訂）
- `attribution`/`source_url`/`license_url` は ADR-URD-014/UR-011 由来の**固定定数**。FR-009 が中央データ `merged_summit.geojson` に刻む（ADR-URD-014 §2「中央データに出典埋め込み」維持＝案A）。
- metadata の**構造定義の正準を FR-009 に集約**。固定定数の**値の正準を ADR-URD-014／データ辞書**に置く。FR-013 は消費側として参照のみ。
- L1261・L1493 の「FR-013 メタデータ定義参照」→「FR-009 参照」に修正。
- キー名不一致是正: ADR-URD-014 §3 は `license`、SRS は `license_url`。**`license_url` に統一**（SRS 側が複数箇所で使用済みのため）。ADR-URD-014 を `license_url` に合わせる。

### finding 2: `gsi_tile_latest_date` を上流化（→ issue + ADR 新規）
- 集約スコープ = **案②（キャッシュ全体の mtime 最大、パイプライン末尾で1回スキャン）**。全国一括運用なら案①と実質同値。表示専用で判定に未使用のため費用対効果で案②。
- **FR-001 で取得時の HTTP `Last-Modified` をキャッシュファイルの mtime に焼き込む**（`os.utime`）。これがないと mtime が「DL日時」になり「提供元更新日」の意味にならない。304 でも値が保たれる。
- FR-009 が metadata に格納。FR-013/FR-019 は表示のみ。
- FR-013 の metadata 一覧に `gsi_tile_latest_date`（上流生成・表示用）を追記。
- 部分解析時にわずかに過大評価しうる点は「既知の制約」として注記。

### finding 3: `is_band_change_candidate` をピーク Point プロパティに追加（→ todo）
- FR-019 が「変更行出力」「rationale UI 表示」の出し分けに使う。rationale 非空判定は編集で壊れるため明示プロパティが必要。
- FR-009（L920）が算出済みの値を GeoJSON プロパティに載せるだけ。FR-013 のピーク Point プロパティ表に1行追加。

### finding 4: dominant の複数 delete サミット注記（→ todo）
- フィーチャ構成表 dominant 行に注記追加:「既存SOTAサミット Point とピーク→SOTAサミット LineString は、当該 delete判定ゾーンに含まれる削除候補サミットの数だけ生成される（複数可）」。

### finding 5: 不備の確認責務を geojson metadata → xlsx へ（→ issue + ADR-SRS-011/013 改訂）
- **top-level boolean 不備フラグを geojson metadata から削除**（消費者なし＝vestigial。FR-013 は exit code でスキップ判定、地図可視化は per-feature プロパティが担う）。per-feature の `key_col_resolved`・`area_complete` は残す。
- **不備確認を xlsx で完成**: `area_complete` 列を追加（is_area_incomplete 相当を per-row 化）、unmatched サミットを行として載せる（`match_status` 値域に delete/unmatched を含める＝「summit中心 xlsx に peak.match_status を載せている」既存不整合の是正も兼ねる）。
- **不備ゲート（データ品質による意図的な異常終了）発動時は xlsx + geojson をセット出力してから停止**するよう FR-009 L914 を書き換え。ハードクラッシュ（入力欠落・例外）は出力保証なしと区別して明記。

### finding 6: コルの `feature_type` を `key_col` に統一（→ todo）
- 中間 `merged_peak.geojson` は `"key_col"`、中心データ `merged_summit.geojson`（FR-013 L985）は `"col"` で不統一。**`"key_col"` に統一**（GLOSSARY「Keyコル」と整合）。
- 補足: 出力プロパティ名 `points`（FR-013/FR-012）は一貫済み。`peak_points`（FR-009 L918）は内部変数。FR-009 に「出力プロパティ名は `points`」と一言補足。

### finding 7: 旧名 `merged.geojson` 追従漏れ（→ todo）
- FR-009 概要 L831・FR-019 L1093 を `merged_summit.geojson` に修正。

### 用語変更: 「中心成果物」→「中心データ」（→ todo）
- 由来は ADR-SRS-013（全結果が集約する1つの中心データ、というユーザーイメージ）。種別は「内部データ」であり最終 deliverable ではないため「成果物」が誤読を生む。ユーザー選択により「中心データ」に統一。
- 対象: SRS 11箇所（L161/223/224/225/831/847/930/935/1178/1395/1399）+ ADR-SRS-013（3）+ ADR-SRS-030（5）。

---

## issue / todo 振り分け

**issue（仕様議論＋ADR を伴う。1論点1課題で個別登録）:**
- ISSUE-A: finding 1（metadata 責務分離・正準定義移動・license_url 統一）— ADR-URD-014 改訂
- ISSUE-B: finding 2（gsi_tile_latest_date 上流化・FR-001 mtime 焼き込み）— ADR 新規（SRS）
- ISSUE-C: finding 5（不備確認責務 geojson→xlsx・セット出力・xlsx 列追加）— ADR-SRS-011/013 改訂

**todo.md（仕様確定済み・残りは機械的反映）:**
- finding 3（is_band_change_candidate 追加）
- finding 4（dominant 複数注記）
- finding 6（feature_type key_col 統一 + points 補足）
- finding 7（旧名追従）
- 用語変更（中心成果物→中心データ）

※ issue 登録は CLAUDE.md ルールによりユーザー確認後に実施。

---

## 反映対象ファイル

- `docs/20_SRS.md`: FR-013(L927-1038), FR-009(L831/910-915), FR-001(L311-324), 6.5(L1382-1391), 6.6(L1395-1404 / L1402), FR-012(L1199-1228), FR-019(L1093/1108-1110/1116/1119), L1261, L1493、用語11箇所
- `docs/decisions/ADR-URD-014-...md`: 正準定義の受け皿化・`license_url` 統一
- `docs/decisions/ADR-SRS-011-...md`・`ADR-SRS-013-...md`: 不備の集中管理先を geojson metadata → xlsx へ変更
- `docs/decisions/ADR-SRS-0NN-...md`: gsi_tile_latest_date 上流化の新規 ADR
- `docs/decisions/ADR-SRS-013/030`: 「中心成果物」→「中心データ」
- `mgmt/todo.md`: finding 3/4/6/7・用語変更を列挙

---

## 段取り

1. ISSUE-A/B/C を tracker に登録（ユーザー確認後）
2. ADR 改訂/新規作成（ADR-URD-014・ADR-SRS-011/013・新規）
3. SRS へ反映（上記対象箇所）
4. todo.md に機械反映項目を列挙
5. docs 更新ごとに commit（ドキュメント更新ルール）

## 検証

- `grep -rn "中心成果物" docs/` が 0 件
- `grep -rn "merged\.geojson" docs/`（`merged_summit`/`merged_peak` を除く）が 0 件
- `grep -rn '"col"' docs/20_SRS.md` のコル feature_type 残存 0 件
- metadata の正準定義参照（L1261/L1493）が FR-009 を指す
- SRS 内アンカーリンク・FR 相互参照の整合（リンク切れなし）
