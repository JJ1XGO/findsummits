import matplotlib

matplotlib.use('Agg')
import matplotlib.pyplot as plt
import numpy as np

# 元画像に近い雰囲気の疑似標高データ生成（複数ピークを持つ山岳地形）
np.random.seed(0)
size = 400
y, x = np.mgrid[0:size, 0:size]

def peak(cx, cy, h, r):
    return h * np.exp(-(((x - cx) ** 2 + (y - cy) ** 2) / (2 * r ** 2)))

elev = np.zeros((size, size), dtype=float)
elev += peak(120, 100, 3800, 40)   # 主峰（富士山級）
elev += peak(300, 150, 2800, 60)
elev += peak(200, 300, 3000, 70)
elev += peak(80, 320, 2200, 50)
elev += 200 * np.random.rand(size, size)  # 微細ノイズ（尾根っぽさ）
elev = np.clip(elev, 0, None)

candidates = ['terrain', 'gist_earth']
fig, axes = plt.subplots(1, len(candidates), figsize=(10, 5))
for ax, name in zip(axes, candidates, strict=True):
    im = ax.imshow(elev, cmap=plt.get_cmap(name), vmin=0, vmax=3800)
    ax.set_title(name)
    ax.axis('off')
    fig.colorbar(im, ax=ax, fraction=0.046, pad=0.04)

plt.tight_layout()
plt.savefig('/workspace/analysis/colormap_test/terrain_vs_gistearth.png', dpi=150)
print("done")
