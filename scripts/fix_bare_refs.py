#!/usr/bin/env python3
"""docs/ 内の FR/UR/NFR 裸参照を Markdown リンクに一括変換する。

使用方法:
  python3 scripts/fix_bare_refs.py            # 本実行
  python3 scripts/fix_bare_refs.py --dry-run  # 変更プレビューのみ
"""

import argparse
import os
import re
from pathlib import Path

WORKSPACE = Path(__file__).resolve().parent.parent
DOCS_DIR = WORKSPACE / 'docs'
SRS_FILE = DOCS_DIR / '20_SRS.md'
URD_FILE = DOCS_DIR / '10_URD.md'

# ---- スラッグ生成 -------------------------------------------------------

def github_slug(heading_text):
    """GitHub Markdown のアンカースラッグを生成する。"""
    s = heading_text.lower()
    s = re.sub(r'[^\w\s-]', '', s, flags=re.UNICODE)
    s = re.sub(r'\s+', '-', s)
    s = re.sub(r'-+', '-', s)
    return s.strip('-')


# ---- 参照マップ構築 -------------------------------------------------------

def build_ref_map():
    """SRS/URD から {ID: anchor_without_hash} のマップを返す。"""
    ref_map = {}

    # SRS: `#### FR-NNN: タイトル` / `### NFR-NNN: タイトル`
    with open(SRS_FILE, encoding='utf-8') as f:
        for line in f:
            m = re.match(r'^#{1,6}\s+((FR|NFR)-\d+):?\s*', line.rstrip())
            if m:
                id_ = m.group(1)
                heading_text = line.rstrip().lstrip('#').strip()
                ref_map[id_] = github_slug(heading_text)

    # URD: `<a id="ur-NNN">` 形式の HTML アンカー
    with open(URD_FILE, encoding='utf-8') as f:
        for line in f:
            for anchor in re.findall(r'<a\s+id=["\']?(ur-\d+)["\']?', line, re.IGNORECASE):
                id_upper = 'UR-' + anchor.split('-')[1]
                ref_map[id_upper] = anchor  # e.g. 'ur-001'

    return ref_map


# ---- リンクプレフィックス決定 -------------------------------------------

def get_link_target(ref_id, source_file, ref_map):
    """参照IDとソースファイルから href 文字列（プレフィックス込み）を返す。"""
    id_type = ref_id.split('-')[0]  # 'FR', 'NFR', 'UR'
    target_file = SRS_FILE if id_type in ('FR', 'NFR') else URD_FILE
    anchor = ref_map.get(ref_id)
    if anchor is None:
        return None

    source_file = Path(source_file).resolve()
    if source_file == target_file.resolve():
        # 同一ファイル内アンカー
        return '#' + anchor

    rel_prefix = os.path.relpath(target_file, source_file.parent)
    return rel_prefix + '#' + anchor


# ---- トークン分割 ---------------------------------------------------------

_TOKEN_RE = re.compile(r'(\[[^\]]*\]\([^)]*\)|`[^`\n]+`)')


def tokenize_line(line):
    """行を (type, text) のリストに分割する。type は 'skip' または 'text'。"""
    tokens = []
    pos = 0
    for m in _TOKEN_RE.finditer(line):
        if pos < m.start():
            tokens.append(('text', line[pos:m.start()]))
        tokens.append(('skip', m.group()))
        pos = m.end()
    if pos < len(line):
        tokens.append(('text', line[pos:]))
    return tokens


# ---- スラッシュ連鎖の正規化 -----------------------------------------------

_SLASH_CHAIN_RE = re.compile(r'\b(FR|UR|NFR)-(\d+)(?:/\d+)+\b')


def _normalize_slash_chain(m):
    """FR-NNN/MMM → FR-NNN/FR-MMM に展開する。"""
    prefix_type = m.group(1)
    parts = m.group(0).split('/')
    result = []
    for p in parts:
        if re.match(r'^\d+$', p):
            result.append(prefix_type + '-' + p)
        else:
            result.append(p)
    return '/'.join(result)


# ---- テキストセグメント内の置換 -------------------------------------------

_BARE_REF_RE = re.compile(r'\b((?:FR|UR|NFR)-\d+)\b')


def replace_in_text(text, ref_map, source_file):
    """テキストセグメント内の裸参照をリンクに置換する。"""
    # スラッシュ連鎖を先に正規化
    text = _SLASH_CHAIN_RE.sub(_normalize_slash_chain, text)

    def repl(m):
        ref_id = m.group(1)
        href = get_link_target(ref_id, source_file, ref_map)
        if href is None:
            return m.group(0)  # 不明な ID はそのまま
        return f'[{ref_id}]({href})'

    return _BARE_REF_RE.sub(repl, text)


# ---- ファイル処理 ---------------------------------------------------------

def process_file(filepath, ref_map, dry_run=False):
    """ファイルを処理して変更があれば True を返す。"""
    filepath = Path(filepath)
    with open(filepath, encoding='utf-8') as f:
        lines = f.readlines()

    new_lines = []
    in_code_block = False
    changed = False

    for i, line in enumerate(lines):
        # フェンスコードブロックの追跡
        if re.match(r'^```', line):
            in_code_block = not in_code_block
            new_lines.append(line)
            continue

        if in_code_block or re.match(r'^#{1,6}\s', line):
            new_lines.append(line)
            continue

        # トークン分割して text 部分のみ置換
        tokens = tokenize_line(line.rstrip('\n'))
        new_parts = []
        for tok_type, tok_text in tokens:
            if tok_type == 'text':
                new_parts.append(replace_in_text(tok_text, ref_map, filepath))
            else:
                new_parts.append(tok_text)
        suffix = '\n' if line.endswith('\n') else ''
        new_line = ''.join(new_parts) + suffix

        if new_line != line:
            changed = True
            if dry_run:
                print(f"  L{i + 1:4d} - {line.rstrip()}")
                print(f"       + {new_line.rstrip()}")

        new_lines.append(new_line)

    if changed and not dry_run:
        with open(filepath, 'w', encoding='utf-8') as f:
            f.writelines(new_lines)

    return changed


# ---- エントリポイント -----------------------------------------------------

def collect_docs_files():
    """処理対象の .md ファイルを収集する（archive 除外）。"""
    files = []
    for path in sorted(DOCS_DIR.rglob('*.md')):
        if 'archive' in path.parts:
            continue
        files.append(path)
    return files


def main():
    parser = argparse.ArgumentParser(description='docs/ 内の FR/UR/NFR 裸参照をリンク化する')
    parser.add_argument('--dry-run', action='store_true', help='変更せずプレビューのみ')
    parser.add_argument('--sample', type=int, default=0,
                        help='dry-run 時に最初の N 件のみ表示する')
    args = parser.parse_args()

    ref_map = build_ref_map()
    print(f"参照マップ: {len(ref_map)} 件")

    files = collect_docs_files()
    print(f"処理対象: {len(files)} ファイル\n")

    changed_files = []
    sample_shown = 0
    for filepath in files:
        if args.dry_run and args.sample and sample_shown >= args.sample:
            break
        rel = filepath.relative_to(WORKSPACE)
        changed = process_file(filepath, ref_map, dry_run=args.dry_run)
        if changed:
            changed_files.append(rel)
            if args.dry_run:
                print(f"[{rel}]")
                sample_shown += 1

    mode = '[dry-run] ' if args.dry_run else ''
    print(f"\n{mode}変更ファイル数: {len(changed_files)}/{len(files)}")
    if not args.dry_run:
        for f in changed_files:
            print(f"  更新: {f}")


if __name__ == '__main__':
    main()
