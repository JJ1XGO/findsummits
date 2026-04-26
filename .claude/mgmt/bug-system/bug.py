#!/usr/bin/env python3
"""
bug.py - バグ管理CLI
Claude Codeから自然言語で操作するためのスクリプト

使い方:
  python3 bug.py list                          # 一覧表示
  python3 bug.py list --status 未対応          # ステータスで絞り込み
  python3 bug.py list --severity 高            # 重大度で絞り込み
  python3 bug.py show BUG-001                 # 詳細表示
  python3 bug.py add                          # 対話形式で追加
  python3 bug.py add --title "..." --severity 高 --category 解析エンジン
  python3 bug.py update BUG-001 --status 解決済 --comment "修正完了"
  python3 bug.py close BUG-001 --comment "本番確認OK"
  python3 bug.py verify BUG-001 --comment "動作確認OK"  # 解決確認日を記録
  python3 bug.py summary                      # 集計サマリー
  python3 bug.py export                       # Excel出力

フィールド名対応（JSON内部名 → 表示名）:
  id            → ID
  title         → タイトル
  description   → 説明
  stage         → 発生源ステージ  [URD/SRS/HLD/LLD/COD/UT/IT/ST/OPS]
  category      → カテゴリ
  found_in      → 発生プログラム
  cause         → 原因プログラム
  reporter      → 報告者
  found_date    → 発見日
  repro         → 再現性
  severity      → 重大度
  status        → ステータス
  assignee      → 担当者
  resolution    → 対応方針
  notes         → 備考
  resolved_date → 対応完了日
  verified_date → 解決確認日
  history       → 変更履歴
"""

import json
import argparse
import sys
import os
from datetime import datetime, date

DATA_FILE = os.path.join(os.path.dirname(__file__), "data", "bugs.json")

VALID_STATUSES   = ["未対応", "対応中", "対応完了", "解決済", "却下"]
VALID_SEVERITIES = ["高", "中", "低"]
VALID_CATEGORIES = ["解析エンジン", "merge.py", "GeoJSON出力", "prefetch", "設定", "その他"]
VALID_STAGES     = ["URD", "SRS", "HLD", "LLD", "COD", "UT", "IT", "ST", "OPS"]
# URD: ユーザー要求定義 / SRS: システム要件定義 / HLD: 基本設計 / LLD: 詳細設計
# COD: コーディング / UT: 単体テスト / IT: 結合テスト / ST: 総合テスト / OPS: 本番運用

SEVERITY_ORDER = {"高": 0, "中": 1, "低": 2}
STATUS_COLOR   = {
    "未対応":   "\033[91m",
    "対応中":   "\033[93m",
    "対応完了": "\033[96m",
    "解決済":   "\033[92m",
    "却下":     "\033[90m",
}
SEVERITY_COLOR = {
    "高": "\033[91m\033[1m", "中": "\033[93m", "低": "\033[92m",
}
RESET = "\033[0m"
BOLD  = "\033[1m"
DIM   = "\033[2m"

def load():
    with open(DATA_FILE, encoding="utf-8") as f:
        return json.load(f)

def save(data):
    data["meta"]["last_updated"] = date.today().isoformat()
    with open(DATA_FILE, "w", encoding="utf-8") as f:
        json.dump(data, f, ensure_ascii=False, indent=2)

def next_id(data):
    data["meta"]["counter"] += 1
    return f"BUG-{data['meta']['counter']:03d}"

def colored(text, color):
    return f"{color}{text}{RESET}"

def new_bug_dict(bug_id):
    """バグの空テンプレートをXLSX列順で返す"""
    return {
        "id":            bug_id,
        "title":         None,
        "description":   None,
        "reporter":      None,
        "found_date":    date.today().isoformat(),
        "category":      None,
        "found_in":      None,
        "repro":         None,
        "severity":      "中",
        "status":        "未対応",
        "assignee":      None,
        "stage":         None,
        "cause":         None,
        "resolution":    None,
        "notes":         None,
        "resolved_date": None,
        "verified_date": None,
        "history":       [],
    }

