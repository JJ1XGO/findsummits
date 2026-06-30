---
description: lessons.md の蓄積内容を取り込み best_practices.md を再合成する
model: opus
---

# Best Practices 更新

`lessons.md` の全項目を再分析し、`best_practices.md` を最新状態に合成し直す。

## 手順

1. `/workspace/mgmt/lessons.md` を全件読む
2. `/workspace/mgmt/best_practices.md` を読み、現在の原則構成を把握する
3. lessons.md の全項目を以下の5観点で横断分析し、14〜18原則に再蒸留する：
   - 手戻り防止
   - 判断コスト削減
   - 信頼性の担保
   - コンテキスト継続
   - 仕様と実装の整合
4. 各原則を以下フォーマットで記述する（既存原則を踏襲しつつ新内容を反映する）：

   ```markdown
   ## N. 【原則名（5〜10文字）】

   **何をするか**: （1〜2行の処方形）

   **なぜ有効か**: （「〜するようにしたら〜が減った/できるようになった」の形で1〜3行）

   **よくある失敗パターン**: （1行）
   ```

5. `best_practices.md` を以下ヘッダー付きで上書き保存する：

   ```markdown
   # Best Practices

   > `mgmt/lessons.md` の N 項目を蒸留した高レベル原則集。
   > 詳細な教訓・事例は `mgmt/lessons.md` を参照。
   ```

6. `/workspace/mgmt/lessons.md` の `^-` で始まる行数を数え、
   `/workspace/.claude/best_practices_watermark` に数値のみ書き込む
7. `make lint` を実行し警告ゼロを確認する
8. コミットする（対象: `mgmt/best_practices.md` と `.claude/best_practices_watermark`）

## 注意事項

- 除外：プロジェクト固有の技術詳細（dem10b 解像度・openpyxl API 等）
- 既存原則が新内容をカバーしていれば統合・更新でよい（原則を増やしすぎない）
- 原則数は14〜18件程度に抑える（多すぎると読まれなくなる）
- Sonnet への切り替えは不要（synthesis タスクは Opus のまま継続）
