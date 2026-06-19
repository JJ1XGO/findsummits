# ADR-SRS-013: merged.geojson を中心成果物とするデータモデルへの移行

| 状態 | 採用・未実装 |
| 決定日 | 2026-05-28 |

## Context

### 現状の 2 ファイル分割構造

フェーズ3 末尾の中心データが以下のように分割されており、設計上の課題を生んでいた：

- `merged.csv`（FR-012 規定）: ピーク・サミットの属性データ（Point のみ）
- `merged_peak.geojson`（FR-018/6.11 規定）: ポリゴン（活性化ゾーン・delete判定ゾーン）のみ・Point フィーチャなし

最終的な `merged.geojson` は FR-013（GeoJSON/HTML ビューア生成）で上記 2 ファイルを統合して組み立てる構造であった。

### 課題

1. **rationale プロパティの格納場所問題**: 申請書 XLSX の列 I（根拠テキスト）を HTML ビューアで編集可能にするには、`rationale` プロパティをピーク・サミットフィーチャに持たせる必要がある。`merged_peak.geojson` は Point フィーチャを持たず、`merged.csv` にカラム追加する案はポリゴンを持てないため中心データにはなれない

2. **中心データのイメージ乖離**: ユーザーの本来のイメージは「バッチ処理完了時に全結果が集約した 1 つの中心データが出来ていて、HTML ビューアはそれを表示するだけ」というものだったが、現設計では 2 ファイルを FR-013 で統合するまで中心データが存在しなかった

3. **不備フラグの格納場所**: ADR-SRS-011 では不備フラグを merged.csv 列に追加するとしていたが、中心 GeoJSON を中心とするなら metadata プロパティに持つ方が自然

4. **dominant ケースの 2 行問題**: dominant ピーク 1 エントリは申請書 XLSX で「追加（dominant）」と「削除（既存サミット）」の 2 行に展開され、それぞれ異なる根拠（※2 と ※4）が必要。Point フィーチャが独立していれば各フィーチャに rationale を持たせることで自然に解決できる

## Decision

### 中心データを GeoJSON 1 つに統一

FR-009（SOTA リスト突合・match_status 判定）の出力を `merged_summit.geojson` とし、フェーズ3 末尾の中心成果物と位置付ける（※ 本 ADR 決定時のファイル名は `merged.geojson` であったが、[ADR-SRS-030](ADR-SRS-030-rename-central-geojson-merged-summit.md) によって `merged_summit.geojson` へリネームされた）：

| ファイル | 新しい役割 |
|---|---|
| `merged_summit.geojson` | **フェーズ3 末尾の中心データ**（全 Point + 全 Polygon + rationale + 不備フラグを含む） |
| `merged.csv` | `merged_summit.geojson` から派生する**エビデンス CSV**（UR-005 対応）。`rationale` 列は含めない |
| `merged_peak.geojson` | per-mesh activation 統合の**内部中間ファイル**（デバッグ・差分検査用）。物理出力は残す |
| `merged_viewer.html` | フェーズ4 で `merged_summit.geojson` のみを入力に生成（責務縮小） |

### merged_summit.geojson のフィーチャ構成

```
merged_summit.geojson
├ Point: peak（new/dominant/matched_band_change に rationale プロパティ付与）
├ Point: col
├ Point: summit（match_status=delete に rationale プロパティ付与）
├ Polygon: activation_zone
├ Polygon: delete_zone
├ LineString: peak→col 接続線
├ LineString: peak→summit 接続線
└ metadata（summitslist_date, generated_at, 不備フラグ）
```

### rationale プロパティの配置

- **ピークフィーチャ**（new / dominant / matched_band_change）: FR-009 で `rationale` プロパティを付与（※2 追加根拠 or ※5 変更根拠。テンプレート定義は FR-009 に集約）
- **削除サミットフィーチャ**（match_status=delete）: FR-009 で `rationale` プロパティを付与（※4 削除根拠）

dominant 行は申請書 XLSX で 2 行（追加 + 削除）に展開されるため、ピーク Point と削除サミット Point の 2 つに独立した rationale を持たせる。Point フィーチャが独立しているため自然に両立する。

### テンプレート定義の集約

※2/※4/※5 のフォーマット定義を FR-009 に集約する。FR-011 からは「FR-009 で生成された `rationale` プロパティを XLSX 列 I に転記」と参照する形に変える。

### rationale の編集と XLSX 反映