def print_bug_row(bug):
    sid    = colored(bug["id"], BOLD)
    sstat  = colored(f"{bug['status']:<5}", STATUS_COLOR.get(bug["status"], ""))
    ssev   = colored(f"{bug['severity']:<3}", SEVERITY_COLOR.get(bug["severity"], ""))
    title  = bug["title"][:42].ljust(42)
    assign = (bug.get("assignee") or "-")[:8].ljust(8)
    print(f"  {sid}  {sstat}  {ssev}  {title}  {assign}")

def cmd_list(args):
    data = load()
    bugs = data["bugs"]

    if args.status:
        bugs = [b for b in bugs if b["status"] == args.status]
    if args.severity:
        bugs = [b for b in bugs if b["severity"] == args.severity]
    if args.assignee:
        bugs = [b for b in bugs if b.get("assignee") == args.assignee]
    if args.category:
        bugs = [b for b in bugs if b.get("category") == args.category]
    if args.open:
        bugs = [b for b in bugs if b["status"] not in ("解決済", "却下")]

    bugs = sorted(bugs, key=lambda b: (SEVERITY_ORDER.get(b["severity"], 9), b["id"]))

    if not bugs:
        print("該当するバグはありません。")
        return

    print(f"\n  {BOLD}{'ID':<9}  {'状態':<7}  {'重大度':<5}  {'タイトル':<42}  {'担当者':<8}{RESET}")
    print("  " + "-" * 80)
    for b in bugs:
        print_bug_row(b)
    print(f"\n  {DIM}合計: {len(bugs)} 件{RESET}\n")

def cmd_show(args):
    data = load()
    bug  = next((b for b in data["bugs"] if b["id"] == args.id), None)
    if not bug:
        print(f"エラー: {args.id} が見つかりません")
        sys.exit(1)

    print(f"\n{'='*55}")
    print(f"  {BOLD}{bug['id']}  {bug['title']}{RESET}")
    print(f"{'='*55}")
    print(f"\n  {BOLD}説明:{RESET}")
    print(f"    {bug.get('description') or '-'}")
    print(f"\n  報告者        : {bug.get('reporter') or '-'}")
    print(f"  発見日        : {bug.get('found_date') or '-'}")
    print(f"  カテゴリ      : {bug.get('category') or '-'}")
    print(f"  発生プログラム: {bug.get('found_in') or '-'}")
    print(f"  再現性        : {bug.get('repro') or '-'}")
    print(f"  重大度        : {colored(bug['severity'], SEVERITY_COLOR.get(bug['severity'], ''))}")
    print(f"  ステータス    : {colored(bug['status'], STATUS_COLOR.get(bug['status'], ''))}")
    print(f"  担当者        : {bug.get('assignee') or '-'}")
    print(f"  発生源ステージ: {bug.get('stage') or '-'}")
    print(f"  原因プログラム: {bug.get('cause') or '-'}")
    if bug.get("resolution"):
        print(f"\n  {BOLD}対応方針:{RESET}")
        print(f"    {bug['resolution']}")
    if bug.get("notes"):
        print(f"\n  {BOLD}備考:{RESET}")
        print(f"    {bug['notes']}")
    print(f"\n  対応完了日    : {bug.get('resolved_date') or '-'}")
    print(f"  解決確認日    : {bug.get('verified_date') or '-'}")

    history = bug.get("history", [])
    if history:
        print(f"\n  {BOLD}変更履歴:{RESET}")
        for h in history:
            print(f"    {DIM}{h['date'][:10]}{RESET}  {h['actor']}  {h['from']} → {h['to']}")
            if h.get("comment"):
                print(f"             {DIM}{h['comment']}{RESET}")
    print()

