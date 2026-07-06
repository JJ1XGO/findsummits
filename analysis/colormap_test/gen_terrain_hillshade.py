"""
terrain および gist_earth に LightSource.shade() で陰影起伏を合成したサンプルを生成する。
既存の C 側キャッシュ (5338_cmp_bigtile.cache: uint32 width, uint32 height, float32 data...) を
numpy.memmap でそのまま読み込み、間引いてダウンサンプルしてから処理する（16GB全体は読まない）。
本番パイプライン外の一時調査用スクリプト。

gist_earth は深海〜高山という地球規模の標高レンジを想定した配色で、単純な Normalize 調整
（vmin/vmax の t0 スキャン）だけでは「海を青くすると陸地の低地も同化し、陸地を早く緑にすると
海も緑寄りになる」というトレードオフが解消できないと判明した（terrain を優位と判断した時期あり）。
しかし標高 0m 以下を固定色（マスク）で塗りつぶす方式により、gist_earth のカラーマップ・陰影を
海面から完全に切り離せることが分かり、方針転換した。

採用決定（2026-07-06、ADR-SRS-047）: `5338_hillshade_gist_earth_ve8_alt77.7_t0-0.25_mask-white.png`
（gist_earth + LightSource(azdeg=180, altdeg=77.7) + vert_exag=8 + t0=0.25（vmaxは解析範囲の
実測最大標高を使う相対値）+ 標高0m以下を白 `#FFFFFF` でマスク）。生成スクリプトは
`gen_terrain_gistearth_ocean_variants.py`。SRS（`docs/20_SRS.md` 6.2.3節）への反映は
`ADR-SRS-047-terrain-color-scheme-gist-earth-hillshade.md` を参照。
"""
import matplotlib.cm as cm
import matplotlib.pyplot as plt
import numpy as np
from matplotlib.colors import LightSource, to_rgba

CACHE_PATH = "/data/images/5338_cmp_bigtile.cache"
OUT_DIR = "/workspace/analysis/colormap_test"
STRIDE = 12  # 70146x57602 -> 約5845x4800

# ズーム15・5338メッシュ中央緯度(35.6667度)でのWebメルカトル解像度(m/px) x STRIDE。
# mesh.c の latlon_to_tile と同じ球体近似(地球全周40075016.686m・タイル256px)で算出。
# dx/dy をこの実距離に合わせないと、np.gradient が標高差(m)を1px=1mとして計算してしまい
# 勾配が実際より遥かに急峻になる（法線がほぼ水平になり尾根だけ発光する二極化の原因）。
PIXEL_RESOLUTION_M = 3.8812 * STRIDE

with open(CACHE_PATH, "rb") as f:
    width = np.fromfile(f, dtype=np.uint32, count=1)[0]
    height = np.fromfile(f, dtype=np.uint32, count=1)[0]
    header_bytes = 8

print(f"元サイズ: {width}x{height}")

data = np.memmap(CACHE_PATH, dtype=np.float32, mode="r",
                  offset=header_bytes, shape=(height, width))
elevs = np.array(data[::STRIDE, ::STRIDE])  # 間引いてメモリへコピー
print(f"間引き後サイズ: {elevs.shape[1]}x{elevs.shape[0]}")

ls = LightSource(azdeg=180, altdeg=90)

# vert_exag=8: ve3/ve8/ve15 の目視比較で最も自然な陰影バランスだった値
VERT_EXAG = 8

# vmin/vmax調整なし(データの実min/maxをそのまま使う既定のNormalize)。
# 最初に「見やすい」と評価されたのはこの状態。
rgb_shaded = ls.shade(elevs, cm.terrain, dx=PIXEL_RESOLUTION_M, dy=PIXEL_RESOLUTION_M,
                       vert_exag=VERT_EXAG)

fig, ax = plt.subplots()
fig.set_size_inches(16.53 * 2, 11.69 * 2)
ax.imshow(rgb_shaded)
ax.set_xticks([])
ax.set_yticks([])
out_path = f"{OUT_DIR}/5338_hillshade_terrain_ve{VERT_EXAG}.png"
plt.savefig(out_path, bbox_inches="tight", dpi=150)
plt.close(fig)
print(f"出力完了: {out_path}")

# 夏至の太陽南中高度(理論最大値、5338メッシュ中央緯度35.6667度で計算): 約77.7度。
# altdeg=90(天頂)は日本の緯度では物理的に有り得ない光源位置だったため、
# 現実的な光源高度に変更して比較する(azdeg=180=南中時はそのまま維持)。
ls_summer = LightSource(azdeg=180, altdeg=77.7)

# 比較用: gist_earth で標高0mの位置(t0)を0-0.4まで0.05刻みで振った版。
# vmin = -t0*vmax/(1-t0)（マスクなし、素の Normalize 調整のみ）
vmax = elevs.max()
for t0_int in range(0, 41, 5):
    t0 = t0_int / 100
    gist_earth_vmin = -t0 * vmax / (1 - t0)
    rgb_ge = ls_summer.shade(elevs, cm.gist_earth, dx=PIXEL_RESOLUTION_M, dy=PIXEL_RESOLUTION_M,
                              vert_exag=VERT_EXAG, vmin=gist_earth_vmin)
    fig, ax = plt.subplots()
    fig.set_size_inches(16.53 * 2, 11.69 * 2)
    ax.imshow(rgb_ge)
    ax.set_xticks([])
    ax.set_yticks([])
    out_path = f"{OUT_DIR}/5338_hillshade_gist_earth_ve{VERT_EXAG}_alt77.7_t0-{t0:.2f}.png"
    plt.savefig(out_path, bbox_inches="tight", dpi=150)
    plt.close(fig)
    print(f"出力完了: {out_path}")

# t0=0.00 は最も詳細だが 0m と 0m超の境界が不明瞭なため、0m以下を固定色で塗りつぶす
# マスク版も生成する（陰影・カラーマップの対象から外し、海を陸地と明確に区別する）。
ocean_mask = elevs <= 0.0
rgb_t0_0 = ls_summer.shade(elevs, cm.gist_earth, dx=PIXEL_RESOLUTION_M, dy=PIXEL_RESOLUTION_M,
                            vert_exag=VERT_EXAG, vmin=0.0)
mask_colors = {
    "pink": "#FFE4E9",    # 以前試した薄桜色
    "purple": "#E6D9F5",  # 新規候補の薄紫
}
for name, hex_color in mask_colors.items():
    rgb_masked = rgb_t0_0.copy()
    rgb_masked[ocean_mask] = to_rgba(hex_color)
    fig, ax = plt.subplots()
    fig.set_size_inches(16.53 * 2, 11.69 * 2)
    ax.imshow(rgb_masked)
    ax.set_xticks([])
    ax.set_yticks([])
    out_path = f"{OUT_DIR}/5338_hillshade_gist_earth_ve{VERT_EXAG}_alt77.7_t0-0.00_mask-{name}.png"
    plt.savefig(out_path, bbox_inches="tight", dpi=150)
    plt.close(fig)
    print(f"出力完了: {out_path}")
