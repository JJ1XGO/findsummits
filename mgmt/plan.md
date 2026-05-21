# （現在進行中の plan なし）

直近の作業: ISSUE-020 delete判定ゾーンポリゴン導入

- Phase A（SRS / ADR / GLOSSARY 改修）: ✅ 完了（2026-05-21）
- Phase B（delete_zone_max_drop 実測確定 → 250m）: ✅ 完了（2026-05-21）
- ISSUE-020 close 済み。実装は以下に分離:
  - ISSUE-040: C++/OpenCV で FR-016 実装（AZ + delete判定ゾーン）
  - ISSUE-043: merge.py 改修（AZ/delete-zone PIP, 不備フラグ, exit code, dominant 改名）
  - ISSUE-044: output_geojson.py 改修 + merged_viewer.html 実装
  - ISSUE-045: params/config.ini.example に delete_zone_max_drop = 250 追加

次の plan を立てる際は本ファイルを丸ごと上書きする。