def cmd_add(args):
    data   = load()
    bug_id = next_id(data)

    if args.title:
        bug = new_bug_dict(bug_id)
        bug.update({
            "title":       args.title,
            "status":      args.status    or "未対応",
            "severity":    args.severity  or "中",
            "assignee":    args.assignee,
            "reporter":    args.reporter,
            "stage":       args.stage,
            "category":    args.category,
            "found_in":    args.found_in,
            "cause":       args.cause,
            "repro":       args.repro,
            "description": args.description,
            "resolution":  args.resolution,
            "notes":       args.notes,
        })
        data["bugs"].append(bug)
        save(data)
        print(f"\n  ✅ {colored(bug_id, BOLD)} を登録しました: {args.title}\n")
        return

    print(f"\n{BOLD}新規バグを登録します{RESET}\n")

    def ask(prompt, required=True, choices=None, default=None):
        hint = f" [{'/'.join(choices)}]" if choices else ""
        hint += f" (default: {default})" if default else ""
        while True:
            val = input(f"  {prompt}{hint}: ").strip()
            if not val and default:
                return default
            if not val and required:
                print("  ※ 必須項目です")
                continue
            if choices and val not in choices:
                print(f"  ※ 次のいずれかを入力してください: {', '.join(choices)}")
                continue
            return val or None

    print(f"  ID: {colored(bug_id, BOLD)}\n")

    bug = new_bug_dict(bug_id)
    bug.update({
        "title":       ask("タイトル"),
        "description": ask("説明",           required=False),
        "reporter":    ask("報告者",         required=False),
        "category":    ask("カテゴリ",       choices=VALID_CATEGORIES, required=False),
        "found_in":    ask("発生プログラム", required=False),
        "repro":       ask("再現性",         required=False),
        "severity":    ask("重大度",         choices=VALID_SEVERITIES, default="中"),
        "status":      ask("ステータス",     choices=VALID_STATUSES,   default="未対応"),
        "assignee":    ask("担当者",         required=False),
        "stage":       ask("発生源ステージ", choices=VALID_STAGES,     required=False),
        "cause":       ask("原因プログラム", required=False),
        "resolution":  ask("対応方針",       required=False),
        "notes":       ask("備考",           required=False),
    })

    data["bugs"].append(bug)
    save(data)
    print(f"\n  ✅ {colored(bug_id, BOLD)} を登録しました\n")

def cmd_update(args):
    data = load()
    bug  = next((b for b in data["bugs"] if b["id"] == args.id), None)
    if not bug:
        print(f"エラー: {args.id} が見つかりません")
        sys.exit(1)

    changed = []

    if args.status and args.status != bug["status"]:
        entry = {
            "date":    datetime.now().isoformat(timespec="seconds"),
            "actor":   args.actor or "不明",
            "from":    bug["status"],
            "to":      args.status,
            "comment": args.comment or "",
        }
        bug["history"].append(entry)
        old = bug["status"]
        bug["status"] = args.status
        changed.append(f"ステータス: {old} → {args.status}")

        if args.status == "対応完了" and not bug.get("resolved_date"):
            bug["resolved_date"] = date.today().isoformat()

    for field, val in [
        ("severity",   args.severity),
        ("assignee",   args.assignee),
        ("stage",      args.stage),
        ("category",   args.category),
        ("found_in",   args.found_in),
        ("cause",      args.cause),
        ("repro",      args.repro),
        ("description",args.description),
        ("resolution", args.resolution),
        ("notes",      args.notes),
    ]:
        if val is not None:
            changed.append(f"{field}: {bug.get(field)} → {val}")
            bug[field] = val

    if not changed:
        print("変更はありませんでした。")
        return

    save(data)
    print(f"\n  ✅ {args.id} を更新しました:")
    for c in changed:
        print(f"     • {c}")
    print()

def cmd_close(args):
    """ショートカット: ステータスを対応完了にして対応完了日を記録（Claudeが修正完了時に使う）"""
    class _A:
        pass
    a = _A()
    a.id = args.id
    a.status = "対応完了"
    a.actor = args.actor
    a.comment = args.comment
    a.severity = a.assignee = a.stage = a.category = None
    a.found_in = a.cause = a.repro = a.description = a.resolution = a.notes = None
    cmd_update(a)

