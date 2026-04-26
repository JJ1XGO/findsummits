#!/bin/bash
# run_all.sh - 全メッシュを一括解析するラッパースクリプト
#
# 使い方:
#   ./run_all.sh                           # デフォルト設定で全メッシュ処理
#   ./run_all.sh params/mesh_list_japan.txt

MESH_LIST="${1:-params/mesh_list_japan.txt}"
FINDSUMMITS="./build/findsummits"

# params/config.ini から DATA_DIR を読む
if [ -f params/config.ini ]; then
    _DATA_DIR=$(grep -i '^DATA_DIR\s*=' params/config.ini | head -1 | cut -d= -f2- | tr -d ' \r')
fi
DATA_DIR="${_DATA_DIR:-/data}"
LOG_DIR="$DATA_DIR/logs"

if [ ! -f "$MESH_LIST" ]; then
    echo "メッシュリストが見つかりません: $MESH_LIST" >&2
    exit 1
fi

if [ ! -x "$FINDSUMMITS" ]; then
    echo "findsummits バイナリが見つかりません。make を実行してください。" >&2
    exit 1
fi

mkdir -p "$LOG_DIR"

LOG_FILE="$LOG_DIR/findsummits_$(date +%Y%m%d_%H%M%S).log"

echo "=== 全メッシュ解析開始 ==="
echo "リスト: $MESH_LIST"
echo "DATA_DIR: $DATA_DIR"
echo "開始時刻: $(date)"
echo "ログ: $LOG_FILE"
echo ""

# リスト全体を1プロセスに渡す（隣接判定用MeshSetが正しく構築される）
"$FINDSUMMITS" "$MESH_LIST" 2>&1 | tee "$LOG_FILE"
EXIT_CODE=${PIPESTATUS[0]}

echo ""
echo "=== 全メッシュ解析完了 ==="
echo "終了時刻: $(date)"

NG_COUNT=$(grep -c "解析失敗\|解析に失敗" "$LOG_FILE" 2>/dev/null || echo 0)
if [ "$NG_COUNT" -gt 0 ]; then
    echo "失敗メッシュ数: $NG_COUNT"
    grep "解析失敗\|解析に失敗" "$LOG_FILE"
fi

exit $EXIT_CODE
