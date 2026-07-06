"""
gen_terrain_custom_scheme.py の陸地配色(現行)と、Fableが提案した3案(A_olive/B_sage/C_green)を
矩形色見本(9ストップ×4案)で比較するための検証用画像を生成する。
本番パイプライン外の一時調査用スクリプト。
"""
import matplotlib

matplotlib.use('Agg')
import matplotlib.pyplot as plt
from matplotlib.colors import to_rgb

LAND_STOPS_M = [0.0, 100.0, 300.0, 600.0, 1000.0, 1500.0, 2000.0, 2700.0, 3800.0]

PALETTES = {
    "current": [
        "#6b6b62", "#b59449", "#89b260", "#5cbc8d",
        "#7cc483", "#aecc98", "#d2d9af", "#e8dec8", "#f9f4f4",
    ],
    "A_olive": [
        "#6e7160", "#7f8b5e", "#8aa163", "#9cb072",
        "#b5b981", "#cabf8e", "#dccba1", "#ecdfc5", "#faf6f0",
    ],
    "B_sage": [
        "#767b6c", "#8c9377", "#9da67f", "#adad86",
        "#bcb28b", "#ccbd97", "#dbcda9", "#eadfc8", "#f9f6f1",
    ],
    "C_green": [
        "#5c7d55", "#6d9059", "#82a061", "#96ac6c",
        "#aeb479", "#c5ba8a", "#d8c9a0", "#eadfc6", "#faf7f2",
    ],
}

n_rows = len(PALETTES)
n_cols = len(LAND_STOPS_M)

fig, axes = plt.subplots(n_rows, n_cols, figsize=(n_cols * 1.4, n_rows * 1.3))

for row, (name, colors) in enumerate(PALETTES.items()):
    for col, (stop, hexcolor) in enumerate(zip(LAND_STOPS_M, colors, strict=True)):
        ax = axes[row, col]
        ax.imshow([[to_rgb(hexcolor)]])
        ax.set_xticks([])
        ax.set_yticks([])
        if row == 0:
            ax.set_title(f"{stop:.0f}m", fontsize=8)
        ax.set_xlabel(hexcolor, fontsize=6)
        if col == 0:
            ax.set_ylabel(name, rotation=0, ha="right", va="center", fontsize=9)

plt.tight_layout()
out_path = "analysis/colormap_test/color_palette_candidates_compare.png"
plt.savefig(out_path, dpi=150, bbox_inches="tight")
print(f"出力完了: {out_path}")
