#!/bin/bash
# run_all.sh - 176メッシュを並列処理するラッパースクリプト
#
# 使い方:
#   ./run_all.sh                    # デフォルト設定で全メッシュ処理
#   ./run_all.sh tasks/mesh_list_japan.txt
#
# 並列数: 8 (16コアの半分)

MESH_LIST="${1:-tasks/mesh_list_japan.txt}"
PARALLEL=8
FINDSUMMITS="./build/findsummits"
LOG_DIR="/mnt/findsummits/logs"

if [ ! -f "$MESH_LIST" ]; then
    echo "メッシュリストが見つかりません: $MESH_LIST" >&2
    exit 1
fi

if [ ! -x "$FINDSUMMITS" ]; then
    echo "findsummits バイナリが見つかりません。make を実行してください。" >&2
    exit 1
fi

mkdir -p "$LOG_DIR"

echo "=== 全メッシュ解析開始 ==="
echo "リスト: $MESH_LIST"
echo "並列数: $PARALLEL"
echo "開始時刻: $(date)"
echo ""

# メッシュコードを1行1コードとして読み込み、xargs で並列実行
# 各メッシュのログは /mnt/findsummits/logs/{meshcode}.log に保存
grep -v '^#' "$MESH_LIST" | grep -v '^[[:space:]]*$' | \
    xargs -P "$PARALLEL" -I{} sh -c \
        '"$0" {} > "$1/{}.log" 2>&1 && echo "[OK] {}  " || echo "[NG] {}  "' \
        "$FINDSUMMITS" "$LOG_DIR"

echo ""
echo "=== 全メッシュ解析完了 ==="
echo "終了時刻: $(date)"
echo "ログ: $LOG_DIR/"

# 失敗したメッシュを報告
NG_COUNT=$(ls "$LOG_DIR"/*.log 2>/dev/null | xargs grep -l "解析失敗\|解析に失敗" 2>/dev/null | wc -l)
if [ "$NG_COUNT" -gt 0 ]; then
    echo "失敗メッシュ数: $NG_COUNT"
    ls "$LOG_DIR"/*.log | xargs grep -l "解析失敗\|解析に失敗" 2>/dev/null | \
        sed 's|.*/||;s|\.log||' | sort
fi
