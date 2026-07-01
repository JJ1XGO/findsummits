#!/usr/bin/env python3
"""docs/ の自作 lint チェッカー（PyMarkdown 補完）

検査A: 相対リンクの実在チェック
検査B: 太字ラベル直前の空行欠落（レイアウト崩れ）検出
検査C: 内部トラッカーID（ISSUE-NNN/BUG-NNN）および mgmt/ パス参照の混入検出
検査D: FR/UR/NFR 裸参照（リンクでもコード表記でもない参照）の検出
"""
import os
import re
import sys
from pathlib import Path

# 検査Bで対象とする太字ラベル単独行（入力/出力/説明 等）
BOLD_SECTION_RE = re.compile(
    r'^\s*\*\*(?:入力|出力|説明|前提条件|処理フロー|処理)\*\*:\s*$'
)

# 相対リンクのパターン（http/https は除外、.md 終端）
LINK_RE = re.compile(r'\]\(([^)#\s]+\.md)(?:#[^)]*)?\)')

# 検査C: 内部トラッカーID（コードブロック内は除外対象外のためシンプルに全行検査）
TRACKER_ID_RE = re.compile(r'\b(?:ISSUE|BUG)-\d+\b')

# 検査C: mgmt/ への具体的パス参照（リンク形式・コードスパン形式・裸表記のいずれも対象）。
# 「mgmt/」の後にパスセグメントが続く場合のみ検出し、`mgmt/` 単体の言及（ルール説明文等）は除外する。
# インラインコード除去前の生の行に対して検査する（コードスパン内の参照を拾うため）。
MGMT_PATH_RE = re.compile(r'mgmt/[^\s`)\]]+')

# 検査D: 裸参照パターン（FR/UR/NFR-NNN）
BARE_REF_RE = re.compile(r'\b((?:FR|UR|NFR)-\d+)\b')
# 検査D: トークン分割（既存リンク [TEXT](URL) とインラインコード `...` をスキップ対象に）
_TOKEN_RE = re.compile(r'\[[^\]]*\]\([^)]*\)|`[^`\n]+`')

# 検査D: 既知 ID キャッシュ（SRS の FR/NFR 見出し・URD の UR アンカーから収集）
_known_refs_cache = None

# 検査C・D 対象外ファイル（ルール自体の経緯を記録したメタドキュメント）。
# ADR-SRS-040 は「ISSUE-ID/mgmt/ 参照禁止ルール」の導入決定そのものを記録しており、
# 本文中でルール対象の概念（ISSUE-ID・mgmt/tracker/）に言及するのが趣旨のため対象外とする。
CHECK_CD_EXEMPT_FILES = {
    'ADR-SRS-040-remove-internal-tracker-id-from-docs.md',
}


def _get_known_refs():
    """SRS/URD に実際にアンカーが存在する ID セットを返す（遅延初期化）。

    削除済み ID や非標準表記（FR-6.11 等）は定義がないため自動的に除外される。
    """
    global _known_refs_cache
    if _known_refs_cache is not None:
        return _known_refs_cache
    docs_dir = Path(__file__).resolve().parent.parent / 'docs'
    known = set()
    srs = docs_dir / '20_SRS.md'
    urd = docs_dir / '10_URD.md'
    if srs.exists():
        with open(srs, encoding='utf-8') as f:
            for line in f:
                m = re.match(r'^#{1,6}\s+((FR|NFR)-\d+)', line)
                if m:
                    known.add(m.group(1))
    if urd.exists():
        with open(urd, encoding='utf-8') as f:
            for line in f:
                for anchor in re.findall(r'<a\s+id=["\']?(ur-\d+)["\']?', line, re.IGNORECASE):
                    known.add('UR-' + anchor.split('-')[1])
    _known_refs_cache = known
    return known


def _bare_refs_in_line(line):
    """行内の裸参照 ID を返す（リンク・インラインコード内を除く）。"""
    refs = []
    pos = 0
    for m in _TOKEN_RE.finditer(line):
        for rm in BARE_REF_RE.finditer(line[pos:m.start()]):
            refs.append(rm.group(1))
        pos = m.end()
    for rm in BARE_REF_RE.finditer(line[pos:]):
        refs.append(rm.group(1))
    return refs


