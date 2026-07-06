"""
tingwu.info (https://tingwu.info/pylab/lab04.html) で紹介されている
「gist_earthの階調を指数関数的なboundsに割り当てる」手法の検証。

手法:
  1. 標高境界(bounds)を等間隔の値ではなく exp(v) で指数的に間隔を広げて生成する
     （低標高側の階調を密に、高標高側を疎にする）
  2. BoundaryNorm で標高値を色インデックスへ離散対応させる

疑似データでの先行検証(同ディレクトリのgit履歴参照)で、gist_earth先頭5色（暗い紺系）を
除去してもしなくても結果はほぼ同じ（300m以上がほぼ同一色域に潰れる）と確認済みのため、
本スクリプトでは除去なし版のみを実データ(5338中心3×3メッシュ結合キャッシュ)で再確認する。

既存のC側キャッシュ (5338_cmp_bigtile.cache: uint32 width, uint32 height, float32 data...) を
numpy.memmap でそのまま読み込み、間引いてダウンサンプルしてから処理する（16GB全体は読まない）。
本番パイプライン外の一時調査用スクリプト。
"""
import matplotlib

matplotlib.use('Agg')
import matplotlib.colors as mcol
import matplotlib.pyplot as plt
import numpy as np

CACHE_PATH = "/mnt/findsummits/images/5338_cmp_bigtile.cache"
OUT_DIR = "analysis/colormap_test"
STRIDE = 12  # 70146x57602 -> 約5845x4800

with open(CACHE_PATH, "rb") as f:
    width = np.fromfile(f, dtype=np.uint32, count=1)[0]
    height = np.fromfile(f, dtype=np.uint32, count=1)[0]
    header_bytes = 8

print(f"元サイズ: {width}x{height}")
data = np.memmap(CACHE_PATH, dtype=np.float32, mode="r",
                  offset=header_bytes, shape=(height, width))
elevs = np.array(data[::STRIDE, ::STRIDE])
print(f"間引き後サイズ: {elevs.shape[1]}x{elevs.shape[0]}")

ocean_mask = elevs <= 0.0
land_max = elevs[~ocean_mask].max()
print(f"陸地最大標高: {land_max:.1f}m")

# --- 指数bounds生成（tingwu.info方式、色除去なし版） ---
# exp(log(land_max)) が land_max をわずかに超えるよう上限を決める
log_max = np.log(land_max)
m = np.arange(0, log_max + 0.2, 0.2)
bounds = [0.0]
for val in m:
    bounds.append(np.exp(val))

cmap_exp = mcol.LinearSegmentedColormap.from_list(
    "gist_earth_expbounds", [plt.cm.gist_earth(i) for i in range(plt.cm.gist_earth.N)], len(bounds) + 1)
norm_exp = mcol.BoundaryNorm(np.array(bounds), cmap_exp.N)

OCEAN_COLOR_HEX = "#1a4f72"

# --- 比較描画 ---
fig, axes = plt.subplots(1, 2, figsize=(20, 9))

rgba_linear = plt.cm.gist_earth(np.clip(elevs, 0.0, land_max) / land_max)
rgba_linear[ocean_mask] = mcol.to_rgba(OCEAN_COLOR_HEX)
axes[0].imshow(rgba_linear)
axes[0].set_title("gist_earth (linear Normalize)")
axes[0].set_xticks([])
axes[0].set_yticks([])

rgba_exp = cmap_exp(norm_exp(elevs))
rgba_exp[ocean_mask] = mcol.to_rgba(OCEAN_COLOR_HEX)
axes[1].imshow(rgba_exp)
axes[1].set_title("gist_earth + exp bounds (tingwu.info)")
axes[1].set_xticks([])
axes[1].set_yticks([])

plt.tight_layout()
out_path = f"{OUT_DIR}/5338_gist_earth_expbounds_vs_linear.png"
plt.savefig(out_path, dpi=150, bbox_inches="tight")
print(f"出力完了: {out_path}")