def cmd_verify(args):
    """ユーザーが解決を確認：解決確認日を記録してステータスを解決済にする"""
    data = load()
    bug  = next((b for b in data["bugs"] if b["id"] == args.id), None)
    if not bug:
        print(f"エラー: {args.id} が見つかりません")
        sys.exit(1)

    today = date.today().isoformat()
    bug["verified_date"] = today
    entry = {
        "date":    datetime.now().isoformat(timespec="seconds"),
        "actor":   args.actor or "不明",
        "from":    bug["status"],
        "to":      "解決済",
        "comment": f"[解決確認] {args.comment or ''}".strip(),
    }
    bug["history"].append(entry)
    bug["status"] = "解決済"
    save(data)
    print(f"\n  ✅ {args.id} を解決済にしました（解決確認日: {today}）\n")

def cmd_summary(args):
    data  = load()
    bugs  = data["bugs"]
    total = len(bugs)

    print(f"\n  {BOLD}{'─'*40}")
    print(f"  バグ管理サマリー  (合計 {total} 件)")
    print(f"  {'─'*40}{RESET}")

    print(f"\n  {BOLD}ステータス別{RESET}")
    for s in VALID_STATUSES:
        n   = sum(1 for b in bugs if b["status"] == s)
        bar = "█" * n
        col = STATUS_COLOR.get(s, "")
        print(f"  {colored(s.ljust(6), col)}  {colored(bar.ljust(20), col)}  {n}")

    open_bugs = [b for b in bugs if b["status"] not in ("解決済", "却下")]
    print(f"\n  {BOLD}重大度別 (未解決){RESET}")
    for sv in VALID_SEVERITIES:
        n   = sum(1 for b in open_bugs if b["severity"] == sv)
        bar = "█" * n
        col = SEVERITY_COLOR.get(sv, "")
        print(f"  {colored(sv.ljust(3), col)}  {colored(bar.ljust(20), col)}  {n}")

    resolved = sum(1 for b in bugs if b["status"] == "解決済")
    rate     = resolved / total * 100 if total else 0
    print(f"\n  {BOLD}KPI{RESET}")
    print(f"  未解決件数  : {len(open_bugs)}")
    print(f"  重大度「高」: {colored(str(sum(1 for b in open_bugs if b['severity'] == '高')), SEVERITY_COLOR['高'])}")
    print(f"  解決率      : {resolved}/{total} ({rate:.1f}%)")
    print()

