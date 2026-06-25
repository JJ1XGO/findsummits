#!/usr/bin/env python3
"""docs/ の自作 lint チェッカー（PyMarkdown 補完）

検査A: 相対リンクの実在チェック
検査B: 太字ラベル直前の空行欠落（レイアウト崩れ）検出
"""
import os
import re
import sys

# 検査Bで対象とする太字ラベル単独行（入力/出力/説明 等）
BOLD_SECTION_RE = re.compile(
    r'^\s*\*\*(?:入力|出力|説明|前提条件|処理フロー|処理)\*\*:\s*$'
)

# 相対リンクのパターン（http/https は除外）
LINK_RE = re.compile(r'\]\(([^)#\s]+\.md)(?:#[^)]*)?\)')


def check_file(filepath):
    violations = []
    base_dir = os.path.dirname(os.path.abspath(filepath))

    try:
        with open(filepath, encoding='utf-8') as f:
            lines = f.readlines()
    except (OSError, UnicodeDecodeError):
        return violations

    for i, raw in enumerate(lines):
        line = raw.rstrip('\n')

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
