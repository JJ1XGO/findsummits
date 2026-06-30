# geojson_v{N}・summitslist の「ユーザー入力」再分類と `$DATA_DIR/ref/` 移設

## Context

`SOTA 既存サミット GeoJSON（geojson_v{N}）` と `SOTA サミットリスト CSV（summitslist.csv）` は、現状 SRS §6（外部I/F）に分類されている。しかし両者ともシステムが自動取得せず、ユーザーが手動で `ref/` 配下に配置・差し替えする入力ファイルである。これは ADR-SRS-016 の前例（N03 行政区域 ZIP を「ユーザーが手動配置するファイルはツールへの入力」として §7 ユーザー入力へ分類）と同性質であり、外部I/F に置くのは不整合。

ユーザー合意により以下を確定:

1. **再分類**: geojson_v{N}・summitslist.csv の両方を §6 外部I/F → §7 ユーザー入力 へ移す。
2. **配置統一**: 保存先を N03 と揃えて `$DATA_DIR/ref/` 直下に統一し、git 管理外とする。`ref/summitslist.csv` → `$DATA_DIR/ref/summitslist.csv`、`ref/geojson_v{N}/` → `$DATA_DIR/ref/geojson_v{N}/`。

この (2) は過去 lessons（「DATA_DIR 配下にソースコード付属データを置くのは誤り」）と逆向きだが、当時の前提「ソースコード付属データ＝git 管理すべき小データ」を「ユーザー入力＝ユーザーが手動で用意するもの」へ変更する方針転換に伴う正当な更新（lessons も更新する）。

### スコープ方針（重要）

- **生きた仕様（SRS・GLOSSARY・`docs/CLAUDE.md`・`ref/SOURCES.md`）はパス・分類を更新する。**
- **ADR は時点記録のため、過去の `ref/summitslist.csv` 等の記述を一括書き換えしない。** 新しい分類・配置の正規記録は ADR-SRS-016 の改訂注記に集約する。
- **コード（C/Python）・`fetch_config.ini`・`params/` のパス追従は本計画の対象外**（仕様確定後に `mgmt/todo.md` へ登録）。

## タスク

全タスク機械的なドキュメント整合修正のため既定 **Sonnet**。

### T1: SRS §7 へ geojson・summitslist を移設（モデル: Sonnet）

- `docs/20_SRS.md` §6.2.1（SOTA 既存サミット GeoJSON）と §6.2.11（SOTA サミットリスト CSV）の詳細小節を §7.2 末尾へ移動（churn 最小化のため末尾追加）:
  - 新 §7.2.4 SOTA サミットリスト CSV
  - 新 §7.2.5 SOTA 既存サミット GeoJSON（geojson_v{N}）
- 移動した小節内のファイルパスを `$DATA_DIR/ref/summitslist.csv` / `$DATA_DIR/ref/geojson_v{N}/...` に更新し、「git 管理外・ユーザー手動配置」を明記（省略時の動作等の既存仕様記述は維持する）
- **§7.1 ユーザー入力一覧を §7.2 小節登場順に並び替え、No. を振り直した上で2行追加**:

  現在の §7.1（No. 順が §7.2 小節順と不一致）:
  | No. | 名前 | §7.2 小節 |
  |---|---|---|
  | 1 | N03 行政区域 ZIP | §7.2.3 |
  | 2 | 1次メッシュコードリスト | §7.2.1 |
  | 3 | HTML ビューア上のユーザー入力 | §7.2.2 |

  変更後（§7.2 登場順に並び替え＋2行追加）:
  | No. | 名前 | §7.2 小節 |
  |---|---|---|
  | 1 | 1次メッシュコードリスト | §7.2.1 |
  | 2 | HTML ビューア上のユーザー入力 | §7.2.2 |
  | 3 | N03 行政区域 ZIP | §7.2.3 |
  | 4 | SOTA サミットリスト CSV | §7.2.4（新設）|
  | 5 | SOTA 既存サミット GeoJSON（geojson_v{N}）| §7.2.5（新設）|

  ※ 既存3行（No.1〜3）の「補足」列アンカーは §7.2.x 番号が変わらないため更新不要
  ※ 新規追加行（No.4 summitslist / No.5 geojson）の「補足」列アンカーは最初から `[7.2.4 参照](#724-...)` / `[7.2.5 参照](#725-...)` として設定する（T3 での別途張り替え不要）
- 既存 §7.2.1〜7.2.3（メッシュ・HTMLビューア・N03）の小節番号・アンカーは変更しない

### T2: SRS §6 から2件を除去し §6.2.x を採番繰り上げ（モデル: Sonnet）

- §6.1 外部I/F一覧表から No.1（geojson）・No.11（summitslist）の行を削除し、残り行の No. を 1〜11 に振り直す
  - 削除後の §6.1 順は §6.2 小節登場順と一致するため並び替え不要
- §6.2 詳細小節を 6.2.2〜6.2.10 → 6.2.1〜6.2.9 に繰り上げ（見出し番号）
- §6 内のアンカー参照を全張り替え:
  - `#622-…`→`#621-…`、`#623`→`#622`、`#624`→`#623`、`#625`→`#624`、`#626`→`#625`、`#627`→`#626`、`#628`→`#627`、`#629`→`#628`、`#6210-地理院標高タイル`→`#629-地理院標高タイル`
