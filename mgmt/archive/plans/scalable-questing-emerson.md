# 計画: ADR 命名ルールの拡張（全ステージ対応・ステージ別独立連番）

## Context

前回のセッション（2026-05-31_1646）で ADR 命名体系を `ADR-NNN` → `ADR-{STAGE}-NNN` に刷新したが、`{STAGE}` を **URD / SRS の 2 種類のみ** に限定し、連番は **ステージ種別を跨いだ全体通し番号** とした。

ユーザーから方針変更の指示:

1. **全開発ステージのコードを使えるようにする** — URD / SRS / HLD / LLD / COD / UT / IT / ST / OPS の 9 種類すべてを `{STAGE}` に許可
2. **連番はステージごとに独立** — 全体通し番号をやめ、各ステージ内で連番を採る
3. **既存 ADR の番号は維持** — リネームしない
   - ADR-URD の現状最大値 = 014 → 次の URD は **ADR-URD-015**
   - ADR-SRS の現状最大値 = 013 → 次の SRS は **ADR-SRS-014**
   - 新ステージ（HLD/LLD/COD/UT/IT/ST/OPS）の最初の ADR は **001** から採番
   - 例: 初の HLD 判断 → `ADR-HLD-001-...`

### 既存番号の「穴」について

既存 14 個の ADR は前回の刷新で全体通し番号のまま `ADR-URD-` / `ADR-SRS-` に振り分けたため、ステージ別に見ると番号が飛んでいる:

- URD: 005, 007, 009, 014（001〜004, 006, 008, 010〜013 は欠番）
- SRS: 001, 002, 003, 004, 006, 008, 010, 011, 012, 013（005, 007, 009, 014 は欠番）

ユーザー指示「現在 ADR-URD は 014 が最大値、ADR-SRS は 013 が最大値なので、次は ADR-URD-015 / ADR-SRS-014」より、**欠番はそのまま放置** し新規 ADR は各ステージの最大値 + 1 から採番する方針で確定（一過性の歴史的経緯として許容）。

---

## 変更内容

### 1. `docs/00_GLOSSARY.md` の「ADR 命名規約」サブセクション更新

**該当箇所**: lines 169-177（`#### ADR 命名規約`）

**変更後の内容**:

```markdown
#### ADR 命名規約

`ADR-{STAGE}-NNN-kebab-case-description.md`

- `{STAGE}`: 判断が発生したステージ。**URD / SRS / HLD / LLD / COD / UT / IT / ST / OPS** の 9 種類から選ぶ
  - 当該ステージの正式文書が未作成でも、判断種別として該当すれば使用してよい
  - 判定ルール: 判断の中身が最も自然に属するステージを選ぶ（上流側で決められるなら上流を優先）
- `NNN`: 3 桁連番。**ステージごとに独立した連番**（各ステージ内で 001 から採番）
  - 既存 ADR（URD: 005/007/009/014, SRS: 001/002/003/004/006/008/010/011/012/013）は前回刷新時の経緯で全体通し番号を維持しているため番号に欠番がある
  - 新規 ADR は各ステージの現状最大値 + 1 から採番する
    - 次の URD: `ADR-URD-015-...`
    - 次の SRS: `ADR-SRS-014-...`
    - 初の HLD: `ADR-HLD-001-...`（以降のステージも同様に 001 から）

例: `ADR-URD-005-northern-territories-exclusion.md`、`ADR-SRS-001-hybrid-c-python-architecture.md`、（将来）`ADR-HLD-001-...`
```

### 2. `/workspace/CLAUDE.md` の「ADR 管理ルール」命名規則更新

**該当箇所**: lines 230-235

**変更後の内容**:

```markdown
**命名規則**: `docs/decisions/ADR-{STAGE}-NNN-kebab-case-description.md`

- `{STAGE}` は `URD` / `SRS` / `HLD` / `LLD` / `COD` / `UT` / `IT` / `ST` / `OPS` の 9 種類
- 判断の中身が最も自然に属するステージを選ぶ（上流側で決められるなら上流を優先）
- `NNN` は 3 桁連番で **ステージごとに独立**
- 既存 ADR の番号は維持。新規は各ステージの現状最大値 + 1 から採番（次の URD = 015、次の SRS = 014、初の HLD = 001）
- 詳細は `docs/00_GLOSSARY.md` の「ADR 命名規約」を参照
```

### 3. ファイルリネーム・参照更新は不要

- 既存 14 個の ADR ファイル名はそのまま維持
- 既存 ADR を参照しているコード・ドキュメント・トラッカーも変更なし

---

## 実行順序

1. `docs/00_GLOSSARY.md` の ADR 命名規約サブセクションを Edit
2. `/workspace/CLAUDE.md` の ADR 管理ルール命名規則を Edit
3. `git status` で対象ファイルを確認
4. **個別ファイル指定**で `git add` してコミット
   - メッセージ案: `refactor(docs): ADR命名ルールを全ステージ対応・ステージ別独立連番に変更`

---

## 検証

1. `docs/00_GLOSSARY.md` を Read し、`#### ADR 命名規約` セクションが新ルールどおり書かれていること
2. `/workspace/CLAUDE.md` を Read し、ADR 管理ルールの命名規則が新ルールどおりであること
3. `grep -rEn "URD / SRS の 2 種類|URD または SRS" /workspace --include="*.md"` で、現行 docs・CLAUDE.md に旧表現が残っていないこと（`mgmt/plan.md`・handover には残ってよい — 履歴記録のため）

---

## スコープ外（本計画では実施しない）

- 既存 ADR のリネーム・番号振り直し（ユーザー指示により維持）
- 既存 ADR の HLD/LLD 等への再分類（HLD/LLD 文書を作成するタイミングで必要なら別途検討）
- `mgmt/plan.md`・`.claude/handovers/*` 内の旧表現の遡及修正（履歴ファイルとしてそのまま残す）

---

## 関連ファイル

- `/workspace/docs/00_GLOSSARY.md`（編集対象）
- `/workspace/CLAUDE.md`（編集対象）
- `/workspace/mgmt/plan.md`（plan モード終了後にこのファイルをコピー）
