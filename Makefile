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

.PHONY: all clean findsummits test_mesh_analyze test_analyze
