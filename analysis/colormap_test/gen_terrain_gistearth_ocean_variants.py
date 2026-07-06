"""
5338_hillshade_gist_earth_ve8_alt77.7_t0-0.00(海面マスクなしのgist_earth標準版)に、
異なるt0・海マスク色を適用したバリエーションを生成する。

gen_terrain_hillshade.py と同じ計算(gist_earth + LightSource(azdeg=180, altdeg=77.7) +
vert_exag=8)を再利用する。t0=0.00(vmin=0.0)だと標高0m付近がgist_earthの最も暗い側
(ほぼ黒)にマッピングされ関東平野が暗く沈むため、t0を上げてvminを負側にずらし、
陸地の低地がより明るい側にマッピングされるようにする。t0を上げると海も同じ
カラーマップで明るくなり陸地の低地と同化する問題があったが、海面(elev<=0)を
別途マスク色で塗りつぶすことでこれを回避する。
vmax(=elevs.max())はこのメッシュの実測最大標高であり、固定値ではなく解析範囲ごとの
相対値として扱う(2026-07-06、低山メッシュでの色レンジ確保を優先してユーザーが決定)。

採用決定(2026-07-06、ADR-SRS-047): t0=0.25 + mask-white
(`5338_hillshade_gist_earth_ve8_alt77.7_t0-0.25_mask-white.png`)。
customoceanマスク・他のt0値は採用検討時の比較用サンプル。
本番パイプライン外の一時調査用スクリプト。
"""
import matplotlib.cm as cm
import matplotlib.pyplot as plt
import numpy as np
from matplotlib.colors import LightSource, to_rgba

CACHE_PATH = "/mnt/findsummits/images/5338_cmp_bigtile.cache"
OUT_DIR = "analysis/colormap_test"
STRIDE = 12
PIXEL_RESOLUTION_M = 3.8812 * STRIDE
T0_LIST = [0.25, 0.30]

with open(CACHE_PATH, "rb") as f:
    width = np.fromfile(f, dtype=np.uint32, count=1)[0]
    height = np.fromfile(f, dtype=np.uint32, count=1)[0]
    header_bytes = 8

print(f"元サイズ: {width}x{height}")
data = np.memmap(CACHE_PATH, dtype=np.float32, mode="r",
                  offset=header_bytes, shape=(height, width))
elevs = np.array(data[::STRIDE, ::STRIDE])
print(f"間引き後サイズ: {elevs.shape[1]}x{elevs.shape[0]}")

ls_summer = LightSource(azdeg=180, altdeg=77.7)
VERT_EXAG = 8

vmax = elevs.max()
ocean_mask = elevs <= 0.0

mask_colors = {
    "customocean": "#B7E5FA",
    "white": "#FFFFFF",
}

for t0 in T0_LIST:
    vmin = -t0 * vmax / (1 - t0)
    print(f"t0={t0}: vmin={vmin:.1f}m")
    rgb_t0 = ls_summer.shade(elevs, cm.gist_earth, dx=PIXEL_RESOLUTION_M, dy=PIXEL_RESOLUTION_M,
                              vert_exag=VERT_EXAG, vmin=vmin)

    for name, hex_color in mask_colors.items():
        rgb_masked = rgb_t0.copy()
        rgb_masked[ocean_mask] = to_rgba(hex_color)

        fig, ax = plt.subplots()
        fig.set_size_inches(16.53 * 2, 11.69 * 2)
        ax.imshow(rgb_masked)
        ax.set_xticks([])
        ax.set_yticks([])
        out_path = f"{OUT_DIR}/5338_hillshade_gist_earth_ve{VERT_EXAG}_alt77.7_t0-{t0:.2f}_mask-{name}.png"
        plt.savefig(out_path, bbox_inches="tight", dpi=150)
        plt.close(fig)
        print(f"出力完了: {out_path}")
