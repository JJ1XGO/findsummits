"""
color_palette_all.png の各標高ストップ色見本に、独自hillshade合成(LightSource.blend_overlay)で
intensityを変化させたときの色見本を追加した検証用画像を生成する。

gen_terrain_hybrid_gistearth.py の現在の配色ロジック(低地色↔gist_earth山岳色の標高
クロスフェード、白tint)による最終色を再現して表示する。overlayブレンドはintensity=0.5が
元の色のまま、0に近いほど暗く、1に近いほど明るくなる性質を持つため、0.25/0.5/0.75の
3段階(暗め/元色/明るめ)で確認する。
海(ocean)は地形図上でも陰影をかけない平坦色のため、shade行では対象外とする。
本番パイプライン外の一時調査用スクリプト。
"""
import matplotlib

matplotlib.use('Agg')
import matplotlib.cm as cm
import matplotlib.pyplot as plt
import numpy as np
from matplotlib.colors import LightSource, LinearSegmentedColormap, to_rgb

OUT_DIR = "analysis/colormap_test"

LAND_STOPS_M = [0.0, 100.0, 300.0, 600.0, 1000.0, 1500.0, 2000.0, 2700.0, 3800.0]
LAND_COLORS_HEX = [
    "#716e4e", "#8b895e", "#a1a160", "#b0ae8c",
    "#b5b981", "#cabf8e", "#dccba1", "#ecdfc5", "#faf6f0",
]
OCEAN_COLOR_HEX = "#B7E5FA"
VMAX = LAND_STOPS_M[-1]
positions = [s / VMAX for s in LAND_STOPS_M]
low_cmap = LinearSegmentedColormap.from_list(
    "custom_land", list(zip(positions, LAND_COLORS_HEX, strict=True)), N=256
)

GIST_EARTH_LOW = 0.6
GIST_EARTH_HIGH = 0.85
BLEND_LOW_M = 100.0
BLEND_HIGH_M = 400.0
WHITE_TINT = 0.10


def final_color(elev_m):
    elev_clipped = np.clip(elev_m, 0.0, VMAX)
    low_rgb = np.array(low_cmap(elev_clipped / VMAX)[:3])
    mountain_t = np.clip((elev_clipped - BLEND_HIGH_M) / (VMAX - BLEND_HIGH_M), 0.0, 1.0)
    gist_t = GIST_EARTH_LOW + mountain_t * (GIST_EARTH_HIGH - GIST_EARTH_LOW)
    high_rgb = np.array(cm.gist_earth(gist_t)[:3])
    blend_ratio = np.clip((elev_clipped - BLEND_LOW_M) / (BLEND_HIGH_M - BLEND_LOW_M), 0.0, 1.0)
    rgb = low_rgb * (1 - blend_ratio) + high_rgb * blend_ratio
    rgb = rgb * (1 - WHITE_TINT) + np.array([1.0, 1.0, 1.0]) * WHITE_TINT
    return rgb


final_colors = [final_color(s) for s in LAND_STOPS_M]

INTENSITIES = [0.25, 0.5, 0.75]

ls = LightSource()
n_land = len(LAND_STOPS_M)
n_cols = 1 + n_land  # 海 + 陸地9色
n_rows = 1 + len(INTENSITIES)  # ベース行 + shade段階行

fig, axes = plt.subplots(n_rows, n_cols, figsize=(n_cols * 1.4, n_rows * 1.3))

labels = ["ocean"] + [f"{s:.0f}m" for s in LAND_STOPS_M]
display_hex = [OCEAN_COLOR_HEX] + [
    "#{:02x}{:02x}{:02x}".format(*(np.clip(c, 0.0, 1.0) * 255).astype(int)) for c in final_colors
]
display_rgb = [np.array(to_rgb(OCEAN_COLOR_HEX))] + final_colors
for col, (label, hexcolor, rgbcolor) in enumerate(zip(labels, display_hex, display_rgb, strict=True)):
    ax = axes[0, col]
    ax.imshow([[rgbcolor]])
    ax.set_xticks([])
    ax.set_yticks([])
    ax.set_title(f"{label}\n{hexcolor}", fontsize=8)

for row, intensity in enumerate(INTENSITIES, start=1):
    axes[row, 0].axis("off")
    for col, rgbcolor in enumerate(final_colors, start=1):
        base = np.array(rgbcolor).reshape(1, 1, 3)
        intensity_arr = np.full((1, 1, 1), intensity)
        shaded = ls.blend_overlay(base, intensity_arr)
        ax = axes[row, col]
        ax.imshow(shaded)
        ax.set_xticks([])
        ax.set_yticks([])
        if col == 1:
            ax.set_ylabel(f"I={intensity:.2f}", rotation=0, ha="right", va="center", fontsize=8)

plt.tight_layout()
out_path = f"{OUT_DIR}/color_palette_all_with_shade.png"
plt.savefig(out_path, dpi=150, bbox_inches="tight")
print(f"出力完了: {out_path}")
