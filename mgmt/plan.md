# 中心データ＝merged.geojson モデルへの SRS 改訂（rationale プロパティ追加を含む）

## Context

findsummits4sotaja プロジェクトの SRS（`docs/02_SRS.md`）は現在、フェーズ3 末尾の中心データを以下の 2 ファイルに分割している：

- `merged.csv`（FR-012 規定）: ピーク・サミットの表データ
- `merged_activation.geojson`（6.11 規定）: ポリゴン（活性化ゾーン・delete判定ゾーン）のみ・Point フィーチャなし

最終 `merged.geojson` はフェーズ4 の FR-013 で上記 2 つを統合して組み立てる構造。

一方、ユーザーの本来のイメージは「**フェーズ3 末尾で `merged.geojson` 1 つに全てが集約**され、HTML ビューアはそれを表示するだけ（rationale を編集する以外は読み取り専用）」というもの。

申請書 XLSX に出力する「I: 根拠」テキスト（FR-011 の ※2/※4/※5 テンプレート）をピーク・サミットフィーチャのプロパティとして持たせる議論を契機に、中心データの位置付けを根本から見直し、ユーザー本来のイメージに合わせる SRS 改訂を行う。

また、ISSUE-055（GeoJSON ビューア HTML UI 要件追加）を本改訂に統合する（rationale 編集 UI・初期値テンプレート・XLSX 反映の要件が重複するため）。

## 設計判断

### 中心データの位置付け

| 項目 | 新しい位置付け |
|---|---|
| `merged.geojson` | **フェーズ3 末尾の中心成果物**（全 Point + 全 Polygon + rationale + 不備フラグを含む） |
| `merged.csv` | merged.geojson から派生する**エビデンス CSV**（UR-005）。**`rationale` 列は含めない** |
| `merged_activation.geojson` | per-mesh activation 統合の**内部中間ファイル**。**物理出力は残す**（デバッグ・差分検査用） |
| `merged_viewer.html` | フェーズ4 で `merged.geojson` のみを入力に生成（責務縮小） |

### rationale プロパティの配置

- **ピークフィーチャ**（new / dominant / matched_band_change）の `rationale` プロパティ ← FR-009 で生成（※2 追加根拠 or ※5 変更根拠）
- **削除サミットフィーチャ**（dominant ケースの既存 SOTA サミット側）の `rationale` プロパティ ← FR-009 で生成（※4 削除根拠）

dominant 行は申請書 XLSX で 2 行（追加 + 削除）に展開されるため、ピーク Point と削除サミット Point の 2 つに独立した `rationale` を持たせる必要がある。Point フィーチャが独立しているため自然に両立する。

### テンプレート定義の置き場

※2/※4/※5 のフォーマット定義を **FR-009 に集約**し、FR-011 からは「FR-009 で生成された `rationale` プロパティを XLSX 列 I に転記」と参照する形に変える。

### HTML ビューアでの編集と XLSX 反映

- rationale はビューア上で編集可能（textarea）
- 編集結果は XLSX 出力時に反映される（編集後の値が優先・未編集なら初期値）
- 永続化方式（localStorage 等）と XLSX 出力時の値マージロジックの詳細は **HLD 範疇** とし、SRS では要件のみ明記
- 公開用 HTML エクスポート時の rationale 編集内容の扱いも SRS で明示

## SRS 改訂箇所

| セクション | 変更概要 |
|---|---|
| **3.2 主要コンポーネント構成** | 統合・突合コンポーネントの主要出力を `merged.geojson` に一本化。可視化生成コンポーネントの責務を「`merged.geojson` から HTML ビューア生成」に縮小 |
| **3.3 フェーズ俯瞰** | フェーズ3 末尾を「`merged.geojson`（中心）+ `merged.csv`（派生エビデンス）」に書き換え。`merged_activation.geojson` は内部中間として表示 |
| **FR-008（per-mesh CSV 統合）** | 出力を「内部 work CSV」と位置付け（物理 `merged.csv` の責務は FR-012 へ） |
| **FR-009（SOTA リスト突合・match_status 判定）** | 出力を **`merged.geojson` のフィーチャプロパティ集合**として記述。**※2/※4/※5 テンプレート定義を集約**。`peak_rationale` / `summit_rationale` を生成しフィーチャの `rationale` プロパティに付与。不備フラグの格納先も merged.geojson プロパティに |
| **FR-011（申請書 XLSX 生成）** | ※2/※4/※5 を FR-009 への参照に変更。XLSX 列 I は `rationale` プロパティ（ビューア編集後の値）を転記。HTML ビューア内編集→XLSX 反映の要件を明示 |
| **FR-012（エビデンス CSV 生成）** | 「merged.geojson から派生する CSV」と再定義。**`rationale` 列は含めない**。Polygon 情報も含まない（Point のみが行に変換される） |
| **FR-013（GeoJSON・HTML ビューア生成）** | **merged.geojson 生成をフェーズ3 末尾に前倒し**。フェーズ4 は HTML ビューア生成のみ。フィーチャプロパティ表に `rationale` 行を追加（ピーク・削除サミット）。ISSUE-055 由来の UI 要件（ポップアップ表示項目・編集可否・初期値テンプレート参照・XLSX エクスポート）を統合 |
| **FR-014（独立峰のコル探索）** | 入力は引き続き内部 work CSV（merged.csv 相当）。最終結果は merged.geojson 経由で表現される旨を補足 |
| **FR-018（per-mesh activation 統合）** | 出力 `merged_activation.geojson` を「内部中間ファイル」と明記。物理出力は残す |
| **6.4 出力: エビデンス CSV** | merged.geojson 派生として再定義。`rationale` 列を含めない旨と Polygon 不含を明記 |
| **6.5 出力: GeoJSON**（既存節を中心成果物として書き換え） | 中心成果物として再定義。`rationale` プロパティ仕様（ピーク・削除サミット）を追記 |
| **6.11 中間ファイル: 全国統合済みピーク域 GeoJSON** | 役割を「per-mesh activation の全国統合用の内部中間」と再定義。HTML ビューア生成への直接参照を削除 |