- HTML ビューア（merged_viewer.html）で rationale を編集可能（textarea）とする
- 編集後の rationale が申請書 XLSX 出力（FR-011）に反映される
- 永続化方式（localStorage 等）と XLSX 出力時の値マージロジックの詳細は HLD 範疇

## Alternatives

### 案 A: 現状維持（2 ファイル分割）（不採用）

`merged.csv` + `merged_peak.geojson` の 2 ファイル分割を維持し、FR-013 で統合する案。`rationale` プロパティの格納場所問題が解決できない。`merged_peak.geojson` は Point フィーチャを持たないため、ピーク・サミットの rationale を持てない。`merged.csv` に rationale 列を追加しても、ポリゴンフィーチャの rationale との統一的な管理ができない。

### 案 B: GeoJSON 中心化 + rationale を localStorage のみに保持（不採用）

中心データを `merged_summit.geojson` に統一するが、rationale は HTML ビューアの localStorage にのみ保持する案。公開用 HTML をエクスポートして別端末で開いた場合に rationale が消失する。`merged_summit.geojson` の `rationale` プロパティとして保持することで、HTML エクスポート時にも rationale が埋め込まれ消失しない。

### 案 C: CSV 中心化（非現実的・不採用）

`merged.csv` を中心データとし、ポリゴン情報を CSV に格納する案。CSV は Polygon/LineString ジオメトリを表現できないため非現実的。

## Consequences

### SRS への影響

| セクション | 変更内容 |
|---|---|
| **3.2 主要コンポーネント構成** | 統合・突合コンポーネントの主要出力を `merged_summit.geojson` に一本化 |
| **3.3 フェーズ俯瞰** | フェーズ3 末尾を「`merged_summit.geojson`（中心）+ `merged.csv`（派生エビデンス）」に書き換え |
| **FR-008** | 出力を「内部 work CSV」と位置付け |
| **FR-009** | 出力を `merged_summit.geojson` として記述。※2/※4/※5 テンプレート集約。rationale 生成要件追加 |
| **FR-011** | ※2/※4/※5 を FR-009 参照に変更。XLSX 列 I は rationale プロパティを転記 |
| **FR-012** | 「`merged_summit.geojson` から派生する CSV」と再定義。`rationale` 列を含めない |
| **FR-013** | `merged_summit.geojson` 生成をフェーズ3 末尾に前倒し。フェーズ4 は HTML ビューア生成のみ |
| **FR-018** | 出力 `merged_peak.geojson` を「内部中間ファイル」と明記 |
| **6.4/6.5/6.11** | 出力物・中間ファイルの役割を新方針に合わせて再定義 |

### 既存 ADR への波及

- **ADR-SRS-011**（delete-zone-polygon）: Consequences の「不備フラグ列は merged.csv に追加」記述を「不備フラグは `merged_summit.geojson` のフィーチャプロパティ（metadata）に格納し、merged.csv（派生エビデンス）には含めない」に補足追記
- **ADR-SRS-030**（rename-central-geojson-merged-summit）: 本 ADR 決定時のファイル名 `merged.geojson` を `merged_summit.geojson`（和名「突合済み統合 GeoJSON」）へリネームした。本 ADR の本文は最新名に更新済み
- **ADR-SRS-033**（defect-confirmation-via-xlsx）: 本 ADR で決定した「不備フラグを `merged_summit.geojson` metadata に格納する」設計を改訂。top-level boolean 不備フラグは metadata から削除し、不備確認の責務を `merged_summit.xlsx`（per-row 表示）へ移行する。per-feature プロパティ（`key_col_resolved`・`area_complete`）は維持する。
- **ADR-SRS-004 / ADR-SRS-010**: 影響なし（per-mesh 段階の出力フォーマットは変更不要）

### 関連 ISSUE への影響

| ISSUE | 影響 |
|---|---|
| **ISSUE-055**（HTML ビューア UI 要件追加） | 本改訂に統合・クローズ |
| **ISSUE-043**（merge.py: delete判定ゾーン対応） | スコープ再評価が必要。merge.py が GeoJSON を出力するか・output_geojson.py との責務分担は本改訂後に決定 |
| **ISSUE-044**（output_geojson.py: delete判定ゾーン対応） | スコープ再評価が必要。同上 |

### スコープ外

- コード（merge.py, output_geojson.py 等）の修正は HLD/COD ステージで対応（ISSUE-043/044 として継続）
- rationale 永続化方式・XLSX マージロジックの詳細は HLD ステージで決定
