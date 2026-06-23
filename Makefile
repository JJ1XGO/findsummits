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

# Markdown lint（チェックのみ・ファイルは書き換えない）。対象は LINT_MD_PATHS（既定: docs/）
# -r でサブディレクトリ（docs/decisions/ の ADR 等）まで再帰的に走査する
LINT_MD_PATHS ?= docs/
lint-md: venv
	venv/bin/python3 -m pymarkdown -c .pymarkdown scan -r $(LINT_MD_PATHS)

.PHONY: all clean findsummits test_mesh_analyze test_analyze venv venv-rebuild lint-md
