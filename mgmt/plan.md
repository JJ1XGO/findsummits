# ミーティング議事録・決定事項

## Context

SRS作成は別セッションで実施予定。今回はその前提となる管理体制の整備を議論した。

---

## 決定事項 1: mgmt/ の置き場所とブランチ運用

### 方針
- `mgmt/` をプロジェクトルート（`/workspace/mgmt/`）に移動（`.claude/mgmt/` から）
- `devel` ブランチで通常通りgit管理（証跡・履歴を残す）
- `.claude/handovers/` は Claude Code 連携があるため `.claude/` に残す

### リリース時の運用
```
devel → release/vX.X（mgmt/ を git rm）→ main
```

```bash
git checkout devel
git checkout -b release/vX.X
git rm -r mgmt/
git commit -m "chore: リリース用にmgmt/除外"
git checkout main
git merge release/vX.X
git branch -d release/vX.X
```

**ルール**: `devel → main` の直接マージ禁止。必ず `release/*` ブランチを介す。

---

## 決定事項 2: グローバル CLAUDE.md の修正

### 修正内容（3点）

1. **パス修正**
   - `tasks/lessons.md` → `mgmt/lessons.md`
   - `tasks/todo.md` → 削除（tracker に一本化）

2. **セッション開始ルーティンのパス修正**
   - `tasks/lessons.md` → `mgmt/lessons.md`

3. **コア原則「5. バグ修正の対応」を削除**
   - 「できる限り自律的に修正する」がプロジェクトのtracker運用ルールと矛盾するため
   - プロジェクト側のルール（発見→一覧提示→確認→登録→承認→着手）を優先

### タスク管理の流れ（修正後）
```
1. 開発課題は track.py issue add で登録
2. 実装前に計画を確認・承認
3. 進捗は track.py issue update で更新
4. 完了時は track.py issue close
5. 修正があったら mgmt/lessons.md を更新
```

---

## 実施が必要な作業（このミーティング終了後）

- [ ] `.claude/mgmt/` → `mgmt/` への移動（tracker/, lessons.md, plan.md）
- [ ] グローバル CLAUDE.md の修正（上記3点）
- [ ] プロジェクト CLAUDE.md のパス参照を更新（mgmt/ に合わせる）
- [ ] `.gitignore` の確認・更新
- [ ] ISSUE として登録

---

## 決定事項 3: 用語集の新設

- `docs/00_GLOSSARY.md` を新規作成する
- 各ドキュメント（URD/SRS等）の冒頭に「用語定義は 00_GLOSSARY.md を参照」と記載
- URD内の「メッシュ」という表記はそのまま維持（変更不要）
- 最初に登録する用語: 「メッシュ」= 第一次地域メッシュ（第一次地域区画）

---

## SRS検討事項（メモ）

### パイプラインの分割粒度
現状の `run_all.sh` は一気通貫だが、実運用では以下の3フェーズに分けることを検討：
1. 標高タイルのダウンロード（最新化）
2. 3×3メッシュ解析
3. 申請書等の作成（XLSX/CSV/GeoJSON）

**背景**: 各フェーズの境界でログや結果の目視確認が入るため。
**ただし**: 精度が十分高ければ一気通貫でも問題ない可能性あり。SRS策定時に要件として明記するか判断する。

---

## 未確認事項

なし

---

## handover 記載事項

グローバルCLAUDE.mdを修正した際は、他プロジェクトを開く最初のセッションでユーザーが「グローバルCLAUDE.mdを修正したので影響を確認して」と一言添える運用にする。handoverにその旨を必ず記載すること。