def check_file(filepath):
    violations = []
    abs_filepath = os.path.abspath(filepath)
    base_dir = os.path.dirname(abs_filepath)
    # 検査C・D は docs/ 配下のみ適用（mgmt/ 等の内部管理ファイルは除外）。
    # ただし CHECK_CD_EXEMPT_FILES に該当するメタドキュメントは対象外。
    _apply_check_cd = (
        (os.sep + 'docs' + os.sep in abs_filepath or abs_filepath.endswith(os.sep + 'docs'))
        and os.path.basename(abs_filepath) not in CHECK_CD_EXEMPT_FILES
    )

    try:
        with open(filepath, encoding='utf-8') as f:
            lines = f.readlines()
    except (OSError, UnicodeDecodeError):
        return violations

    in_code_block = False

    for i, raw in enumerate(lines):
        line = raw.rstrip('\n')

        # フェンスコードブロックの追跡（検査D の除外判定に使用）
        if line.startswith('```'):
            in_code_block = not in_code_block

        # 検査A: 相対リンク実在チェック
        for m in LINK_RE.finditer(line):
            target = m.group(1)
            if target.startswith(('http://', 'https://')):
                continue
            abs_target = os.path.normpath(os.path.join(base_dir, target))
            if not os.path.exists(abs_target):
                violations.append(
                    f"{filepath}:{i + 1}: BROKEN-LINK: リンク先が存在しません: {target}"
                )

        # 検査B: 太字ラベル直前の空行欠落
        if BOLD_SECTION_RE.match(line):
            if i >= 1 and lines[i - 1].rstrip('\n').strip() != '':
                violations.append(
                    f"{filepath}:{i + 1}: LAYOUT-BREAK: "
                    f"太字ラベル「{line.strip()}」の直前が空行ではありません"
                )

        if _apply_check_cd:
            line_no_inline_code = re.sub(r'`[^`\n]+`', '', line)

            # 検査C: インラインコードを除いた行で検査
            for m in TRACKER_ID_RE.finditer(line_no_inline_code):
                violations.append(
                    f"{filepath}:{i + 1}: TRACKER-ID: "
                    f"内部トラッカーID「{m.group()}」は docs に書かないでください"
                    f"（経緯は日付・ADR リンクで残す。根拠: ADR-SRS-040）"
                )

            # 検査C: mgmt/ パス参照はコードスパン内も対象のため、生の行で検査する
            for m in MGMT_PATH_RE.finditer(line):
                violations.append(
                    f"{filepath}:{i + 1}: MGMT-PATH: "
                    f"mgmt/ への参照「{m.group()}」は docs に書かないでください"
                    f"（main ブランチで参照不能。根拠: docs/CLAUDE.md）"
                )

            # 検査D: コードブロック内・見出し行を除く本文中の裸参照を検出
            # 既知 ID のみ対象（削除済み ID・非標準表記は除外）
            if not in_code_block and not line.startswith('#'):
                known = _get_known_refs()
                for ref_id in _bare_refs_in_line(line):
                    if ref_id in known:
                        violations.append(
                            f"{filepath}:{i + 1}: BARE-REF: "
                            f"「{ref_id}」はリンク化してください（docs/CLAUDE.md L108）"
                        )

    return violations


def collect_md_files(paths):
    md_files = []
    for path in paths:
        if os.path.isfile(path) and path.endswith('.md'):
            md_files.append(path)
        elif os.path.isdir(path):
            for root, _, files in os.walk(path):
                for f in sorted(files):
                    if f.endswith('.md'):
                        md_files.append(os.path.join(root, f))
    return md_files


def main():
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <path> [path...]", file=sys.stderr)
        sys.exit(1)

    files = collect_md_files(sys.argv[1:])
    all_violations = []
    for f in files:
        all_violations.extend(check_file(f))

    for v in all_violations:
        print(v)

    sys.exit(1 if all_violations else 0)


if __name__ == '__main__':
    main()
