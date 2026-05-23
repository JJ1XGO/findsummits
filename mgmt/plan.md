# Plan: ISSUE-043/044 notes 補正（入力2系統の明記）

## Context

2026-05-21 セッションで「merged.csv に全情報を集約し、不備がなければ後続で merged.geojson と merged_viewer.html を生成する」というフロー合意があった（handover 2026-05-21_1830.md 記載）。

しかし AZ・delete判定ゾーンのポリゴン情報は CSV に載らないため、output_geojson.py の入力は実際には「merged.csv + merged_activation.geojson」の2系統である必要がある。

調査結果:
- **SRS (02_SRS.md:440-444 FR-013)**: 既に「入力: merged.csv + merged_activation.geojson」と正しく規定済み → 補正不要
- **ISSUE-043 notes**: 列計算（points/sota_points 等）のみ記載。merge.py が `merged.csv` と `merged_activation.geojson` の2系統を出力する点が未記載
- **ISSUE-044 notes**: UI 廃止・自動エクスポートのみ記載。入力2系統が未記載

ISSUE notes と SRS は矛盾していないが、一昨日の口約束との差分が明示されていないため、後続セッションが handover を見て同じ誤解を踏みやすい。本 Plan はこの差分を ISSUE notes に追記して整合させる。

## 補正方針

### ISSUE-043 notes に追記

`venv/bin/python3 mgmt/tracker/track.py issue update 043 --notes "..."` で末尾追記。

追記文面（要旨）:
- merge.py の出力は `merged.csv`（ピーク台帳・ステータス・不備フラグ）と `merged_activation.geojson`（AZ・delete判定ゾーンポリゴンの per-mesh 統合）の2系統である（SRS FR-018 参照）
- AZ/delete-zone PIP 判定の素材は per-mesh activation GeoJSON
- 不備フラグ・exit code 判定は merged.csv 側で完結する

### ISSUE-044 notes に追記

`venv/bin/python3 mgmt/tracker/track.py issue update 044 --notes "..."` で末尾追記。

追記文面（要旨）:
- output_geojson.py の入力は `merged.csv` + `merged_activation.geojson` の2系統（SRS FR-013 参照）
- ポリゴン情報（AZ・delete判定ゾーン）は merged_activation.geojson から取り込む
- 出力は `merged.geojson`（Point + LineString + Polygon の可視化集約）と `merged_viewer.html`

### SRS（02_SRS.md）

補正不要。FR-013/FR-018 の記述は既に正確。

## 不変箇所（合意のまま残る部分）

- merge.py が merged.csv を生成し、不備があれば exit code で後続を止める（ISSUE-043 の根幹）
- output_geojson.py が後続で merged.geojson と merged_viewer.html を生成する（ISSUE-044 の根幹）
- merged.csv は「1レコード=1ピーク」のフラットなステータス原簿として有効

## 触れないこと

- merge.py / output_geojson.py のコード（仕様優先原則、SRS フェーズ中はコード変更しない）
- SRS / ADR / GLOSSARY 本体（既に正しい）
- handover 過去ログ（事実記録として残す）

## 検証手順

1. `venv/bin/python3 mgmt/tracker/track.py issue show 043` で notes に2系統出力が明記されたことを確認
2. `venv/bin/python3 mgmt/tracker/track.py issue show 044` で notes に2系統入力が明記されたことを確認
3. SRS FR-013（02_SRS.md:440-444）と ISSUE-044 notes が同じ表現で整合していることを目視確認

## 後続作業

- 補正コミット後、ISSUE-043/044 を SRS フェーズ完了時点で改めて実装着手対象として扱う（SRS 確定後）
