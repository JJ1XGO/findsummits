"""
「平坦地の明るさ(intensity)を0.5に固定アンカーする」独自hillshade合成の試作。

matplotlib LightSource.shade() の既定挙動は、画像内のintensity最小値〜最大値でその都度
0-1に正規化するため、光源高度altdeg=77.7(ほぼ天頂)では平坦地の理論intensity(sin(altdeg)≈0.977)が
正規化後もほぼ最大値に張り付き、overlay/softブレンドの「intensity=0.5で恒等」という性質により
平坦地のベース色がほぼ白へ収束してしまう(白飛び)ことが判明した(gen_terrain_japan_topo_hillshade.py
での検証、Fableへのアーキテクチャ相談で数式的にも確認済み)。

本スクリプトでは、matplotlib の内部正規化を使わず、
  1. LightSource と同じ法線ベクトル計算(np.gradientベース)で生intensity(cos類似度)を算出
  2. 平坦地の理論値 sin(altdeg) を 0.5 にアンカーする独自の線形写像+clipで0-1intensityを作る
  3. blend_overlay()に直接そのintensityを渡す
という手順で、メッシュ内容に依存しない絶対的な写像に置き換える。

色は Fable(上位モデル)による設計助言をベースにした試作ストップ表(中明度・色相で標高を運ぶ)。
本番パイプライン外の一時調査用スクリプト。
"""
import matplotlib.pyplot as plt
import numpy as np
from matplotlib.colors import LightSource, LinearSegmentedColormap, to_rgba

CACHE_PATH = "/mnt/findsummits/images/5338_cmp_bigtile.cache"
OUT_DIR = "analysis/colormap_test"
STRIDE = 12
PIXEL_RESOLUTION_M = 3.8812 * STRIDE

# gist_earth(vmin=-t0*vmax/(1-t0), t0=0.25)から9ストップ抽出した色に置き換え。
# 元のFable試作案は1000-2000m帯で明度が谷型に落ち込み(L=0.537→0.412)暗い赤褐色に
# 潰れる問題があったため、gist_earthの単調増加する明度カーブを踏襲する。
LAND_STOPS_M = [0.0, 100.0, 300.0, 600.0, 1000.0, 1500.0, 2000.0, 2700.0, 3800.0]
LAND_COLORS_HEX = [
    "#6b6b62", "#b59449", "#89b260", "#5cbc8d",
    "#7cc483", "#aecc98", "#d2d9af", "#e8dec8", "#f9f4f4",
]
OCEAN_COLOR_HEX = "#B7E5FA"
VMAX = LAND_STOPS_M[-1]

positions = [s / VMAX for s in LAND_STOPS_M]
custom_cmap = LinearSegmentedColormap.from_list(
    "custom_land", list(zip(positions, LAND_COLORS_HEX, strict=True)), N=256
)

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

# --- 独自intensity計算(matplotlib hillshade()と同じ法線計算を再現) ---
dy = -PIXEL_RESOLUTION_M
dx = PIXEL_RESOLUTION_M
e_dy, e_dx = np.gradient(VERT_EXAG * elevs, dy, dx)
normal = np.empty(elevs.shape + (3,))
normal[..., 0] = -e_dx
normal[..., 1] = -e_dy
normal[..., 2] = 1
normal /= np.linalg.norm(normal, axis=2, keepdims=True)
raw_intensity = normal.dot(ls.direction)

flat_raw = np.sin(np.radians(ALTDEG))  # 平坦地の理論生intensity

ocean_mask = elevs <= 0.0
rgb_base = custom_cmap(np.clip(elevs, 0.0, VMAX) / VMAX)[..., :3]

for k in (1.0, 2.0):
    intensity = np.clip(0.5 + k * (raw_intensity - flat_raw), 0.0, 1.0)
    rgb_shaded = ls.blend_overlay(rgb_base, intensity[..., np.newaxis])
    rgb_masked = np.dstack([rgb_shaded, np.ones(elevs.shape)])
    rgb_masked[ocean_mask] = to_rgba(OCEAN_COLOR_HEX)

    fig, ax = plt.subplots()
    fig.set_size_inches(16.53 * 2, 11.69 * 2)
    ax.imshow(rgb_masked)
    ax.set_xticks([])
    ax.set_yticks([])
    out_path = f"{OUT_DIR}/5338_custom_scheme_flatanchor_k{k:.1f}.png"
    plt.savefig(out_path, bbox_inches="tight", dpi=150)
    plt.close(fig)
    print(f"出力完了: {out_path}")
