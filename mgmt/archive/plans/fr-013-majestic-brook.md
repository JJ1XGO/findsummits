# FR-013 spec-panel レビュー指摘の tracker 登録

## Context
`/spec-panel FR-013`（HTML ビューア生成）を4視点でレビューし、10件の指摘を抽出した。
ユーザーが「全件登録」を選択。うち指摘10（summitslist_date の (UTC) 表記）は、SOTA が
英国発祥で GMT(UTC) を暗黙のデフォルトとするとのユーザー判断で**取り下げ**。
本計画は残り9件を「残作業が文書・仕様の議論を伴うか」で issue（6件）と todo.md（3件）に
振り分けて登録する作業手順。コード・SRS 本体は変更しない（指摘の決着・反映は登録後に別途進める）。
impersonation 禁止に従い issue の reporter は `Opus`。

## 登録内容

### issue 登録（6件 / 仕様・設計議論を伴う）
すべて `--stage SRS --reporter Opus --status 未対応`。

1. [高/設計/GeoJSON出力] merged_summit.geojson のフィーチャ構成・プロパティスキーマ正本を
   消費者 FR-013 から生成者 FR-009（または 7.2 内部データ）へ移設。FR-013/FR-019 は参照化。
   （該当: SRS L959–1045 vs FR-009 L848・L917）
2. [中/改善/その他] FR-013↔FR-019 の役割境界整理。生成機構（テンプレート同梱・JS変数埋め込み・
   出力パス）の記述が FR-019(L1099–1100) にあり「生成」FR-013 と重複。
3. [中/改善/その他] viewer が使う metadata キー列挙が FR-013(L954–958) と FR-019(L1123–1126)
   で重複かつ不一致（6件 vs 3件）。一方へ集約。
4. [中/改善/GeoJSON出力] AZ `area_complete=false` は viewer 非到達（FR-019 L1108）なのに
   FR-013(L1016) が両値定義＝死に仕様。常に true の注記 or 中間データ専用と明確化。
5. [中/設計/GeoJSON出力] `key_col_resolved=false` ピークの `prominence` プロパティ値表現
   （空/センチネル/省略）が未規定（SRS L979・L988・L1030）。
6. [中/設計/GeoJSON出力] dominant ピークの勢力圏に削除候補サミットが複数入る場合、
   「ピーク→SOTAサミット」線（coord_diff）が複数本でき、全て同じ `summit_code`（ピーク仮コード）を持つ。
   どの線がどの削除候補に対応するかをプロパティで判別できず幾何のみで対応。既存サミット点側にも
   親 dominant ピークを示すプロパティがない（SRS L965・L1037–1045）。※実害は限定的（地図描画は幾何で成立）。
   - **推奨方向（非拘束メモ）**: 相手コードのプロパティ追加（既存仕様は対応づけを `summit_code` で明示する方針のため整合）。
     線に「相手サミットのSOTAコード」を新プロパティで追加、サミット点に「親 dominant ピーク仮コード」を追加する案。
   - **設計判断は本 issue 着手時に行う（今は決めない）**: 新プロパティ名・付与範囲・FR-009 生成ロジックおよび
     FR-019(相互ジャンプ)・FR-012/FR-021 への波及を確認のうえ決定し、必要なら ADR 化。登録時は status=未対応。

### todo.md 追記（3件 / 文言修正・実装/参照追従のみ）
`## 次回着手（優先順）` の該当優先度セクションへ追記。

7. (低) summit_name_jp/summit_name の取得元を「FR-009 が geojson_v{N} から取得し格納」へ修正（SRS L977・L1005）
8. (低) feature_type 用語ゆれ統一: 本文 `col`→`key_col`（SRS L992 vs FR-019 L1106）
9. (低) FR-013 概要に「作業用のみ生成・公開用は FR-020」を明記（SRS L938）

※ 指摘10（summitslist_date の (UTC) 表記）は取り下げ（SOTA は英国発祥で GMT(UTC) を暗黙デフォルトとする）。

## 実行手順
1. issue 6件を `venv/bin/python3 mgmt/tracker/track.py issue add` で登録
   （例: `... issue add --title "..." --type 設計 --priority 高 --stage SRS --category GeoJSON出力 --reporter Opus --description "..." --resolution "..."`）
2. `mgmt/todo.md` に 3件を該当優先度（低）セクションへ追記
3. `venv/bin/python3 mgmt/tracker/track.py issue export --if-changed` で xlsx 更新
4. ドキュメント整合性原則に従い、todo.md 追記分を含めコミット（Conventional Commits・本文日本語）

## 検証
- `venv/bin/python3 mgmt/tracker/track.py issue list --open` で6件が登録され ID 採番されていること
- `mgmt/todo.md` に3件が追記されていること
- `git status` がコミット後クリーンであること（xlsx 含む）

## 注意
- 本登録は「指摘の記録」まで。各指摘の決着（SRS 改訂・ADR 作成）は issue ごとに別途進める。
- 指摘1〜6 は相互に関連（特に 1 がスキーマ一元化で 2・3・4 の温床を解消）。着手時は 1 を起点に検討。