- 完了後 `grep -oE "#62[0-9]+-" docs/20_SRS.md` で旧番号アンカー（#6210・#6211・#621-sota 等）が残存しないことを確認

### T3: SRS 本文の参照・分類・パスの追従修正（モデル: Sonnet）

- §6.2.1/§6.2.11 へのリンク（geojson・summitslist）を新 §7.2.5/§7.2.4 アンカー（`#725-…`・`#724-…`）へ張り替え（目次・本文 l.125・l.865・l.876・FR-009 本文リンクを含む）
  - ※ l.1489（§6.1 No.1）・l.1499（§6.1 No.11）は T2 で削除される行なので対象外
- FR-009 入出力表（l.862・l.865）の種別を `外部I/F` → `ユーザー入力` に変更
- `grep -n "外部I/F" docs/20_SRS.md` で geojson/summitslist に紐づく `外部I/F` ラベルが残る箇所を洗い出し修正
- §10 制約・前提（l.1805-1806）: summitslist/geojson の「git 管理（`ref/` 配下）」記述を「git 管理外・`$DATA_DIR/ref/` 直下」に修正
- §10 外部システム依存表の SOTA データベース行: 「`ref/summitslist.csv` として配置」→「`$DATA_DIR/ref/summitslist.csv`」、参照リンクを新 §7.2 アンカーへ、誤記「（詳細: §10）」→「（詳細: §7.2）」に修正
- 目次（TOC）の §6.2.x / §7.2.x 項目を実際の見出しと一致させる

### T4: ADR-SRS-016 へ改訂注記を追加（モデル: Sonnet）

- `docs/decisions/ADR-SRS-016-data-classification-external-user-internal.md` に改訂注記を追記:
  - geojson_v{N}・summitslist.csv を「ユーザーが手動配置する入力」として外部I/F → ユーザー入力（§7）へ再分類（N03 前例と同基準）
  - 配置を `$DATA_DIR/ref/` 直下・git 管理外へ統一
  - 結果、§6 外部I/F は「実行時取得リモート資源＋出力成果物」に純化された旨を明記

### T5: GLOSSARY・docs/CLAUDE.md の分類例を整合（モデル: Sonnet）

- `docs/00_GLOSSARY.md`:
  - l.124 外部I/F 定義の例から「SOTA データベース」を外す
  - l.104 geojson_v{N} 用語・l.20/l.21 等の `ref/...` パスを `$DATA_DIR/ref/...` に追従
- `docs/CLAUDE.md` l.82 分類ルール表の外部I/F 例から「SOTA データベース」を外す

### T6: `ref/SOURCES.md` のパス追従・SRS から参照先明記（モデル: Sonnet）

- summitslist.csv / geojson_v{N} の配置パス記述（`ref/` 起点）を `$DATA_DIR/ref/` に追従
- **geojson_v{N} の取得元は `ref/SOURCES.md` l.37 に記載済み**（`little-ctc.com`）。新 §7.2.5 の詳細仕様内に取得元として `（出典・利用規約: ref/SOURCES.md）` 等の参照を追記する
  - ※ summitslist（§7.2.4）は `取得元` 列に URL を直接記載する形式で `ref/SOURCES.md` 参照なし。geojson はそれとは別に `ref/SOURCES.md` 参照を新規追記する

### T7: lessons.md 更新（モデル: Sonnet）

- `mgmt/lessons.md` の「パスはプロジェクトディレクトリからの相対で」項に方針変更を追記:
  - geojson_v{N}・summitslist.csv は「ユーザー入力」に再分類したため `$DATA_DIR/ref/` 配下・git 管理外へ移した（N03 等のユーザー入力ファイルには当該レッスンを適用しない）

## 検証

- `make lint` 警告ゼロ（特に BROKEN-LINK・見出し整合）
- `grep -oE "#62[0-9]+-[^)]*" docs/20_SRS.md | sort -u` で §6.2 アンカーが新採番（#621〜#629）のみ、geojson/summitslist の旧アンカーが残っていないこと
- `grep -rn "ref/summitslist.csv\|ref/geojson_v{N}" docs/20_SRS.md docs/00_GLOSSARY.md docs/CLAUDE.md ref/SOURCES.md` で生きた仕様側に旧パス（`$DATA_DIR` なし）が残っていないこと
- §6 外部I/F一覧が出力成果物＋実行時取得リモート資源のみで構成されていること
- §7 ユーザー入力一覧に geojson・summitslist・N03 の3手動配置ファイルが揃い、§7.2 小節登場順に並んでいること

## コミット方針

SRS＋ADR＋GLOSSARY＋CLAUDE＋lessons を整合した1〜2コミットにまとめる（Conventional Commits、`docs(srs): …` 等）。push は別途指示まで行わない。

## 実装後の follow-up（本計画の対象外・todo.md へ）

- コード（C/Python）・`fetch_config.ini`・`params/` の summitslist/geojson 読み込みパスを `$DATA_DIR/ref/` へ追従
- `.gitignore` から `ref/summitslist.csv`・`ref/geojson_v{N}/` の git 追跡を外す対応（git 管理外化）
