# SRS 4. 機能要件 の処理フロー組み替え + 関連用語整理（ISSUE-018 統合対応）

## Context

ISSUE-018（FR-009 match_status 整理）の議論中に、ユーザーから「SOTAサミットリスト突合を境に **ピーク中心 → SOTAサミット中心** に処理視点が切り替わる」という構造整理が提示された。これに伴い、SRS 4. 機能要件 の章構成を処理フローに沿って組み替えると同時に、ステータス用語・フィーチャ名・dominant peak の採番方針を一括で整える。

合意済みの変更点（議論経緯）：
1. **フェーズ構成**：ピーク中心と SOTAサミット中心の境界が明確になるよう組み替える
2. **ステータス値の用語**：summit 側を `deleted` → `delete` にリネーム（命令形に揃え、申請アクションと整合）
3. **GeoJSON フィーチャ名**：`sota_summit` → `summit`（プロジェクト内で "summit" は SOTA 限定で使用済み）
4. **dominant peak の `summit_code`**：既存 SummitCode 引き継ぎを廃止し、**仮サミットコード採番** に変更（パターン② の新ピークを「新規候補」として申請書「追加」行に載せるため）
5. **申請書「追加」行の生成対象**：`peak.match_status="new"` のみ → `{"new", "dominant"}` に拡張（パターン② で 2 レコード = 追加+削除 が自然に生成される）

ピーク・サミット両側の独立属性は維持（ADR-007 の方針を継承）：
- `peak.match_status`: `matched` / `new` / `dominant`（3値）
- `summit.match_status`: `matched` / `delete`（2値）

## 新しいフェーズ構成

### フェーズ1：タイル取得・前処理（共通基盤）
- FR-017 N03 行政区域前処理（独立・初回のみ）
- FR-001 標高タイル事前取得
- FR-002 DEM 階層フォールバック
- FR-003 標高デコード・NODATA 処理

### フェーズ2：ピーク中心解析（per-mesh）
- FR-004 3×3 メッシュ結合解析
- FR-005 ピーク候補検出
- FR-006 Keyコル検出・プロミネンス計算
- FR-016 アクティベーションゾーン計算（コル等高線ポリゴン含む）
- FR-007 プロミネンスフィルタ・per-mesh CSV 出力
- FR-014 独立峰対応（レベル14 広域再解析・per-mesh の追加処理として実行）
- FR-015 標高地形図出力

### フェーズ3：ピーク中心統合（全国）
- FR-008 per-mesh CSV 統合
- FR-018 per-mesh activation.geojson 統合

### フェーズ4：SOTAサミット中心（突合・出力）
- FR-009 SOTAリスト突合・match_status 判定
- FR-010 削除候補のスコープ
- FR-013 GeoJSON・HTML ビューア生成
- FR-011 申請書 XLSX 生成
- FR-012 エビデンス CSV 生成

総計 18 FR（変更前と同数）。FR 番号は維持し、所属フェーズと記述順のみ変更する。

## SRS 編集詳細

### docs/02_SRS.md

| 箇所 | 変更内容 |
|---|---|
| 4. 章構成 | 上記新フェーズ構成に沿って FR を並べ替え。各フェーズ冒頭に「ピーク中心」「SOTAサミット中心」の視点切り替え説明を追加 |
| FR-014 入力・出力記述 | 入力を「FR-007 が出力した per-mesh CSV と FR-016 が出力した per-mesh activation.geojson（同一メッシュ）」に変更。出力を「同 per-mesh CSV と activation.geojson を上書き更新」に変更（統合済み merged.csv / merged_activation.geojson の上書きを廃止）。詳細実装は HLD で詰める旨を明記 |
| FR-008 入力 | 「FR-014 で更新済みの per-mesh CSV」を前提とする記述に整える |
| FR-018 入力 | 同上（FR-014 更新済み per-mesh activation.geojson 前提） |
| FR-009 入力 | 「FR-008 / FR-018 が出力した統合済みファイル」と素直に書ける（FR-014 の後処理に言及不要） |
| FR-009 match_status 判定 | (a) 判定順序を明示：①AZ 内 → matched ②コル等高線内 → dominant ③それ以外 → new（peak 側）。summit 側は ①AZ 内 → matched ②AZ 外 → delete<br>(b) summit.match_status の独立定義文を追加<br>(c) dominant ピークの `summit_code` を **仮サミットコード採番** に変更（既存 SummitCode 引き継ぎを廃止）<br>(d) ADR-008 フォールバック時：紐付け先 peak の match_status は変更しない（dominant に昇格させない）旨を明記 |
| FR-011 アクション別カラムマッピング | 「追加」行の生成対象を `peak.match_status in {"new", "dominant"}` に拡張。<br>※2 追加根拠フォーマットは現状維持（dominant も同フォーマットで自然に成立） |
| FR-012 エビデンス CSV | `match_status` の値域記述を更新。`summit_code` 説明を「matched は正式コード、new / dominant は仮サミットコード」に変更。`sota_lat/lon/alt_m/points` の補足を「matched / dominant のみ」（dominant の場合は削除候補サミットの情報）と整える |
| FR-013 フィーチャ構成表 | `type: "sota_summit"` → `type: "summit"` 全箇所置換。dominant 行のプロパティ説明も同様 |
| FR-013 HTML ビューア仕様 | 「申請書エクスポート」記述の「sota_summit feature の match_status="deleted"」を「summit feature の match_status="delete"」に更新 |
| 6.5 / 6.10 / 6.11 / 6.12 | feature 名 `sota_summit` → `summit` を反映 |
| 6.5 GeoJSON プロパティ表 | match_status 値域を peak / summit で分けて明記 |

