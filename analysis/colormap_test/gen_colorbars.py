import matplotlib

matplotlib.use('Agg')
import matplotlib.pyplot as plt
import numpy as np

# matplotlib 2.0.2 時点に存在したカラーマップのうち、地形図的（低→高で「白」に抜ける、
# または多色でカラフル）な候補に絞る。target画像は 500m刻みで濃紺→シアン→緑→黄緑→
# オレンジ→山頂(3776m付近)で白飽和、という特徴を持つ。
candidates = [
    'terrain', 'gist_earth', 'ocean',
    'gist_ncar', 'nipy_spectral', 'gist_stern',
    'CMRmap', 'cubehelix', 'gnuplot', 'gnuplot2',
    'jet', 'rainbow', 'gist_rainbow',
]

fig, axes = plt.subplots(len(candidates), 1, figsize=(8, len(candidates) * 0.6))
gradient = np.linspace(0, 1, 256).reshape(1, -1)

for ax, name in zip(axes, candidates, strict=True):
    ax.imshow(gradient, aspect='auto', cmap=plt.get_cmap(name))
    ax.set_yticks([])
    ax.set_xticks([])
    ax.set_ylabel(name, rotation=0, ha='right', va='center', fontsize=10)

plt.tight_layout()
plt.savefig('/workspace/analysis/colormap_test/candidates.png', dpi=150)
print("done")
