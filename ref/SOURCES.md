# 参照資料 出典一覧

本ディレクトリに格納されている参照ファイルおよび関連参照文書の出典を記載する。

---

## summitslist.csv

| 項目 | 内容 |
|---|---|
| タイトル | SOTA山岳リスト |
| 提供元 | [SOTA Database](https://www.sotadata.org.uk/) |
| 取得元URL | https://www.sotadata.org.uk/summitslist.csv |
| 備考 | JAで始まるサミットが日本支部対象。定期的に更新されるため再取得時は日付を確認すること。 |

---

## SOTA-Summit-list-revision-request.xlsx

| 項目 | 内容 |
|---|---|
| タイトル | SOTA日本支部 山岳データ登録変更申請書 |
| 提供元 | [SOTA日本山岳リスト](https://www.kawauchi.homeip.mydns.jp/sotajp/) |
| 取得元URL | https://www.kawauchi.homeip.mydns.jp/sotajp/wp-content/uploads/2024/03/SOTA-Summit-list-revision-request.xlsx |
| 備考 | 申請書テンプレート。1シート構成。アクション列に追加・変更・削除・その他を選択して申請する。 |

---

## geojson_v{N}/（バージョン番号付きディレクトリ）

ディレクトリ名はダウンロード時のバージョン番号を含む（例: `geojson_v31/`）。
更新時は新バージョンのディレクトリを追加し、古いバージョンは削除して運用する。

| 項目 | 内容 |
|---|---|
| タイトル | SOTA日本山名リスト GeoJSON |
| 提供元 | [ジオサミットでひとこえ](https://little-ctc.com/sota_hp/geojson/) |
| 取得元URL | https://little-ctc.com/?sdm_process_download=1&download_id=11249 |
| ファイル構成 | ja0〜ja9（エリア別10ファイル）。ja0=全国、ja1〜ja9=各地方ブロック。 |
| 備考 | 国土地理院地図での目視確認・申請内容のエビデンス用途で参照する。 |

---

## 国土地理院標高タイル（データソース）

| 項目 | 内容 |
|---|---|
| 提供元 | 国土地理院 |
| データ種別 | DEM5a / DEM5b / DEM5c（ズームレベル 15）、DEM10b（ズームレベル 14） |
| 参照 URL | https://maps.gsi.go.jp/development/ichiran.html |
| 利用規約 | 国土地理院コンテンツ利用規約（https://www.gsi.go.jp/kikakuchousei/kikakuchousei40182.html） |
| 帰属表示義務 | 利用成果物に「国土地理院」の帰属表示が必要 |
| 本プロジェクトでの利用形態 | タイルをローカルキャッシュとして保存して標高解析に使用。タイルデータ自体はリポジトリに含めない（.gitignore）。生成する HTML ビューアの帰属表示に `© 国土地理院` を含める。 |

---

## OpenStreetMap（地図タイル）

| 項目 | 内容 |
|---|---|
| 提供元 | OpenStreetMap contributors |
| 利用規約 | Open Database License (ODbL) — https://www.openstreetmap.org/copyright |
| 帰属表示義務 | 利用成果物に `© OpenStreetMap contributors` の表示が必要 |
| 本プロジェクトでの利用形態 | HTML ビューアの背景地図タイルとして使用（国土地理院地図・OpenTopoMap との切り替え用） |

---

## OpenTopoMap（地図タイル）

| 項目 | 内容 |
|---|---|
| 提供元 | OpenTopoMap contributors |
| 利用規約 | CC-BY-SA — https://opentopomap.org/about#verwendung |
| データソース | OpenStreetMap データ（ODbL）+ SRTM 標高データ |
| 帰属表示義務 | 利用成果物に `© OpenTopoMap contributors` の表示が必要 |
| 本プロジェクトでの利用形態 | HTML ビューアの背景地図タイルとして使用（等高線・山名確認用） |

---

## 国土地理院 基準点タイル（データソース）

| 項目 | 内容 |
|---|---|
| 提供元 | 国土地理院 |
| データ種別 | 基準点（電子基準点・一等／二等／三等三角点）。ベクトルタイル（GeoJSON 形式） |
| 取得 URL | `https://cyberjapandata.gsi.go.jp/xyz/cp/{z}/{x}/{y}.geojson` |
| 参照 URL | https://maps.gsi.go.jp/development/ichiran.html |
| 利用規約 | 国土地理院コンテンツ利用規約（https://www.gsi.go.jp/kikakuchousei/kikakuchousei40182.html） |
| 帰属表示義務 | 利用成果物に「国土地理院」の帰属表示が必要 |
| 本プロジェクトでの利用形態 | HTML ビューアの「基準点」参照レイヤー（デフォルト OFF）で、表示範囲のタイルをブラウザから実行時取得して描画する。タイルデータはリポジトリに含めない。レイヤー ON 時にビューアの帰属表示へ基準点データの出典として `国土地理院` を併記する。採用経緯: [ADR-SRS-034](../docs/decisions/ADR-SRS-034-viewer-reference-layers.md) |

---

## 参照文書（ファイル未格納）

### SOTA日本支部 参照マニュアル（標高バンド・ポイント算出ルール）

| 項目 | 内容 |
|---|---|
| タイトル | SOTA日本支部 参照マニュアル 2025年7月改定版 |
| 提供元 | [SOTA日本支部](https://www.kawauchi.homeip.mydns.jp/sotajp/) |
| 参照URL | https://www.kawauchi.homeip.mydns.jp/sotajp/%e3%82%84%e3%81%a3%e3%81%a6%e3%81%bf%e3%82%88%e3%81%86/ （セクション「8. その場所は何ポイント?」） |
| 備考 | 標高バンドに基づくポイント数（1/2/4/6/8/10）の算出表を規定。全国統一（地域依存なし）。本プロジェクトでは `points` / `sota_points` の算出根拠として参照する。詳細な表は `docs/00_GLOSSARY.md` の「標高バンド（Points 算出表）」を参照。 |

---

### SOTA日本支部 FAQ

| 項目 | 内容 |
|---|---|
| タイトル | SOTA日本支部 よくある質問（FAQ） |
| 提供元 | [SOTA日本支部](https://www.kawauchi.homeip.mydns.jp/sotajp/) |
| URL | https://www.kawauchi.homeip.mydns.jp/sotajp/faqs/ |
| 備考 | アクティベーションゾーンの定義（Q12: 「山頂の最高地点で運用することは...このような時のために、山頂の高度から25m以内の高度差であれば山頂からの運用とみなします。この25mの高度差のエリアをアクティベーションゾーンとしています。」）等、SOTAルールの参照に使用。 |

---

### 国土数値情報 N03 行政区域

| 項目 | 内容 |
|---|---|
| タイトル | 国土数値情報 行政区域データ（N03-2026） |
| 提供元 | [国土交通省 国土数値情報ダウンロードサービス](https://nlftp.mlit.go.jp/) |
| URL | https://nlftp.mlit.go.jp/ksj/gml/datalist/KsjTmplt-N03-2026.html |
| ファイル形式 | GeoJSON（全国版、約 765MB） |
| 主要属性 | `N03_001`=都道府県名、`N03_002`=北海道振興局名 |
| 利用規約 | 国土数値情報利用規約（https://nlftp.mlit.go.jp/ksj/other/agreement.html） |
| 本プロジェクトでの利用形態 | merge.py での新規ピーク都道府県/振興局判定に使用。`scripts/preprocess_pref_boundaries.py` で都道府県/振興局単位に dissolve してから参照（`$DATA_DIR/ref/N03-2026_regions.geojson`）。ファイルサイズが大きいため git 管理外（.gitignore）。 |

---

### 日本の国土にかかる第1次地域区画

| 項目 | 内容 |
|---|---|
| タイトル | 日本の国土にかかる第1次地域区画（メッシュコード定義） |
| 提供元 | [政府統計の総合窓口（e-Stat）](https://www.e-stat.go.jp/) |
| 整備 | 総務省統計局 |
| 運用管理 | 独立行政法人統計センター |
| 参照URL | https://www.e-stat.go.jp/pdf/gis/primary_mesh_jouhou.pdf |
| 備考 | 解析に使用する1次メッシュコード（4桁）の地理的範囲の定義元。 |