### docs/00_GLOSSARY.md

| 箇所 | 変更内容 |
|---|---|
| 「サミット」項目 | GeoJSON feature 名としても `summit` を使うことを補足 |
| 「dominant peak」項目 | リンク先 ADR-007 の更新版に追随（記述自体は概ね現行で OK） |
| 新規追加「削除（delete）」 | summit の状態としての `delete` の定義を追加（FR-010 スコープ内 かつ いずれの検出ピークの AZ にも包含されない） |

### docs/decisions/ADR-007-peak-match-status-terminology.md

| 箇所 | 変更内容 |
|---|---|
| Decision テーブル | `sota_summit feature の match_status`：`matched / deleted` → `matched / delete`（命令形に統一）<br>feature 名のリネーム `sota_summit → summit` を変更対象に追加 |
| Consequences | feature 名リネームの影響を追記。merge.py 実装での `deleted` → `delete` 出力値変更 |

### docs/decisions/ADR-008-dominant-peak-identification.md

| 箇所 | 変更内容 |
|---|---|
| 全体 | feature 名 `sota_summit` → `summit` に揃える |
| Decision | dominant peak の `summit_code` 採番方針が「仮サミットコード」であることを明示（FR-009 と整合） |
| Consequences | フォールバック時に紐付け先 peak の `match_status` を dominant に昇格させない旨を追記（ISSUE-018 H-3 案 B） |

### docs/mockup/viewer_mockup.html

| 箇所 | 変更内容 |
|---|---|
| ダミー GeoJSON | `type: "sota_summit"` → `type: "summit"`、`match_status: "deleted"` → `"delete"` |
| `categorize()` 関数 | `=== "deleted"` → `=== "delete"`（summit 側）。dominant ラベルは維持 |
| `filterGroups` / `data-cat` | summit 側の値を `delete` に統一 |
| popup バッジ・凡例 | 内部値を更新（表示ラベル「削除」は維持） |

### mgmt/tracker への反映

- ISSUE-018 を本作業完了時に `close`（本変更で対応方針 (1)(2)(3)(4)(5) を一括解消）
- ISSUE-019（仮サミットコード採番順序）：本変更で dominant ピークにも仮コード採番が必要になるため、採番ルールに「new と dominant を区別するか」を含めるよう Description を補強する必要あり
- ISSUE-021（FR-014 上書きスコープ明確化）：本変更で「FR-014 = per-mesh の追加処理。出力は per-mesh CSV / activation.geojson の上書き」が確定するため、Description にその方針を明記し、HLD で per-mesh 実装の詳細を詰める旨を残す

## 残課題（本変更のスコープ外・後で議論）

- ADR-008 フォールバック時の細部（包含なしの再現条件・実データでの発生率）
- matched peak が別の delete 候補 SOTA をコル等高線内に含む edge case の扱い（ユーザー所感：実データに出ない見込み）
- 仮サミットコード採番順序（ISSUE-019）
- 手動調査待ちピークの GeoJSON フィーチャ扱い（ISSUE-020）
- FR-014 per-mesh 実装の詳細（同じピークが複数メッシュの per-mesh 解析で別個に flagged になる場合の再解析重複回避ルール等）→ HLD で詰める

## 実行順序

1. SRS 章構成の組み替え（4. 機能要件 全体を新フェーズ順に並べ替え）
2. FR-009 本文の判定順序・summit 独立定義追加
3. FR-011 / FR-012 / FR-013 の「dominant 採番方針変更」と feature 名リネーム反映
4. 6.x 外部インターフェース仕様の整合更新
5. GLOSSARY 更新（用語追加・補足）
6. ADR-007 / ADR-008 更新
7. mockup HTML 更新
8. ISSUE-018 を close、ISSUE-019 の Description 補強

## 検証方法

- SRS 目次・章構成が「フェーズ1〜4」の新構成になっていること
- 4. 機能要件 を通読し、FR-009 の判定順序と summit.match_status の独立定義が明示されていること
- `grep -n "sota_summit" docs/` で残骸ゼロを確認
- `grep -n "deleted" docs/02_SRS.md docs/00_GLOSSARY.md docs/decisions/ADR-007*.md docs/decisions/ADR-008*.md docs/mockup/viewer_mockup.html` で summit 文脈の残骸ゼロを確認（peak の `dominant` ラベルや申請書の「削除」表示ラベルは別物）
- モックアップを `file://` で直接開き、ダミー dominant ピーク + delete summit が想定どおりフィルタ・表示されること
- ISSUE-018 のチェック項目 (1)〜(5) を SRS 該当箇所で確認

## 主要変更ファイル

- `docs/02_SRS.md`
- `docs/00_GLOSSARY.md`
- `docs/decisions/ADR-007-peak-match-status-terminology.md`
- `docs/decisions/ADR-008-dominant-peak-identification.md`
- `docs/mockup/viewer_mockup.html`
- `mgmt/tracker/...`（ISSUE-018 close、ISSUE-019 Description 補強）
