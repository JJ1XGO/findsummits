"""
旧 Japan Topo スキーム（src/mesh_analyze.c の elev_to_rgb() と同じ8段階ストップ・出典:
docs/decisions/research/dem_colormap.html）に、gen_terrain_hillshade.py と同一のパラメータで
hillshade を重ねた場合の見え方を確認する。

Japan Topo は「陰影なし」の状態でのみ比較され却下された経緯があり、hillshade を組み合わせた
検証は未実施だった。ユーザーの再確認依頼を受けて実施。
本番パイプライン外の一時調査用スクリプト。
"""
import matplotlib.pyplot as plt
import numpy as np
from matplotlib.colors import LightSource, LinearSegmentedColormap, to_rgba

CACHE_PATH = "/data/images/5338_cmp_bigtile.cache"
OUT_DIR = "/workspace/analysis/colormap_test"
STRIDE = 12

# gen_terrain_hillshade.py と同じ算出根拠（ズーム15・5338メッシュ中央緯度35.6667度）
PIXEL_RESOLUTION_M = 3.8812 * STRIDE

# src/mesh_analyze.c elev_to_rgb() の陸地側ストップ（0m以降）をそのまま移植
LAND_STOPS_M = [0.0, 300.0, 900.0, 1800.0, 2700.0, 3800.0]
LAND_COLORS_HEX = ["#e8f5c8", "#a8d08d", "#6aa84f", "#b5651d", "#8b7355", "#f0ebe3"]
OCEAN_COLOR_HEX = "#1a4f72"  # Japan Topo の「海洋」色をマスク色として流用
VMAX = LAND_STOPS_M[-1]  # 3800.0

positions = [s / VMAX for s in LAND_STOPS_M]
japan_topo_cmap = LinearSegmentedColormap.from_list(
    "japan_topo_land", list(zip(positions, LAND_COLORS_HEX, strict=True)), N=256
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

# gen_terrain_hillshade.py の採用サンプルと同一の光源条件
ls_summer = LightSource(azdeg=180, altdeg=77.7)
VERT_EXAG = 8

ocean_mask = elevs <= 0.0

# fraction: 平坦地(intensity正規化後1.0近傍)がoverlayブレンドで白飛びする度合いを
# 抑えるためのコントラスト調整。1.0(既定)との見比べ用に複数値を試す。
for fraction in (1.0, 0.6, 0.4):
    rgb_shaded = ls_summer.shade(elevs, japan_topo_cmap, dx=PIXEL_RESOLUTION_M, dy=PIXEL_RESOLUTION_M,
                                  vert_exag=VERT_EXAG, vmin=0.0, vmax=VMAX, fraction=fraction)
    rgb_masked = rgb_shaded.copy()
    rgb_masked[ocean_mask] = to_rgba(OCEAN_COLOR_HEX)

    fig, ax = plt.subplots()
    fig.set_size_inches(16.53 * 2, 11.69 * 2)
    ax.imshow(rgb_masked)
    ax.set_xticks([])
    ax.set_yticks([])
    out_path = f"{OUT_DIR}/5338_hillshade_japan_topo_ve{VERT_EXAG}_alt77.7_frac{fraction:.1f}_mask-ocean.png"
    plt.savefig(out_path, bbox_inches="tight", dpi=150)
    plt.close(fig)
    print(f"出力完了: {out_path}")
