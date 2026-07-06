"""
「山間部はgist_earth風、低地部はcustom_scheme」というハイブリッド配色の試作。

5338_hillshade_gist_earth_ve8_alt77.7_t0-0.00_mask-purple.png と
5338_custom_scheme_flatanchor_k2.0.png は陰影合成方式が異なる
(gist_earth版はmatplotlib標準のls.shade()、custom_schemeは平坦地intensityを0.5に
アンカーする独自blend_overlay方式)ため、そのまま標高マスクで貼り合わせると境目で
陰影の質感が不連続になる。本試作では陰影合成をcustom_scheme側の方式に統一し、
ベースカラー(RGB)のみを標高に応じて低地色(custom_scheme)↔山岳色(gist_earthの
緑〜黄〜オレンジ域を抽出)でブレンドする。
本番パイプライン外の一時調査用スクリプト。
"""
import matplotlib.cm as cm
import matplotlib.pyplot as plt
import numpy as np
from matplotlib.colors import LightSource, LinearSegmentedColormap, hsv_to_rgb, rgb_to_hsv, to_rgba

CACHE_PATH = "/mnt/findsummits/images/5338_cmp_bigtile.cache"
OUT_DIR = "analysis/colormap_test"
STRIDE = 12
PIXEL_RESOLUTION_M = 3.8812 * STRIDE

LAND_STOPS_M = [0.0, 100.0, 300.0, 600.0, 1000.0, 1500.0, 2000.0, 2700.0, 3800.0]
# Fable提案の案A(オリーブ経由)を低地色として採用。現行の濃い緑(彩度C*=42〜47)は
# 山岳部のgist_earth色(オレンジ〜赤)との対比が強すぎ、境目がまだら状になったため、
# 彩度を抑えたオリーブ系に差し替える。
# 関東平野に広く分布する0m/100m帯がグレーがかりすぎるとの指摘のため、彩度のみ引き上げる
# (0m: S0.15→0.30、100m: S0.32→0.42。明度・色相は変えない)。
# 一方300m/600m帯(三浦半島・房総半島などの丘陵に対応)は彩度が高く「濃い」と指摘されたため、
# 100m(S0.42→0.32)/300m(S0.39→0.24)/600m(S0.35→0.20)は彩度を下げる。
# 0-600m帯のHueが70-83度(黄緑〜緑)に集中し「全体に緑のフィルターがかかって見える」との
# 指摘のため、彩度・明度は保ったまま色相のみ黄色寄り(H55-60度)に回転させる。
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

# gist_earthの山岳部分域(緑〜黄〜オレンジ〜赤〜白)のみを抽出して0-1に再マッピングする。
# 下限をさらに黄色寄りのt=0.6へ(濃い緑を抑える)。上限は高所の赤系をさらに抑えたいとの
# 指摘のため0.85→0.75(黄土色止まり、t=0.78以降のオレンジ〜赤域を回避)に下げる。
# 山頂の白飛びはベース色でなく陰影のハイライト効果で担う。
GIST_EARTH_LOW = 0.6
GIST_EARTH_HIGH = 0.68

# 標高に応じたブレンド境界(この間で低地色→山岳色へ線形クロスフェード)
# 房総・三浦半島(大部分が100-300m程度の丘陵)の緑感がまだ弱いため、境界をさらに下げる。
BLEND_LOW_M = 100.0
BLEND_HIGH_M = 400.0

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
elevs_clipped = np.clip(elevs, 0.0, VMAX)

low_rgb = low_cmap(elevs_clipped / VMAX)[..., :3]
# 山岳ゾーン(BLEND_HIGH_M以上)内の相対位置をgist_earthのGIST_EARTH_LOW〜HIGHに
# 線形マッピングする。elevs_clipped/VMAXをそのまま使うと、ブレンド開始標高の時点で
# 既にt=0.5超のオレンジ域に入ってしまい、1000-2000m級の山まで一律オレンジになる
# (色変化がBLEND_HIGH_M〜VMAXのごく一部にしか展開されない)バグがあったため修正。
mountain_t = np.clip((elevs_clipped - BLEND_HIGH_M) / (VMAX - BLEND_HIGH_M), 0.0, 1.0)
gist_t = GIST_EARTH_LOW + mountain_t * (GIST_EARTH_HIGH - GIST_EARTH_LOW)
high_rgb = cm.gist_earth(gist_t)[..., :3]

# gist_earth抽出域(t=0.6-0.85)の彩度はS=0.32-0.50と低地色(S=0.2-0.4程度)より高く、
# これが三浦半島・房総半島(100-400m級)の「濃さ」の本体だったため、彩度を一律に下げる。
GIST_SAT_SCALE = 0.75
high_hsv = rgb_to_hsv(high_rgb)
high_hsv[..., 1] *= GIST_SAT_SCALE
high_rgb = hsv_to_rgb(high_hsv)

blend_t = np.clip((elevs_clipped - BLEND_LOW_M) / (BLEND_HIGH_M - BLEND_LOW_M), 0.0, 1.0)
# 線形クロスフェードだと200m付近(t≈0.33)でも既にgist_earth色が33%混ざり、三浦半島・
# 房総半島(100-400m級)が濃く見えすぎるとの指摘のため、smoothstep(3t²-2t³)で
# 低い方の立ち上がりを緩やかにする(200m: 33%→26%。300-400mでは逆に急峻になり、
# 実際に山らしくなる標高でしっかり山岳色になる)。
blend_ratio = blend_t**2 * (3 - 2 * blend_t)
rgb_base = low_rgb * (1 - blend_ratio[..., np.newaxis]) + high_rgb * blend_ratio[..., np.newaxis]

# 緑に青成分を足して色相を寒色寄りに調整する。
BLUE_BOOST = 0.14
rgb_base[..., 2] = np.clip(rgb_base[..., 2] + BLUE_BOOST, 0.0, 1.0)

# 全体的に緑が強く重いとの指摘のため、ベースカラーに白を軽く混ぜて明るさ・彩度を落とす。
# 陰影(blend_overlay)はベースカラーに対する相対的な明暗変化のため、適度な範囲であれば
# 起伏のコントラスト自体は保たれる。
WHITE_TINT = 0.10
rgb_base = rgb_base * (1 - WHITE_TINT) + np.array([1.0, 1.0, 1.0]) * WHITE_TINT

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
# gist_earth領域の色は低地色(オリーブ)より暗いため、同じk=2.0の陰影を重ねると
# 谷筋が黒潰れする。山岳ゾーンでは陰影強度を弱めて黒潰れを緩和する
# (低地は現状維持のk=2.0のまま、山岳側だけk_highに向けてクロスフェードする)。
K_LOW = 2.0
K_HIGH = 0.7
k_map = K_LOW * (1 - blend_ratio) + K_HIGH * blend_ratio
intensity = np.clip(0.5 + k_map * (raw_intensity - flat_raw), 0.0, 1.0)
rgb_shaded = ls.blend_overlay(rgb_base, intensity[..., np.newaxis])

rgb_masked = np.dstack([rgb_shaded, np.ones(elevs.shape)])
rgb_masked[ocean_mask] = to_rgba(OCEAN_COLOR_HEX)

fig, ax = plt.subplots()
fig.set_size_inches(16.53 * 2, 11.69 * 2)
ax.imshow(rgb_masked)
ax.set_xticks([])
ax.set_yticks([])
out_path = f"{OUT_DIR}/5338_custom_scheme_hybrid_gistearth_k2.0.png"
plt.savefig(out_path, bbox_inches="tight", dpi=150)
plt.close(fig)
print(f"出力完了: {out_path}")
