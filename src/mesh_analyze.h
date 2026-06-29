/*
 * mesh_analyze.h - 1次メッシュ単位の解析
 *
 * SOTA日本支部 未登録サミット候補探索ツール
 * Copyright (C) 2026 JJ1XGO
 * GPL-3.0
 */
#ifndef MESH_ANALYZE_H
#define MESH_ANALYZE_H

#include "mesh.h"
#include "analyze.h"

/*
 * メッシュ解析設定
 */
typedef struct {
    const char    *tile_dir;       /* タイルキャッシュディレクトリ */
    const char    *result_dir;     /* 結果保存ディレクトリ */
    const char    *img_dir;        /* 標高地形図 PNG 出力先（NULL=出力しない） */
    const char    *log_dir;        /* ログ出力先（NULL=出力しない） */
    float          min_prominence; /* 最小プロミネンス(m) */
    const MeshSet *mesh_set;       /* 隣接判定用メッシュリスト */
} MeshAnalyzeConfig;

/* 関数プロトタイプ */
int mesh_analyze(const MeshAnalyzeConfig *cfg, int meshcode);
/* 外部から巨大イメージを作成して使うための関数（視覚化用） */
ElevTile *load_mesh_tile(const char *tile_dir, const MeshTileRange *range, FILE *logfp);
/* ピクセル座標から緯度経度を計算する（外部から使用可能） */
void pixel_to_latlon(const MeshTileRange *range, int px, int py, double *lat, double *lon);
#endif /* MESH_ANALYZE_H */