def cmd_export(args):
    """bugs.jsonをExcelに書き出す（バグ一覧・サマリー・欠陥収束シート）"""
    try:
        from openpyxl import Workbook
        from openpyxl.styles import Font, PatternFill, Alignment, Border, Side
        from openpyxl.utils import get_column_letter
        from openpyxl.chart import ScatterChart, Series, Reference
        from datetime import timedelta
    except ImportError:
        print("openpyxlが必要です: pip install openpyxl")
        sys.exit(1)

    data = load()
    bugs = data["bugs"]
    wb   = Workbook()

    thin   = Side(style='thin', color='BFBFBF')
    border = Border(left=thin, right=thin, top=thin, bottom=thin)

    def hdr_cell(ws, row, col, value, width=None):
        c = ws.cell(row=row, column=col, value=value)
        c.font      = Font(bold=True, color="FFFFFF", name="Arial", size=10)
        c.fill      = PatternFill("solid", fgColor="1F4E79")
        c.alignment = Alignment(horizontal="center", vertical="center")
        c.border    = border
        if width:
            ws.column_dimensions[get_column_letter(col)].width = width
        return c

    def data_cell(ws, row, col, value, bg="FFFFFF", bold=False):
        c = ws.cell(row=row, column=col, value=value)
        c.font      = Font(name="Arial", size=9, bold=bold)
        c.fill      = PatternFill("solid", fgColor=bg)
        c.alignment = Alignment(horizontal="left", vertical="center", wrap_text=True)
        c.border    = border
        return c

    # ── シート1: バグ一覧 ──────────────────────────────────────
    ws1 = wb.active
    ws1.title = "バグ一覧"

    # (ラベル, 内部フィールド名, 列幅)
    columns = [
        ("ID",           "id",            10),
        ("タイトル",     "title",         40),
        ("説明",         "description",   44),
        ("報告者",       "reporter",      14),
        ("発見日",       "found_date",    14),
        ("カテゴリ",     "category",      16),
        ("発生プログラム","found_in",      20),
        ("再現性",       "repro",         10),
        ("重大度",       "severity",      10),
        ("ステータス",   "status",        12),
        ("担当者",       "assignee",      14),
        ("発生源ステージ","stage",         16),
        ("原因プログラム","cause",         20),
        ("対応方針",     "resolution",    30),
        ("備考",         "notes",         30),
        ("対応完了日",   "resolved_date", 14),
        ("解決確認日",   "verified_date", 14),
    ]

    ws1.row_dimensions[1].height = 28
    for i, (label, _, width) in enumerate(columns, 1):
        hdr_cell(ws1, 1, i, label, width=width)

    severity_bg = {"高": "FFE0E0", "中": "FFFBE0", "低": "E8F5E9"}
    sev_col = next(i for i, (_, f, _) in enumerate(columns, 1) if f == "severity")

    for ri, bug in enumerate(bugs, 2):
        bg = "F0F0F0" if bug["status"] in ("解決済", "却下") else ("EBF3FB" if ri % 2 == 0 else "FFFFFF")
        for ci, (_, field, _) in enumerate(columns, 1):
            val = bug.get(field) or ""
            cell_bg = severity_bg.get(val, bg) if ci == sev_col else bg
            data_cell(ws1, ri, ci, val, bg=cell_bg, bold=(ci == sev_col and val in severity_bg))
        ws1.row_dimensions[ri].height = 40

    ws1.freeze_panes = "B2"

    # ステータス・重大度・ステージ列にドロップダウンバリデーションを追加
    from openpyxl.worksheet.datavalidation import DataValidation
    last_row = len(bugs) + 1

    status_col   = next(i for i, (_, f, _) in enumerate(columns, 1) if f == "status")
    severity_col = next(i for i, (_, f, _) in enumerate(columns, 1) if f == "severity")
    stage_col    = next(i for i, (_, f, _) in enumerate(columns, 1) if f == "stage")

    def make_dv(choices, col_letter, last_row):
        dv = DataValidation(
            type="list",
            formula1=f'"{",".join(choices)}"',
            allow_blank=True,
            showDropDown=False,
        )
        dv.sqref = f"{col_letter}2:{col_letter}{last_row}"
        return dv

    ws1.add_data_validation(make_dv(VALID_STATUSES,   get_column_letter(status_col),   last_row))
    ws1.add_data_validation(make_dv(VALID_SEVERITIES, get_column_letter(severity_col), last_row))
    ws1.add_data_validation(make_dv(VALID_STAGES,     get_column_letter(stage_col),    last_row))

    # ── シート2: サマリー ──────────────────────────────────────
    ws2 = wb.create_sheet("サマリー")
    open_bugs = [b for b in bugs if b["status"] not in ("解決済", "却下")]
    resolved  = sum(1 for b in bugs if b["status"] == "解決済")
    total     = len(bugs)

    def section_header(ws, row, col, text, span=2):
        c = ws.cell(row=row, column=col, value=text)
        c.font      = Font(bold=True, color="FFFFFF", name="Arial", size=10)
        c.fill      = PatternFill("solid", fgColor="2E5D9E")
        c.alignment = Alignment(horizontal="left", vertical="center")
        c.border    = border
        for dc in range(1, span):
            nc = ws.cell(row=row, column=col + dc)
            nc.fill   = PatternFill("solid", fgColor="2E5D9E")
            nc.border = border

    def summary_row(ws, row, label, value, label_bg="F5F5F5", value_bg="FFFFFF"):
        lc = ws.cell(row=row, column=1, value=label)
        lc.font      = Font(name="Arial", size=9, bold=True)
        lc.fill      = PatternFill("solid", fgColor=label_bg)
        lc.alignment = Alignment(horizontal="left", vertical="center")
        lc.border    = border
        vc = ws.cell(row=row, column=2, value=value)
        vc.font      = Font(name="Arial", size=9)
        vc.fill      = PatternFill("solid", fgColor=value_bg)
        vc.alignment = Alignment(horizontal="center", vertical="center")
        vc.border    = border

    ws2.column_dimensions["A"].width = 22
    ws2.column_dimensions["B"].width = 12

    row = 1
    section_header(ws2, row, 1, "KPI"); row += 1
    summary_row(ws2, row, "合計件数",   total);          row += 1
    summary_row(ws2, row, "未解決件数", len(open_bugs)); row += 1
    summary_row(ws2, row, "解決済件数", resolved);       row += 1
    summary_row(ws2, row, "解決率", f"{resolved/total*100:.1f}%" if total else "-%"); row += 2

    section_header(ws2, row, 1, "ステータス別"); row += 1
    for s in VALID_STATUSES:
        summary_row(ws2, row, s, sum(1 for b in bugs if b["status"] == s)); row += 1
    row += 1

    section_header(ws2, row, 1, "重大度別（未解決）"); row += 1
    for sv in VALID_SEVERITIES:
        summary_row(ws2, row, sv, sum(1 for b in open_bugs if b["severity"] == sv)); row += 1
    row += 1

    section_header(ws2, row, 1, "カテゴリ別（未解決）"); row += 1
    for cat in VALID_CATEGORIES:
        n = sum(1 for b in open_bugs if b.get("category") == cat)
        if n:
            summary_row(ws2, row, cat, n); row += 1
    row += 1

    section_header(ws2, row, 1, "発生源ステージ別（未解決）"); row += 1
    for st in VALID_STAGES:
        n = sum(1 for b in open_bugs if b.get("stage") == st)
        if n:
            summary_row(ws2, row, st, n); row += 1

    # ── シート3: 欠陥収束 ──────────────────────────────────────
    ws3 = wb.create_sheet("欠陥収束")

    # 日付範囲を決定（found_dateの最小値〜今日）
    all_found = [b["found_date"] for b in bugs if b.get("found_date")]
    if all_found:
        from datetime import timedelta as td
        start = date.fromisoformat(min(all_found))
        end   = date.today()
        dates = []
        d = start
        while d <= end:
            dates.append(d)
            d += td(days=1)

        # ヘッダー行
        for ci, label in enumerate(["日付", "欠陥累計", "対応累計", "確認累計"], 1):
            c = ws3.cell(row=1, column=ci, value=label)
            c.font      = Font(bold=True, color="FFFFFF", name="Arial", size=10)
            c.fill      = PatternFill("solid", fgColor="1F4E79")
            c.alignment = Alignment(horizontal="center", vertical="center")
            c.border    = border
        ws3.column_dimensions["A"].width = 14
        ws3.column_dimensions["B"].width = 12
        ws3.column_dimensions["C"].width = 12
        ws3.column_dimensions["D"].width = 12

        # ExcelシリアルDate値（日付軸の数値）に変換
        def to_serial(d):
            return (d - date(1899, 12, 30)).days

        # 日付ごとの累積値を書き込む（A列はExcelシリアル値で格納し日付書式を適用）
        for ri, d in enumerate(dates, 2):
            ds = d.isoformat()
            found_n    = sum(1 for b in bugs if b.get("found_date")    and b["found_date"]    <= ds)
            resolved_n = sum(1 for b in bugs if b.get("resolved_date") and b["resolved_date"] <= ds)
            verified_n = sum(1 for b in bugs if b.get("verified_date") and b["verified_date"] <= ds)
            c = ws3.cell(row=ri, column=1, value=to_serial(d))
            c.number_format = "YYYY-MM-DD"
            ws3.cell(row=ri, column=2, value=found_n)
            ws3.cell(row=ri, column=3, value=resolved_n)
            ws3.cell(row=ri, column=4, value=verified_n)

        # ScatterChart（X軸を数値軸にすることで scaling.min が効く）
        chart = ScatterChart()
        chart.title  = "欠陥収束曲線"
        chart.style  = 10
        chart.width  = 20
        chart.height = 14
        chart.y_axis.title  = "累計数"
        chart.x_axis.title  = "日付"
        chart.y_axis.numFmt = "0"
        chart.x_axis.numFmt = "YYYY-MM-DD"
        chart.x_axis.scaling.min = to_serial(start)
        chart.x_axis.scaling.max = to_serial(end)
        chart.y_axis.scaling.min = 0

        xvals = Reference(ws3, min_col=1, min_row=2, max_row=len(dates) + 1)
        series_defs = [
            ("欠陥累計", 2, "FF0000"),
            ("対応累計", 3, "4472C4"),
            ("確認累計", 4, "70AD47"),
        ]
        for label, col, color in series_defs:
            yvals = Reference(ws3, min_col=col, min_row=2, max_row=len(dates) + 1)
            s = Series(yvals, xvals, title=label)
            s.graphicalProperties.line.solidFill = color
            s.graphicalProperties.line.width     = 20000  # 2pt
            s.marker.symbol = "circle"
            s.marker.size   = 5
            chart.series.append(s)

        ws3.add_chart(chart, "F2")

    out = args.output or os.path.join(os.path.dirname(__file__), "reports", "bugs_export.xlsx")
    os.makedirs(os.path.dirname(out), exist_ok=True)
    wb.save(out)
    print(f"\n  ✅ エクスポート完了: {out}\n")

