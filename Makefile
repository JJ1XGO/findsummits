CC = gcc
CFLAGS = -O2 -Wall -pthread -I./src
LIBS = -lcurl -lpng -lm

# 本番用ソース（main.c を除くコアファイル）
CORE_SRCS = $(filter-out src/main.c, $(wildcard src/*.c))
CORE_OBJS = $(CORE_SRCS:src/%.c=%.o)

# 本番用メイン
MAIN_SRCS = src/main.c
MAIN_OBJS = main.o

all: findsummits

findsummits: $(CORE_OBJS) $(MAIN_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

# テスト用ターゲット
test_mesh_analyze: tests/test_mesh_analyze.o $(CORE_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

test_analyze: tests/test_analyze.o $(CORE_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

test_fetch: tests/test_fetch.o $(CORE_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

tests/%.o: tests/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o findsummits test_mesh_analyze test_analyze test_fetch test_elevation test_mesh test_unionfind

.PHONY: all clean
