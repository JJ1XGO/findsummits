# キャッシュ済み標高タイルを新ディレクトリ形式へ移行 + 読み取りコード追従

## Context

直前のコミット `94dc49a` で SRS 7.2 のローカルキャッシュ命名を旧 suffix 方式
（`tiles/{z}/{x}/{y}_{dem}.png`）からタイル URL ミラー方式
（`tiles/{サービス名}/{z}/{x}/{y}.png`）へ変更した。

既にローカルにある **98,698 枚**の旧形式キャッシュを新形式へ置き直す。
ただし旧形式を読むコードが 2 関数あり、キャッシュだけ移すと findsummits / prefetch が
タイルを見つけられず壊れるため、**移行と読み取りコード更新を同時に行う**（ユーザー確認済み）。

旧→新マッピング（決定的）:

| 旧 | 新 | 枚数 |
|---|---|---|
| `15/{x}/{y}_a.png` | `dem5a_png/15/{x}/{y}.png` | 83,492 |
| `15/{x}/{y}_b.png` | `dem5b_png/15/{x}/{y}.png` | 250 |
| `15/{x}/{y}_c.png` | `dem5c_png/15/{x}/{y}.png` | 255 |
| `14/{x}/{y}_b.png` | `dem_png/14/{x}/{y}.png` | 14,698 |

サービス名規則: `z==14 → dem_png` / `z==15 → dem5{dem}_png`（dem=a/b/c）。

## 変更内容

### 1. コード追従（パス組み立て 2 関数）

**`src/elevation.c` `make_tile_path`（92-98 行）** — 内部で z→サービス名を導出。呼び出し側（105/126/276/363 行）は変更不要:
```c
/* パス形式: {base}/{service}/{z}/{x}/{y}.png
 * service: z==14 → dem_png / z==15 → dem5{dem}_png */
static void make_tile_path(char *buf, size_t bufsize,
                            const char *base, int z,
                            int x, int y, const char *dem)
{
    if (z == 14)
        snprintf(buf, bufsize, "%s/dem_png/%d/%d/%d.png", base, z, x, y);
    else
        snprintf(buf, bufsize, "%s/dem5%s_png/%d/%d/%d.png", base, dem, z, x, y);
}
```

**`scripts/prefetch_tiles.py` `tile_path`（70-71 行）** — `tile_url`（74-78 行）と対称になる:
```python
def tile_path(tile_dir, z, x, y, dem):
    service = "dem_png" if z == 14 else f"dem5{dem}_png"
    return os.path.join(tile_dir, service, str(z), str(x), f"{y}.png")
```

### 2. キャッシュ移行（一回限り・$DATA_DIR=/data）

一回限りの移行スクリプトを `/tmp/migrate_tiles.py` に作成して `venv/bin/python3` で実行
（git 管理外・実行後破棄）。要点:
- `tiles/{14,15}/{x}/{y}_{suffix}.png` を走査し `os.rename` で `tiles/{service}/{z}/{x}/{y}.png` へ移動
  - `os.rename`（同一 FS の move）は inode 不変で **mtime を保持** → If-Modified-Since 条件付き GET がそのまま機能
  - `os.makedirs(dst_dir, exist_ok=True)` でサービス名ディレクトリを作成
  - suffix は a/b/c のみ許可（想定外名はスキップしてログ）
- 移動後、空になった旧 `14/` `15/` のディレクトリツリーを削除
- 冪等（再実行で既移動分は無害）

### 3. ビルド & todo 整理

- `make` で C エンジン再ビルド
- `mgmt/todo.md` の「FR-001 追従」項目から、今回完了したパス変更サブ項目
  （`tile_path` / `elevation.c:97` を新方式へ）を削除（残りの FR-001 作業は据え置き）

## 対象ファイル

- `src/elevation.c`（92-98 行）
- `scripts/prefetch_tiles.py`（70-71 行）
- `mgmt/todo.md`（FR-001 追従項目の整理）
- `/tmp/migrate_tiles.py`（一回限り・非コミット）

## 実行順序（破壊ウィンドウを作らない）

1. コード 2 関数を編集 → `make`（コンパイル成功確認）
2. `/tmp/migrate_tiles.py` 実行（移行）
3. 検証（下記）
4. コミット（コード + todo.md）

## 検証

- 移行前後でタイル総数一致: `find /data/tiles -name '*.png' | wc -l` が **98,698** のまま
- 新形式に揃ったこと:
  - `find /data/tiles/dem5a_png -name '*.png' | wc -l` = 83,492、`dem5b_png`=250、`dem5c_png`=255、`dem_png`=14,698
  - 旧 `/data/tiles/15` `/data/tiles/14` が消えている（空削除済み）
- `_a.png/_b.png/_c.png` 残存ゼロ: `find /data/tiles -name '*_[abc].png' | wc -l` = 0
- `make` がエラーなく完了
- サンプル整合: 任意の (z,x,y,dem) について `make_tile_path` 相当の新パスを shell で組み立て `ls` でヒット
- （任意・heavy）findsummits の実走確認は待ち時間が長いため、ユーザーの明示 GO があってから実施

## コミット

- 対象: `src/elevation.c` `scripts/prefetch_tiles.py` `mgmt/todo.md`
- Conventional Commits・本文日本語（例: `refactor(elevation): タイルキャッシュ読み取りを URL ミラー形式へ追従`）
- push はしない