def main():
    p = argparse.ArgumentParser(description="バグ管理CLI")
    sub = p.add_subparsers(dest="cmd")

    # list
    pl = sub.add_parser("list", help="一覧表示")
    pl.add_argument("--status",   help="ステータスで絞り込み")
    pl.add_argument("--severity", help="重大度で絞り込み")
    pl.add_argument("--assignee", help="担当者で絞り込み")
    pl.add_argument("--category", help="カテゴリで絞り込み")
    pl.add_argument("--open",     action="store_true", help="未解決のみ")

    # show
    ps = sub.add_parser("show", help="詳細表示")
    ps.add_argument("id", help="バグID (例: BUG-001)")

    # add
    pa = sub.add_parser("add", help="新規バグを登録 (--title 指定で非対話、省略で対話形式)")
    pa.add_argument("--title")
    pa.add_argument("--status",      choices=VALID_STATUSES)
    pa.add_argument("--severity",    choices=VALID_SEVERITIES)
    pa.add_argument("--stage",       choices=VALID_STAGES)
    pa.add_argument("--category",    choices=VALID_CATEGORIES)
    pa.add_argument("--found_in")
    pa.add_argument("--cause")
    pa.add_argument("--assignee")
    pa.add_argument("--reporter")
    pa.add_argument("--repro")
    pa.add_argument("--description")
    pa.add_argument("--resolution")
    pa.add_argument("--notes")

    # update
    pu = sub.add_parser("update", help="バグを更新")
    pu.add_argument("id")
    pu.add_argument("--status",    choices=VALID_STATUSES)
    pu.add_argument("--severity",  choices=VALID_SEVERITIES)
    pu.add_argument("--stage",     choices=VALID_STAGES)
    pu.add_argument("--category",  choices=VALID_CATEGORIES)
    pu.add_argument("--found_in")
    pu.add_argument("--cause")
    pu.add_argument("--repro")
    pu.add_argument("--assignee")
    pu.add_argument("--description")
    pu.add_argument("--resolution")
    pu.add_argument("--notes")
    pu.add_argument("--actor",   default="不明", help="変更者名")
    pu.add_argument("--comment", help="変更コメント")

    # close
    pc = sub.add_parser("close", help="解決済にして対応完了日を記録")
    pc.add_argument("id")
    pc.add_argument("--actor",   default="不明")
    pc.add_argument("--comment", help="解決コメント")

    # verify
    pv = sub.add_parser("verify", help="解決確認日を記録")
    pv.add_argument("id")
    pv.add_argument("--actor",   default="不明")
    pv.add_argument("--comment", help="確認コメント")

    # summary
    sub.add_parser("summary", help="集計サマリーを表示")

    # export
    pe = sub.add_parser("export", help="Excelファイルに出力")
    pe.add_argument("--output", help="出力ファイルパス")

    args = p.parse_args()
    if not args.cmd:
        p.print_help()
        return

    dispatch = {
        "list": cmd_list, "show": cmd_show, "add": cmd_add,
        "update": cmd_update, "close": cmd_close, "verify": cmd_verify,
        "summary": cmd_summary, "export": cmd_export,
    }
    dispatch[args.cmd](args)

if __name__ == "__main__":
    main()
