# 計画: GLOSSARY・SOURCES へのアクティベーションゾーン追加

## Context

SRS でアクティベーションゾーン（SOTA 公式呼称: アクティベーションゾーン）という新用語が登場した。
GLOSSARY と SOURCES に定義・出典を追加し、docs 配下をコミットする。

---

## 変更 A: docs/00_GLOSSARY.md — 地理・メッシュ関連テーブルに行追加

追加する用語（「サミット」行の直後に挿入）:

| 用語 | 正式名称 | 説明 |
|---|---|---|
| アクティベーションゾーン（アクティベーションゾーン） | Activation Zone | SOTAルールにおける山頂での運用可能エリア。山頂の最高地点から標高差 25m 以内の連続エリアをさす。このエリア内での無線運用が「山頂からの運用」として認められる。SOTA日本支部では「アクティベーションゾーン」と表記（参照: [SOTA日本支部 FAQ Q12](https://www.kawauchi.homeip.mydns.jp/sotajp/faqs/)）。本プロジェクトではピークと既存SOTAサミットの照合に使用する（FR-016）。 |

---

## 変更 B: ref/SOURCES.md — SOTA日本支部 FAQ を参照文書として追加

「参照文書（ファイル未格納）」セクションに追加:

```
### SOTA日本支部 FAQ

| 項目 | 内容 |
|---|---|
| タイトル | SOTA日本支部 よくある質問（FAQ） |
| 提供元 | SOTA日本支部 |
| URL | https://www.kawauchi.homeip.mydns.jp/sotajp/faqs/ |
| 備考 | アクティベーションゾーンの定義（Q12: 山頂の最高地点から標高差25m以内のエリア）等、SOTAルールの参照に使用。 |
```

---

## 変更対象ファイル

- `docs/00_GLOSSARY.md`
- `ref/SOURCES.md`

---

## 未解決課題（要検討）

- **ISSUE候補: matched の標高違いの扱い**: matched かつ `peak_elev ≠ sota_alt_m` の場合、申請書に「変更」として出力するかどうか、また閾値（何m以上の差異で変更扱いにするか）をユーザーが検討中。課題登録待ち。

---

<!-- 以下は実装済み（前セッション）-->
# 実装済み: SRS セクション3・FR-009・FR-013 の設計変更

## Context（実装済み）

SRS レビュー中に以下の設計変更が決定した。これらを docs/02_SRS.md に反映する。

主な変更点:
1. セクション3 アーキテクチャ図: 3ステップ運用フロー・フェーズ番号追記
2. FR-009 マッチング方式: tolerance_px 廃止 → アクティベーションゾーン Flood Fill 方式
3. FR-013 GeoJSON フィーチャ構成: アクティベーションゾーンポリゴン追加（TBD-03 解消）
4. 新規 FR: アクティベーションゾーン計算（findsummits の新機能）

---

## 変更 A: セクション3 アーキテクチャ図（3ステップ運用フロー・フェーズ番号追記）

アーキテクチャ図のスクリプト名とセクション4のフェーズ名の対応が分かりにくいため、
図にフェーズ番号と3ステップの運用フローを添える。
merge.py が GeoJSON + HTML を出力することを確定。

```
【ステップ1: タイル取得】
prefetch_tiles.py    タイル事前取得（フェーズ1）
       ↓
【ステップ2: 解析・突合・確認】（GeoJSON/HTML で結果を確認してから次ステップへ）
findsummits (C)      山頂・コル検出・アクティベーションゾーン計算（フェーズ2）
       ├─ per-mesh CSV              ($DATA_DIR/results/csv/<meshcode>.csv)
       ├─ per-peak activation area  ($DATA_DIR/results/csv/<meshcode>_activation.geojson)
       └─ 標高地形図                ($DATA_DIR/images/<meshcode>_terrain.png)
merge.py (Python)    SOTA 突合・差分分類（フェーズ3）
       ├─ merged.csv         ($DATA_DIR/results/merged.csv)
       ├─ merged.geojson     ← 目視確認用 GeoJSON
       └─ merged_viewer.html ← 目視確認用 静的 HTML ビューア
【ステップ3: 申請書生成】（確認済みの場合のみ実行）
output.py (Python)   申請書 XLSX 生成（フェーズ4）
       └─ submission.xlsx
```

---

## 変更 B: FR-009 マッチング方式変更（tolerance_px 廃止）

### 現行方式（廃止）
Chebyshev 距離 `tolerance_px` による pixel座標マッチング。
`tolerance_px = 0` はピーク検出精度の確認を優先するための意図的な設定（マッチングを一時無効化）。
検出精度が十分に高いことが確認できたため、マッチング方式をより高精度なものに刷新する。

### 新方式: アクティベーションゾーン Flood Fill
SOTAルールの「アクティベーションゾーン（サミットから標高差25m以内のエリア）」を活用する。

**findsummits（C）が担当:**
- 各ピークに対して Flood Fill を実行: `elev ≥ peak_elev − 25.0m` の連続エリア
- 輪郭座標列（GeoJSON Polygon）を出力
- Flood Fill が解析範囲外で途切れた場合: `area_truncated: true` フラグを付与（is_tile_top と同様の扱い）
- 出力先: `$DATA_DIR/results/csv/<meshcode>_activation.geojson`（ピークごとのアクティベーションゾーン）

**merge.py（Python）が担当:**
- `ref/summitslist.csv` から JA プレフィックスのサミット座標を読み込む
- 各ピークのアクティベーションゾーンポリゴンに対して point-in-polygon 判定
- マッチング一意性: プロミネンス≥150m の制約により、1アクティベーションゾーン内に複数SOTAサミットは数学的に存在不可

**match_status 判定:**
| match_status | 条件 |
|---|---|
| matched | 検出ピークのアクティベーションゾーン内に既存SOTAサミット座標が存在する |
| new | 検出ピークのアクティベーションゾーン内に既存SOTAサミット座標が存在しない |
| deleted | いずれの検出ピークのアクティベーションゾーンにも含まれない既存SOTAサミット |

---

## 変更 C: FR-013 GeoJSON フィーチャ構成の更新（TBD-03 解消）

### GeoJSON フィーチャ構成

match_status 別に以下のフィーチャを生成する:

| match_status | フィーチャ |
|---|---|
| matched | Point（ピーク）+ Polygon（アクティベーションゾーン）+ Point（Keyコル）+ Point（SOTA サミット）+ LineString（ピーク→Keyコル）+ LineString（ピーク→SOTA サミット） |
| new | Point（ピーク）+ Polygon（アクティベーションゾーン）+ Point（Keyコル）+ LineString（ピーク→Keyコル） |
| deleted | Point（SOTA サミット）のみ |

### 各フィーチャのプロパティ

**Point: 検出ピーク**
- `type`: "peak"
- `match_status`: matched / new
- `summit_code`: SOTA サミットコード（matched のみ）
- `summit_name`: サミット名（matched のみ）
- `peak_elev`: 検出標高（m）
- `prominence`: プロミネンス（m）
- `stability`: confirmed / unstable
- `is_tile_top`: 0 / 1

**Polygon: アクティベーションゾーン**
- `type`: "activation_area"
- `peak_elev`: 対応ピーク標高（m）（ピーク Point との対応付け用）
- `area_truncated`: true / false（Flood Fill が解析範囲外で途切れた場合 true）

**Point: Keyコル**
- `type`: "col"
- `col_elev`: Keyコル標高（m）
- is_tile_top=1 の場合は含めない（col_lat/col_lon が 0.0 のため）

**Point: 既存 SOTA サミット**
- `type`: "sota_summit"
- `match_status`: matched / deleted
- `summit_code`: SOTA サミットコード
- `summit_name`: サミット名
- `sota_alt_m`: SOTA 登録標高（m）

**LineString: ピーク → Keyコル**
- `type`: "prominence_range"
- is_tile_top=1 の場合は生成しない

**LineString: ピーク → SOTA サミット**
- `type`: "coord_diff"
- matched のみ生成

### viewer.html のインタラクション（固定テンプレート方式）

- ピーク Point クリック → 対応するアクティベーションゾーンポリゴンを赤くハイライト
- ハイライト時にアクティベーションゾーン内の既存SOTAサミットの有無を視覚確認できる
- `area_truncated: true` の場合はポリゴン表示に警告色（例: 橙）を付与

---

## 変更対象ファイル

- `docs/02_SRS.md`（セクション3・FR-009・FR-013・TBD テーブル）

## 変更後の TBD

- TBD-03: 解消（GeoJSON フィーチャプロパティ・アクティベーションゾーンを本計画で確定）
- ISSUE-006（`tolerance_px = 0`）: 意図的な一時設定。FR-009 のアクティベーションゾーン方式への移行により解消

---

## 申請書アクションの整理（確認済み）

| match_status | 差異 | 申請書アクション |
|---|---|---|
| matched | なし | 変更なし（申請書に記載不要）|
| matched | 標高差あり | 変更 |
| new | — | 追加 |
| deleted | — | 削除 |

実態: 既存リストが正確な地域では matched+変更なしが大多数。申請書は「追加・削除（まれに変更）」のみ。

### 削除パターン（確認済み）

| パターン | 説明 |
|---|---|
| 従属ピーク（サブピーク） | 局所最大点だが隣接高峰とのKey col差 < 150m（プロミネンス不足） |
| ピーク未検出 | DEM上に局所最大点なし（平坦地形・解像度不足） |
| プロミネンス < 130m | findsummits一次フィルタで除外（DEM計算誤差含む） |

### 未解決課題（要検討）

- **ISSUE候補: matched の標高違いの扱い**: matched かつ `peak_elev ≠ sota_alt_m` の場合、申請書に「変更」として出力するかどうか、また閾値（何m以上の差異で変更扱いにするか）をユーザーが検討中。課題登録待ち。
