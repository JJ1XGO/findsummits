# SOTA 申請「変更」基準の再定義 — URD/SRS/GLOSSARY 改訂

## Context

SOTA 日本支部への申請対象「変更」の意味を再定義する。

- **旧仕様**: 「変更」= 山岳名の変更のみ（HTML ビューアでユーザーが手動記入）
- **新仕様**: 「変更」= matched ペアで **Points バンドをまたぐ標高差** があるケース（自動識別）

**ユーザー判断**:「Points が変わらない変更は SOTA 本部側に意味のない作業を依頼するだけ。Points が変わるなら意味がある」「事実は事実として申請したい」。山岳名変更はピーク解析と無関係なので申請対象から外す。

本タスクは SRS フェーズの仕様改訂のみ。コード実装（merge.py / output_geojson.py 等）は別 ISSUE に切り出す。

## 確定済み判断

| 項目 | 決定 |
|---|---|
| 標高バンド境界値 | SOTA 日本支部公式（全国統一・地域依存なし）。150/500/650/850/1100/1500m で 1/2/4/6/8/10pt |
| 出典 | SOTA 日本支部参照マニュアル 2025年7月改定版 |
| peak_elev の丸め | **切り捨て**（floor）で整数 m に変換（1499.5m は 1499m → 8pt。四捨五入は不可） |
| バンド判定タイミング | **丸めた後の値**でバンド判定（申請書記入値との整合性確保） |
| 新規 ADR | 不要（公式値で設計裁量なし） |
| Phase B（実データ検証） | **不実施**（ユーザー判断） |
| 名称変更入力 UI | 完全廃止（HTML ビューア） |

## Phase A: ドキュメント改訂

### A-1. GLOSSARY 改訂（docs/00_GLOSSARY.md）

「データソース関連」内の SOTA 関連項目に隣接して、標高バンド表を追加:

```
### 標高バンド（Points 算出表）

SOTA 日本支部参照マニュアル（2025年7月改定版）に基づく全国統一値。

| 標高 [m]            | Points |
|--------------------|--------|
| 150 ≤ h < 500      | 1      |
| 500 ≤ h < 650      | 2      |
| 650 ≤ h < 850      | 4      |
| 850 ≤ h < 1100     | 6      |
| 1100 ≤ h < 1500    | 8      |
| 1500 ≤ h           | 10     |

出典は ref/SOURCES.md を参照。本表が points / sota_points 算出の正となる。
```

`ref/SOURCES.md` にもマニュアル出典を追加（URL: `https://www.kawauchi.homeip.mydns.jp/sotajp/やってみよう/` のセクション 8）。

### A-2. URD 改訂（docs/01_URD.md）

- **UR-003（L43）**: 「現行 SOTA リストと突合し、新規・削除・**変更（標高バンドを跨ぐ標高差があるもの）**の候補を識別できること（既存サミットの名称変更・座標変更・バンドを跨がない標高変更は自動識別対象外）」
- **6. スコープ外（L74）**: 旧文を以下 3 項目に分割
  - 既存サミットの**名称変更**申請（自動識別せず）
  - 既存サミットの**座標変更**申請（自動識別せず）
  - **バンドを跨がない標高変動**（Points 値が変わらないため SOTA 本部にとって意味のない変更）

### A-3. SRS 改訂（docs/02_SRS.md）

| 改訂対象 | 内容 |
|---|---|
| **FR-009 新セクション** | 「標高バンド・Points 算出」追加。半開区間定義、切り捨て（floor）による整数 m 丸め、丸め後値でのバンド判定を明記 |
| **FR-009 突合ロジック** | matched ペアに対し `sota_points = band(sota_alt_m)`、`peak_points = band(floor(peak_elev))`、`is_band_change_candidate = (sota_points ≠ peak_points)` を算出 |
| **FR-007（L219-239）** | per-mesh CSV カラム表に `points` 列追加（peak_elev からの算出値）|
| **FR-008（L254-267）** | merged.csv に `points` / `sota_points` / `is_band_change_candidate` 列追加 |
| **FR-011（L497-523）** | 申請書「変更」行マッピングを書き換え（変更前 = sota_alt_m、変更後 = floor(peak_elev)、山岳名は変更前後同値）。**対象 = `is_band_change_candidate=true` の matched 行**|
| **FR-012（L525-558）** | merged.csv 出力カラム表を FR-008 の追加列に追従 |
| **FR-013（L478-495）** | matched ピーク用の名称修正入力フィールド（L480）と、関連 UI / localStorage を**削除**。XLSX エクスポートの「変更」行は `is_band_change_candidate=true` から自動生成 |
| **9. スコープ外（L842）** | URD と同期した文に置換 |

