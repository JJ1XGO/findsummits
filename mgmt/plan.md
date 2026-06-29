# 計画: ビューアモックアップの描画高速化（快速バンドル）

## Context

`docs/mockup/viewer_mockup.html` の等高線レイヤーと1次メッシュレイヤーの描画がパン/ズーム時にカクつく。
原因は2つに分かれる:

- **等高線**: `_fetchElev`（847行付近）がタイルごとに `<img>` + blobUrl + `onload` の往復、
  さらに毎回 `<canvas>` を新規生成して `getImageData` を実行。デコードがメインスレッドを占有する。
- **1次メッシュ**: `syncLabels`（540行付近）が `moveend` 毎にラベル（divIcon マーカー）を
  全消去→再生成する。ドラッグ中に何度も DOM マーカーの destroy/create が走り churn になる。

本計画は**観測挙動を変えない純粋な性能改善**（標高デコード結果・等高線形状・ラベル表示位置は不変）。
SRS 反映は不要、実装具体値としてモックアップが暫定保持する範囲（`docs/CLAUDE.md` モックアップ節準拠）。
Web Worker 化・メッシュ canvas 化は効果は大きいが規模が大きいため本バンドルには含めない（別ステップ）。

対象ファイル: `docs/mockup/viewer_mockup.html`（単一ファイル）

## タスク

### 1. 等高線デコードを createImageBitmap + scratch 再利用へ（Sonnet）

**`initialize`（660行付近）** に使い回しの scratch canvas を追加:

```js
initialize(options) {
  L.GridLayer.prototype.initialize.call(this, options);
  this._cache = new Map(); // url → Promise<Float32Array|null>
  const sc = document.createElement('canvas');
  sc.width = sc.height = 256;
  this._scratchCtx = sc.getContext('2d', { willReadFrequently: true });
},
```

**`_fetchElev`（847行付近）** を置換:

```js
async _fetchElev(url) {
  try {
    const res = await fetch(url);
    if (!res.ok) return null;
    const bmp = await createImageBitmap(await res.blob(), { colorSpaceConversion: 'none' });
    const ctx = this._scratchCtx;
    ctx.drawImage(bmp, 0, 0, 256, 256);
    bmp.close();
    return this._decode(ctx.getImageData(0, 0, 256, 256));
  } catch(e) { return null; }
},
```

- **`colorSpaceConversion: 'none'` は必須**: 標高 RGB のバイト値が変わると `_decode` の標高値が壊れる。
- **scratch 共有の安全性**: `await createImageBitmap` の後、`drawImage`→`getImageData`→`_decode` は
  await を挟まず同期実行されるため、複数タイルの並列呼び出しでもイベントループ上で interleave しない。
  単一 scratch ctx の共有は安全。
- `<img>`/blobUrl/`URL.createObjectURL`/`URL.revokeObjectURL` は不要になり削除。

### 2. メッシュラベル再生成をデバウンス（Sonnet）

**`syncLabels` 登録部（552行付近）** をデバウンス化:

```js
let labelTimer = null;
function scheduleLabels() {
  clearTimeout(labelTimer);
  labelTimer = setTimeout(syncLabels, 120);
}
map.on("zoomend moveend", scheduleLabels);
map.on("overlayadd", e => { if (e.name === "1次メッシュ") syncLabels(); }); // 初回表示は即時
```

- ドラッグ中に連発する `moveend` を1回の再生成に集約する（settle 後 120ms）。
- `overlayadd`（レイヤー ON）は即時のまま（表示の遅延を避ける）。
- `syncLabels` 本体・`buildLabels`・クランプロジックは変更しない。

## 検証

1. `make lint-html`（または `make lint`）警告ゼロを確認。
2. ブラウザで `docs/mockup/viewer_mockup.html` を開く:
   - **等高線**: レイヤー ON → 各ズーム（z=11/14/17/19）で等高線が変更前と同一形状で描画されること。
     パン/ズームが従来より滑らかなこと。標高ポップアップ値が妥当なこと（デコード不変の確認）。
   - **1次メッシュ**: レイヤー ON → z≥7 でラベル表示、ドラッグ中の再生成が settle 後にまとまること。
     画面端でのラベルクランプが従来通り効くこと。
   - 既存サミット/Keyコル/基準点のクリック・ポップアップに影響がないこと（無関係だが回帰確認）。
3. モックアップのため自動テストは無し（紙芝居プロトタイプ）。手動確認で代替。

## 完了後

- `docs/CLAUDE.md` の「ドキュメント更新時のルール」に従い、更新ターン内に Conventional Commits でコミット（push は指示まで不要）。
- 本計画ファイルを `mgmt/plan.md` へ `mv`（`/plan` がグローバル `.claude/plans/` に生成するため）。
