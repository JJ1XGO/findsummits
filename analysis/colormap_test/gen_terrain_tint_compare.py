"""
gen_terrain_custom_scheme.py の陸地ベース色(LAND_COLORS_HEX)が「キツイ(濃い)」という
フィードバックを受け、白tintで薄くした場合の実データでの見え方を、既存のコントラスト係数
k=1.0/2.0との組み合わせで比較する。
tint係数は gen_color_palette_lighten_compare.py の色見本比較で候補に挙がった
tint0.30/0.45を採用する。
本番パイプライン外の一時調査用スクリプト。
"""
import matplotlib.pyplot as plt
import numpy as np
from matplotlib.colors import LightSource, LinearSegmentedColormap, to_rgb, to_rgba

CACHE_PATH = "/mnt/findsummits/images/5338_cmp_bigtile.cache"
OUT_DIR = "analysis/colormap_test"
STRIDE = 12
PIXEL_RESOLUTION_M = 3.8812 * STRIDE

LAND_STOPS_M = [0.0, 100.0, 300.0, 600.0, 1000.0, 1500.0, 2000.0, 2700.0, 3800.0]
LAND_COLORS_HEX_ORIG = [
    "#6b6b62", "#b59449", "#89b260", "#5cbc8d",
    "#7cc483", "#aecc98", "#d2d9af", "#e8dec8", "#f9f4f4",
]
OCEAN_COLOR_HEX = "#B7E5FA"
VMAX = LAND_STOPS_M[-1]
positions = [s / VMAX for s in LAND_STOPS_M]

TINT_LEVELS = [0.30, 0.45]
K_LEVELS = [1.0, 2.0]


def tinted_colors(t):
    white = np.array([1.0, 1.0, 1.0])
    return [tuple(np.array(to_rgb(c)) * (1 - t) + white * t) for c in LAND_COLORS_HEX_ORIG]


with open(CACHE_PATH, "rb") as f:
    width = np.fromfile(f, dtype=np.uint32, count=1)[0]
    height = np.fromfile(f, dtype=np.uint32, count=1)[0]
    header_bytes = 8

print(f"元サイズ: {width}x{height}")
data = np.memmap(CACHE_PATH, dtype=np.float32, mode="r",
                  offset=header_bytes, shape=(height, width))
elevs = np.array(data[::STRIDE, ::STRIDE])
print(f"間引き後サイズ: {elevs.shape[1]}x{elevs.shape[0]}")

AZDEG = 180
ALTDEG = 77.7
VERT_EXAG = 8
ls = LightSource(azdeg=AZDEG, altdeg=ALTDEG)

dy = -PIXEL_RESOLUTION_M
dx = PIXEL_RESOLUTION_M
e_dy, e_dx = np.gradient(VERT_EXAG * elevs, dy, dx)
normal = np.empty(elevs.shape + (3,))
normal[..., 0] = -e_dx
normal[..., 1] = -e_dy
normal[..., 2] = 1
normal /= np.linalg.norm(normal, axis=2, keepdims=True)
raw_intensity = normal.dot(ls.direction)

flat_raw = np.sin(np.radians(ALTDEG))
ocean_mask = elevs <= 0.0

for t in TINT_LEVELS:
    custom_cmap = LinearSegmentedColormap.from_list(
        "custom_land_tint", list(zip(positions, tinted_colors(t), strict=True)), N=256
    )
    rgb_base = custom_cmap(np.clip(elevs, 0.0, VMAX) / VMAX)[..., :3]
    for k in K_LEVELS:
        intensity = np.clip(0.5 + k * (raw_intensity - flat_raw), 0.0, 1.0)
        rgb_shaded = ls.blend_overlay(rgb_base, intensity[..., np.newaxis])
        rgb_masked = np.dstack([rgb_shaded, np.ones(elevs.shape)])
        rgb_masked[ocean_mask] = to_rgba(OCEAN_COLOR_HEX)

        fig, ax = plt.subplots()
        fig.set_size_inches(16.53 * 2, 11.69 * 2)
        ax.imshow(rgb_masked)
        ax.set_xticks([])
        ax.set_yticks([])
        out_path = f"{OUT_DIR}/5338_custom_scheme_tint{t:.2f}_k{k:.1f}.png"
        plt.savefig(out_path, bbox_inches="tight", dpi=150)
        plt.close(fig)
        print(f"出力完了: {out_path}")