変更根拠文（FR-011 出力時の自動生成）テンプレ案:
```
国土地理院 DEM 解析による標高再測定: {sota_alt_m}m ({sota_points}pt) → {floor(peak_elev)}m ({peak_points}pt)
座標: {peak_lat},{peak_lon}（{都道府県} {市区町村}）
```

### A-4. ISSUE 整理（mgmt/tracker/data/issues.json）

| ISSUE | 操作 |
|---|---|
| ISSUE-025（標高バンド境界値の規定） | **本タスク完了時に close → verify**（A-1 で吸収済み） |
| ISSUE-003（output_xlsx.py 新規作成） | notes 更新: 「MOVED/ELEV_CHANGE シート」言及を「FR-011 最新仕様の変更行 = バンド遷移 matched」に置換 |
| ISSUE-043（merge.py: AZ/delete-zone PIP 等） | 作業範囲に「`points` / `sota_points` / `is_band_change_candidate` 列追加」を追記 |
| ISSUE-044（output_geojson.py + merged_viewer.html） | 作業範囲に「名称修正 UI 廃止 + 変更行自動エクスポート（バンド遷移 matched）」を追記 |
| **ISSUE-046（新規）** | 「SRS: 変更申請判定を Points バンド遷移基準に変更（URD/SRS/GLOSSARY 改訂）」を進捗管理用に登録 |

## Commit 粒度（Phase A）

1. `docs(glossary): SOTA 標高バンド表を追加（出典: 参照マニュアル 2025年7月版）`
2. `docs(urd): UR-003 / スコープ外を Points バンド遷移基準に再定義`
3. `docs(srs): FR-009 に標高バンド・Points 算出ロジックを追加`
4. `docs(srs): FR-007/008/012 で points / sota_points / is_band_change_candidate を出力カラムに追加`
5. `docs(srs): FR-011 変更行マッピング更新、FR-013 名称修正 UI を廃止`
6. `mgmt: ISSUE-025 close、ISSUE-003/043/044 notes 更新、ISSUE-046 新規登録`

## 検証

- Phase A 完了後、URD → GLOSSARY → SRS の論理整合性を読み合わせ（用語・参照リンク・スコープの一貫性）
- `mgmt/tracker/track.py issue show ISSUE-025` で本タスク完了後に「対応完了」化、ユーザー verify 待ち
- `mgmt/tracker/track.py issue show ISSUE-046` で SRS 改訂完了を「対応完了」化、ユーザー verify 待ち
- 実装系 ISSUE-003/043/044 は notes 更新のみ（ステータス変更なし）

## 関連ファイル（絶対パス）

- `/workspace/docs/00_GLOSSARY.md`
- `/workspace/docs/01_URD.md`
- `/workspace/docs/02_SRS.md`
- `/workspace/ref/SOURCES.md`
- `/workspace/mgmt/tracker/data/issues.json`
- `/workspace/ref/SOTA-Summit-list-revision-request.xlsx`（参照のみ）

## 参考（実装には踏み込まない）

- `scripts/merge.py` には現在 `points` / `Points` / `band` の処理一切なし → ISSUE-043 で実装
- `analysis/sota_dem_elevation_diff.py` は標高差分析専用で Points 計算なし → 本タスクと無関係
- `ref/summitslist.csv` の Points 列（第 11 列、値 1/2/4/6/8/10）は突合の参照値として既に存在
