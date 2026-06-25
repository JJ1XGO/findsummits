CC = gcc
CFLAGS = -O2 -Wall -pthread -I./src
LIBS = -lpng -lm
BUILDDIR = build

# 本番用ソース（main.c を除くコアファイル）
CORE_SRCS = $(filter-out src/main.c, $(wildcard src/*.c))
CORE_OBJS = $(CORE_SRCS:src/%.c=$(BUILDDIR)/%.o)

# 本番用メイン
MAIN_OBJS = $(BUILDDIR)/main.o

all: $(BUILDDIR)/findsummits

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

$(BUILDDIR)/findsummits: $(CORE_OBJS) $(MAIN_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

# テスト用ターゲット
$(BUILDDIR)/test_mesh_analyze: $(BUILDDIR)/test_mesh_analyze.o $(CORE_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

$(BUILDDIR)/test_analyze: $(BUILDDIR)/test_analyze.o $(CORE_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

$(BUILDDIR)/%.o: src/%.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR)/test_%.o: tests/test_%.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

# 短縮エイリアス
findsummits: $(BUILDDIR)/findsummits
test_mesh_analyze: $(BUILDDIR)/test_mesh_analyze
test_analyze: $(BUILDDIR)/test_analyze
clean:
	rm -rf $(BUILDDIR)

# Python 仮想環境のセットアップ
venv: venv/.installed

venv/.installed: requirements.txt
	python3 -m venv venv
	venv/bin/pip install --upgrade pip
	venv/bin/pip install -r requirements.txt
	@touch venv/.installed

# venv をクリーン再構築（孤立パッケージを除去し requirements.txt と完全一致させる）
# 定期実行・requirements.txt から外したパッケージの除去に使う
venv-rebuild:
	rm -rf venv
	$(MAKE) venv

# 機械的チェックの集約エントリ。ツール追加時はここに依存を足す（例: lint: lint-md lint-c lint-py）
lint: lint-md lint-py

# Markdown lint（チェックのみ・ファイルは書き換えない）。
# 既定対象: git 管理下の全 .md（mgmt/archive/ は凍結スナップショットのため除外）。
# LINT_MD_PATHS を指定した場合はそのパスを再帰走査する（override）。
LINT_MD_PATHS ?=
lint-md: venv
	@if [ -n "$(LINT_MD_PATHS)" ]; then targets="$(LINT_MD_PATHS)"; ropt="-r"; \
	else targets=$$(git ls-files '*.md' ':!:mgmt/archive/**'); ropt=""; fi; \
	venv/bin/python3 -m pymarkdown -c .pymarkdown scan $$ropt $$targets; s1=$$?; \
	venv/bin/python3 scripts/lint_docs.py $$targets; s2=$$?; \
	exit $$([ $$s1 -ge $$s2 ] && echo $$s1 || echo $$s2)

# Python lint（チェックのみ・ファイルは書き換えない）。
# 既定対象: git 管理下の全 .py（mgmt/archive/ は凍結スナップショットのため除外）。
# LINT_PY_PATHS を指定した場合はそのパスを対象にする（override）。
LINT_PY_PATHS ?=
lint-py: venv
	@if [ -n "$(LINT_PY_PATHS)" ]; then targets="$(LINT_PY_PATHS)"; \
	else targets=$$(git ls-files '*.py' ':!:mgmt/archive/**'); fi; \
	venv/bin/python3 -m ruff check $$targets

.PHONY: all clean findsummits test_mesh_analyze test_analyze venv venv-rebuild lint-md lint-py
