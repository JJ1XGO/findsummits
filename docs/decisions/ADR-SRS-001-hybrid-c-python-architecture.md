# ADR-SRS-001: C + Python ハイブリッドアーキテクチャ

| 項目 | 内容 |
|---|---|
| 状態 | 採用・実装済み（ADR-SRS-010 により C 部分が C++ に置換予定） |
| 決定日 | 2026-04-20 |

---

## Context

標高タイルのデコード・Union-Find による山頂/コル検出は大量のピクセルを処理するため、  
スクリプト言語では処理速度・メモリ効率が不十分になる可能性があった。  
一方、申請用出力（XLSX・GeoJSON）は Python に実績コードが存在していた。

## Decision

- **C エンジン** (`src/`): タイル読み込み・標高デコード・Union-Find 山頂/コル検出・per-mesh CSV 出力
- **Python スクリプト** (`scripts/`): 複数 CSV 統合・SOTA リスト突合・XLSX/GeoJSON 生成

## Alternatives

| 案 | 却下理由 |
|---|---|
| 全部 C | XLSX/GeoJSON ライブラリを C に持ち込むコストが高い |
| 全部 Python | Union-Find のメモリ・速度要件を満たせない懸念 |

## Consequences

- C と Python の境界は per-mesh CSV ファイル（`$DATA_DIR/results/csv/<meshcode>.csv`）
- 性能要求のある処理は C、出力フォーマット要求は Python で対応する

## 後続 ADR との関係

[ADR-SRS-010](ADR-SRS-010-cpp-opencv-migration.md) で C エンジン部分を C++ に移行し、画像処理関連で OpenCV を採用することが決まった。
本 ADR のハイブリッド原則（性能要求は C/C++、出力フォーマットは Python）は維持される。
