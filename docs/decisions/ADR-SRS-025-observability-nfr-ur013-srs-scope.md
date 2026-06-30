# ADR-SRS-025: UR-013（観測可能性）の SRS 受け皿設計 — NFR 化とスコープ分割

| 状態 | 採用・未実装 |
| 決定日 | 2026-06-17 |

## Context

[UR-013](../10_URD.md#ur-013)（[ADR-URD-016](ADR-URD-016-observability-intermediate-visualization-ur.md)）が
URD に追加されたが、SRS 側に受け皿が無く [UR-013](../10_URD.md#ur-013) → SRS のトレースが切れていた。
この欠落が [FR-018](../20_SRS.md#fr-018-per-mesh-ピーク候補-geojson-統合) のループ内再入設計を何度も揺れさせた原因である（[ADR-SRS-024](ADR-SRS-024-fr018-loop-reentry-and-peak-filter.md)）。

[UR-013](../10_URD.md#ur-013) が要求する内容は性質の異なる2つの要求に分解できる:

1. **観測可能性**: 中間成果物（per-mesh / 統合 GeoJSON）を生成されたタイミングで物理ファイルとして
   出力し、手動で地図に乗せて位置・形状の妥当性を目視確認できる（横断的な品質特性）
2. **ピーク↔コル対応の可視化**: コル位置を地図上で確認できる（具体的な出力内容。
   現行 GeoJSON はコル座標を持たない。[ADR-SRS-022](ADR-SRS-022-per-mesh-geojson-property-design.md) の join 方式）

① の出力実体は既に SRS に存在する:

- [FR-016](../20_SRS.md#fr-016-ピーク域ポリゴン生成) が per-mesh ピーク候補 GeoJSON を物理ファイルとして出力する
- [FR-018](../20_SRS.md#fr-018-per-mesh-ピーク候補-geojson-統合) が統合ピーク候補 GeoJSON `merged_peak.geojson` を世代ごとに上書き再生成する
- `merged_peak.geojson` はデバッグ・差分検査用に物理出力を残す仕様になっている

足りていたのは「なぜこれを物理出力するか・ループ内再入が必要か」を裏付ける根拠（URD/SRS レベルの要件）だけであった。

② は別途設計検討が必要（ADR-SRS-022 の見直しを伴う可能性がある。[ADR-SRS-026](ADR-SRS-026-intermediate-geojson-peak-col-visualization.md) で対応済み）。

## Decision

[UR-013](../10_URD.md#ur-013) を以下の方針で SRS に反映する:

**(a) 観測可能性（①）は [NFR-009](../20_SRS.md#nfr-009-観測可能性中間成果物の可視化) として受ける**

[NFR-009](../20_SRS.md#nfr-009-観測可能性中間成果物の可視化)「観測可能性（中間成果物の可視化）」を新設（`対応 UR: UR-013`）し、
[FR-016](../20_SRS.md#fr-016-ピーク域ポリゴン生成)/[FR-018](../20_SRS.md#fr-018-per-mesh-ピーク候補-geojson-統合) がその実現手段であることをトレースする。
NFR として扱う根拠: ① は「特定のフィーチャを追加する」ではなく、「既存出力を生成タイミングで
物理ファイルとして残す」という横断的品質特性（観測可能性）であるため、FR よりも NFR が自然。

**(b) 新規 FR は起こさない**

①の出力実体（per-mesh GeoJSON・merged_peak.geojson の物理出力）は既存 FR で充足済み。
機能を追加する必要はなく、根拠付けと相互リンクのみで対応できる。

- [FR-018](../20_SRS.md#fr-018-per-mesh-ピーク候補-geojson-統合) の「再入可能性」説明に [NFR-009](../20_SRS.md#nfr-009-観測可能性中間成果物の可視化) / [UR-013](../10_URD.md#ur-013) への参照を追加

**(c) ピーク↔コル対応の可視化（②）はスコープ分割し [ADR-SRS-026](ADR-SRS-026-intermediate-geojson-peak-col-visualization.md) で対応**

② は ADR-SRS-022 の join 方式（GeoJSON にコル座標を持たない設計）と緊張する設計判断を含む。
本 ADR のスコープ外とし、実データ検証を経た上で別セッションで方式決定する。

## Alternatives

**1. [UR-013](../10_URD.md#ur-013) を FR として受ける案（却下）**

「中間成果物を可視化できること」を新規 FR（例: [FR-023](../20_SRS.md#fr-023-解析パイプライン制御)）として起こす案。
しかし出力実体は既存 FR で充足済みであり、FR を追加しても実装は変わらない。
「根拠付けのためだけに FR を起こす」のは SRS の肥大化を招くため却下。

**2. ピーク↔コル対応を同時解決する案（却下）**

① と ② を本 ADR で一括して扱い、コル情報付与方式まで決定する案。
② は ADR-SRS-022 の join 方式見直しの可能性を含む重い設計判断であり、実データで
「コルが見えないと実際どこまで困るか」を確認してから決めるべき。
リスクが高い判断を急ぐと ADR-SRS-022 の手戻りが発生するため却下。

**3. 専用の「中間可視化コンポーネント」FR を新設する案（却下）**

per-mesh GeoJSON・merged_peak.geojson を一括で扱う専用 FR を新設する案。
現状の物理出力は既存 FR の副産物として自然に生成されており、専用コンポーネントを
導入する実装上の必要性がない。過剰設計のため却下。

## Consequences

- [UR-013](../10_URD.md#ur-013) → [NFR-009](../20_SRS.md#nfr-009-観測可能性中間成果物の可視化) → [FR-016](../20_SRS.md#fr-016-ピーク域ポリゴン生成)/[FR-018](../20_SRS.md#fr-018-per-mesh-ピーク候補-geojson-統合) のトレースが SRS 内で閉じる
- [FR-018](../20_SRS.md#fr-018-per-mesh-ピーク候補-geojson-統合) のループ内再入設計（[ADR-SRS-024](ADR-SRS-024-fr018-loop-reentry-and-peak-filter.md)）の根拠が URD/SRS 両レベルで確立する
- [NFR-009](../20_SRS.md#nfr-009-観測可能性中間成果物の可視化) の保証範囲外（コル座標の地図表示）は [ADR-SRS-026](ADR-SRS-026-intermediate-geojson-peak-col-visualization.md) として明示し、スコープの曖昧さを解消する
- ② の設計が固まった時点で本 ADR を更新するか、新規 ADR-SRS-026 で補完する
