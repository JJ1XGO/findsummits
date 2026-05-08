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
| 本プロジェクトでの利用形態 | タイルをローカルにキャッシュして標高解析に使用。タイルデータ自体はリポジトリに含めない（.gitignore）。生成する HTML ビューアの帰属表示に `© 国土地理院` を含める。 |

---

## 参照文書（ファイル未格納）

### 日本の国土にかかる第1次地域区画

| 項目 | 内容 |
|---|---|
| タイトル | 日本の国土にかかる第1次地域区画（メッシュコード定義） |
| 提供元 | [政府統計の総合窓口（e-Stat）](https://www.e-stat.go.jp/) |
| 整備 | 総務省統計局 |
| 運用管理 | 独立行政法人統計センター |
| 参照URL | https://www.e-stat.go.jp/pdf/gis/primary_mesh_jouhou.pdf |
| 備考 | 解析に使用する1次メッシュコード（4桁）の地理的範囲の定義元。 |