## 新規 ADR-013 の作成

ファイル名: `docs/decisions/ADR-013-merged-geojson-as-central-data.md`

骨子：
- **Context**: 現状の 2 ファイル分割（merged.csv + merged_activation.geojson）はユーザー想定との乖離・rationale 編集機構の設計困難・実装の複雑化を生む
- **Decision**: 中心データを `merged.geojson` 1 つに統一。CSV は派生、activation GeoJSON は内部中間
- **Alternatives**:
  - (A) 現状維持（2 ファイル分割）
  - (B) GeoJSON 中心化 + rationale を localStorage のみに保持（公開用 HTML 配布時に rationale 消失）
  - (C) CSV 中心化（Polygon を扱えないため非現実的）
- **Consequences**: SRS 改訂・ADR-011 への補足追記・ISSUE-043/044/055 のスコープ見直し

## 既存 ADR への波及

### ADR-011（delete-zone-polygon）への補足追記

Consequences の「不備フラグ列は merged.csv に追加」記述を「**不備フラグは merged.geojson のフィーチャプロパティに格納し、merged.csv（派生エビデンス）には含めない**」と書き換える。

### ADR-004 / ADR-010

影響なし（per-mesh 段階の出力フォーマットは変更不要・C エンジンの責務も変わらない）。

## 関連 ISSUE への影響

| ISSUE | 影響 |
|---|---|
| **ISSUE-055**（HTML ビューア UI 要件追加） | 本改訂に統合・対応完了としてクローズ |
| **ISSUE-043**（merge.py: delete判定ゾーン対応） | スコープ要再評価。merge.py が GeoJSON を出力するか・output_geojson.py との責務分担を本 SRS 改訂後に決定。notes に「ADR-013 採用後にスコープ見直し」を追記 |
| **ISSUE-044**（output_geojson.py: delete判定ゾーン対応） | スコープ要再評価。同上 |

## 修正対象ファイル（本セッション）

| ファイル | 修正内容 |
|---|---|
| `docs/02_SRS.md` | 上記改訂箇所すべて |
| `docs/decisions/ADR-013-merged-geojson-as-central-data.md` | 新規作成 |
| `docs/decisions/ADR-011-delete-zone-polygon.md` | Consequences の不備フラグ格納先記述を補足追記 |
| `mgmt/tracker/data/issues.json` | ISSUE-055 クローズ、ISSUE-043/044 notes に再評価メモ追記 |
| `mgmt/tracker/reports/issues_export.xlsx` | 再生成 |

## 新方針データフロー（フェーズ3 末尾）

```
per-mesh CSV     ──→ FR-008 ──→ [内部 work CSV]                ─┐
per-mesh GeoJSON ──→ FR-018 ──→ merged_activation.geojson      │
                                  （内部中間・物理出力残置）         │
                                                                  ↓
                                FR-009: SOTA突合 + match_status判定
                                       + rationale テンプレ展開
                                       + 不備フラグ
                                                ↓
                                       ★ merged.geojson（中心成果物）
                                          ├ Point: peak（rationaleプロパティ含む）
                                          ├ Point: col
                                          ├ Point: summit（削除サミットはrationaleプロパティ含む）
                                          ├ Polygon: activation_zone
                                          ├ Polygon: delete_zone
                                          ├ LineString: peak→col, peak→summit
                                          └ metadata（summitslist_date, generated_at, 不備フラグ）
                                                ↓
                          ┌─────────────────────┴─────────────────────┐
                          ↓                                            ↓
                FR-012: merged.csv（派生）             FR-013: merged_viewer.html
                rationale列なし・Point属性のみ        rationale編集可能（textarea）
                                                              ↓
                                                  FR-011: 申請書XLSX
                                                  （ビューア内生成・編集後のrationaleを反映）
```

## 検証方法

### ドキュメント整合性チェック

- SRS 全 FR の入出力定義が新方針と整合していること
- ADR-013 と SRS の整合
- ADR-011 補足追記が SRS と矛盾しないこと
- `docs/mockup/viewer_mockup.html` の rationale 編集 UI と SRS 要件の整合
- `docs/figures/phases_overview.drawio.svg` の現状フローと SRS 記述のズレ確認（更新は別タスクとして発出）

### 関連 ISSUE スコープ調整の確認

- ISSUE-055 が verify 待ちまで進んでいること
- ISSUE-043/044 の notes に「ADR-013 採用後のスコープ再評価が必要」が記載されていること

## 注意事項・スコープ外

- **本改訂は SRS ステージの作業**。コード（merge.py, output_geojson.py 等）の修正は HLD/COD ステージで対応（ISSUE-043/044 として継続）
- **ADR-010 C++ 移行**（ISSUE-037/038/040）は per-mesh 段階のみが対象で本改訂の影響を受けないため独立して進行可能
- **図表更新**（phases_overview.drawio.svg 等）は本 Plan のスコープ外。別途タスク発出
- **編集機能の HLD 詳細**（localStorage 管理・XLSX マージロジック）は HLD ステージで詰める。SRS では要件のみ
