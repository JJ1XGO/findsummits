# Plan: モックアップの等高線レイヤーを z=19 まで対応

## Context

`docs/mockup/viewer_mockup.html` の等高線レイヤーは、OSM ベース地図（最大 z=19）に切り替えて最大ズームすると等高線が消える。

調査の結果、根本原因は等高線レイヤーの `maxZoom: 18` 設定。Leaflet GridLayer はレイヤーの maxZoom を超える zoom ではタイルを一切リクエストしない。

加えて、デフォルト位置の富士山周辺は dem1a が無いため、z=17 でも dem5 から 4 倍拡大されたデータが返る。これにより z=18 でも等高線は粗いが（前回の修正で表示自体はされるはず）、致命的な「消失」は z=19 で起きていた。

ゴール: OSM ベース地図の最大 z=19 まで等高線を表示する。z=18 の既存ロジックを z=19 にも拡張する形でクリーンに対応する。

## Approach

z=18 専用に書いた `_loadExtendedFor18` / `_drawScaled` を、z=18・z=19 両対応の汎用版にリファクタする。z=19 では z=17 親タイル（2 レベル上）から 65×65 サブグリッドを切り出し、256×256 キャンバスに 4×4 マーク（線幅 4px）で描画する。

### 方針

- z=17 をネイティブソースとして固定（z=18 は 2 倍、z=19 は 4 倍スケール）
- ニアレストネイバー拡大した配列で隣接比較すると交差が検出されない問題は、ソースを z=17 ネイティブに固定する事で回避済み（z=18 の既存修正と同じ考え方）
- z=19 のマークは 4×4 px（z=18 の 2×2、z=17 の 1×1 と整合的にスケール）

### スケール対応表

| map zoom | coords.z | source | 切り出しサイズ | マーク | キャンバス |
|---|---|---|---|---|---|
| 17 | 17 | 257×257 (既存 `_loadExtended`) | 256×256 | 1×1 | 256×256 |
| 18 | 18 | 129×129 (z=17 親の 1/2 クアドラント) | 128×128 | 2×2 | 256×256 |
| 19 | 19 | 65×65 (z=17 親の 1/4 クアドラント) | 64×64 | 4×4 | 256×256 |

## Changes

ファイル: `docs/mockup/viewer_mockup.html`

### 1. options.maxZoom: 18 → 19

```js
options: {
  maxZoom: 19,   // 18 → 19 (OSM の最大ズームに合わせる)
  pane: 'contourPane',
  attribution: ''
},
```

### 2. `_render` を z>=18 で汎用化

```js
async _render(canvas, coords) {
  if (coords.z >= 18) {
    // z=18: scale=2, z=19: scale=4 (z=17 ネイティブをソースに固定)
    const scale = 1 << (coords.z - 17);
    const elev = await this._loadExtendedQuadrant(coords.x, coords.y, scale);
    if (elev) this._drawScaled(canvas, elev, map.getZoom(), scale);
  } else {
    const elev = await this._loadExtended(coords.z, coords.x, coords.y);
    if (elev) this._draw(canvas, elev, map.getZoom());
  }
},
```

### 3. `_loadExtendedFor18` → `_loadExtendedQuadrant(x, y, scale)`

```js
// z=18/19 タイル専用: z=17 親タイルの 257×257 拡張グリッドから
// 対象サブクアドラント(scale 比に応じた領域)を切り出して返す
async _loadExtendedQuadrant(xZ, yZ, scale) {
  const levels = scale === 2 ? 1 : 2;   // z=18→1段, z=19→2段
  const x17 = xZ >> levels, y17 = yZ >> levels;
  const qx  = xZ & (scale - 1), qy = yZ & (scale - 1);
  const ext = await this._loadExtended(17, x17, y17);
  if (!ext) return null;
  const W = 257;
  const subN = 256 / scale;       // 128 or 64
  const subW = subN + 1;          // 129 or 65 (右下に 1px バッファ)
  const sub = new Float32Array(subW * subW);
  const r0 = qy * subN, c0 = qx * subN;
  for (let r = 0; r <= subN; r++)
    for (let c = 0; c <= subN; c++)
      sub[r * subW + c] = ext[(r0 + r) * W + (c0 + c)];
  return sub;
},
```

### 4. `_drawScaled` を scale パラメータ対応に

```js
// z=17 ネイティブ (subN+1)×(subN+1) サブグリッド → 256×256 キャンバスへ scale×scale マークで描画
_drawScaled(canvas, elev, mapZ, scale) {
  const subN = 256 / scale;     // 128 or 64
  const W = subN + 1;
  const { major, minor } = getContourIntervals(mapZ);
  const ctx = canvas.getContext('2d');
  const imd = ctx.createImageData(256, 256);
  const d = imd.data;

  for (let py = 0; py < subN; py++) {
    for (let px = 0; px < subN; px++) {
      const i = py * W + px;
      const e = elev[i];
      if (e === -9999) continue;
      const er = elev[i + 1];
      const eb = elev[i + W];
      let isMajor = false, isMinor = false;

      for (const n of [er, eb]) {
        if (n === -9999) continue;
        const lo = Math.min(e, n), hi = Math.max(e, n);
        if (Math.floor(hi / major) !== Math.floor(lo / major)) { isMajor = true; break; }
        if (Math.floor(hi / minor) !== Math.floor(lo / minor))   isMinor = true;
      }

      if (!isMajor && !isMinor) continue;
      for (let dy = 0; dy < scale; dy++) {
        for (let dx = 0; dx < scale; dx++) {
          const j = ((py * scale + dy) * 256 + (px * scale + dx)) * 4;
          if (isMajor) { d[j]=160; d[j+1]=64;  d[j+2]=32;  d[j+3]=75; }
          else         { d[j]=200; d[j+1]=120; d[j+2]=80;  d[j+3]=40; }
        }
      }
    }
  }
  ctx.putImageData(imd, 0, 0);
},
```

## Files

- `docs/mockup/viewer_mockup.html` (4 箇所の編集)

## 補足: 富士山周辺の dem5 フォールバック

dem1a 非対応地域では、z=17 自体が dem5 (z=15) を 4×4 px ブロックに拡大した結果になる。そのため z=18 では実質 32×32、z=19 では実質 16×16 の固有値しかない。等高線は表示されるが、富士山の緩斜面では「点線」のように見える事がある。これは元データの解像度限界で、本修正の範囲外。

## Verification

1. ブラウザで `docs/mockup/viewer_mockup.html` を開く
2. ベース地図を OSM に切り替え
3. 富士山周辺で z=18, z=19 までズームイン → 等高線が（粗いが）描画される事を確認
4. ベース地図を地理院（標準）に戻し z=18 まで → 等高線が前回修正どおり 2×2 px 幅で表示される事を確認
5. 関東平野（dem1a あり）で z=19 までズーム → 細かい等高線がはっきり表示される事を確認
6. z=14〜17 の動作が変わっていない事を確認（既存の `_draw` パスは無変更）
