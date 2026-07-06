"""
gen_terrain_custom_scheme.py の陸地ベース色(LAND_COLORS_HEX)が「キツイ(濃い)」という
フィードバックを受け、薄くする方向性を比較するための検証用画像を生成する。

薄くする方向を2通り比較する:
  1. 白とのアルファブレンド(tint): 明度を上げつつ彩度も下がる
  2. HSV彩度のみ低下: 明度は保ったまま彩度だけ下げる
本番パイプライン外の一時調査用スクリプト。
"""
import matplotlib

matplotlib.use('Agg')
import matplotlib.pyplot as plt
import numpy as np
from matplotlib.colors import hsv_to_rgb, rgb_to_hsv, to_rgb

OUT_DIR = "analysis/colormap_test"

LAND_STOPS_M = [0.0, 100.0, 300.0, 600.0, 1000.0, 1500.0, 2000.0, 2700.0, 3800.0]
LAND_COLORS_HEX = [
    "#6b6b62", "#b59449", "#89b260", "#5cbc8d",
    "#7cc483", "#aecc98", "#d2d9af", "#e8dec8", "#f9f4f4",
]

TINT_LEVELS = [0.0, 0.15, 0.30, 0.45]
SAT_LEVELS = [0.25, 0.50]

n_cols = len(LAND_COLORS_HEX)
n_rows = len(TINT_LEVELS) + len(SAT_LEVELS)

fig, axes = plt.subplots(n_rows, n_cols, figsize=(n_cols * 1.4, n_rows * 1.1))

row = 0
for t in TINT_LEVELS:
    for col, hexcolor in enumerate(LAND_COLORS_HEX):
        base = np.array(to_rgb(hexcolor))
        tinted = base * (1 - t) + np.array([1.0, 1.0, 1.0]) * t
        ax = axes[row, col]
        ax.imshow([[tinted]])
        ax.set_xticks([])
        ax.set_yticks([])
        if row == 0:
            ax.set_title(f"{LAND_STOPS_M[col]:.0f}m", fontsize=8)
        if col == 0:
            label = "orig" if t == 0.0 else f"tint{t:.2f}"
            ax.set_ylabel(label, rotation=0, ha="right", va="center", fontsize=8)
    row += 1

for s in SAT_LEVELS:
    for col, hexcolor in enumerate(LAND_COLORS_HEX):
        base = np.array(to_rgb(hexcolor))
        hsv = rgb_to_hsv(base)
        hsv[1] *= (1 - s)
        desat = hsv_to_rgb(hsv)
        ax = axes[row, col]
        ax.imshow([[desat]])
        ax.set_xticks([])
        ax.set_yticks([])
        if col == 0:
            ax.set_ylabel(f"sat-{s:.2f}", rotation=0, ha="right", va="center", fontsize=8)
    row += 1

plt.tight_layout()
out_path = f"{OUT_DIR}/color_palette_lighten_compare.png"
plt.savefig(out_path, dpi=150, bbox_inches="tight")
print(f"出力完了: {out_path}")
