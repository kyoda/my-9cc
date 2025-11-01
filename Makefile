CFLAGS=-std=gnu99 -g -fno-common -Wall -Wno-switch -static
# wildcard関数で、testディレクトリの.cを取得
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

TEST_SRCS=$(wildcard test/*.c)
# .cを.oに置き換える
TEST_OBJS=$(TEST_SRCS:.c=.o)
TEST_AS=$(TEST_SRCS:.c=.s)

9cc: $(OBJS)
		$(CC) ${CFLAGS} -o 9cc $(OBJS) $(LDFLAGS)

$(OBJS): 9cc.h

test/%.o: 9cc test/%.c
# -E -P -C で、プリプロセスだけ処理
	$(CC) -o- -E -P -C test/$*.c | ./9cc -o test/$*.s -
# common は、%.cに含まれないように'.c'を外している
	$(CC) -o $@ test/$*.s -xc test/common

test: $(TEST_OBJS)
# $^ は、依存関係（$(TEST_OBJS)）の全てのファイルを表す
# $$i としているのは、Makefileの変数ではなく、シェルの変数として扱うため
	for i in $^; do echo $$i; ./$$i || exit 1; echo; done
	test/driver.sh

# ccのテスト用
CCTEST_OBJS=$(TEST_SRCS:.c=.out)

test/%.out: test/%.c
	$(CC) ${CFLAGS} -o test/$*.out test/$*.c -xc test/common

cctest: $(CCTEST_OBJS)
	for i in $^; do echo $$i; ./$$i || exit 1; echo; done
	test/driver.sh

clean:
	rm -f 9cc *.o *~ tmp* $(CCTEST_OBJS) $(TEST_OBJS) $(TEST_AS)


# -----------------------------
# セルフコンパイルテスト
# -----------------------------
self: 9cc
# 1. gcc で前処理だけする (-E)
	$(CC) -E -P $(SRCS) > 9cc_pre.i

# 2. 自作コンパイラでアセンブリ生成
	./9cc -o tmp.s 9cc_pre.i
	$(CC) -o 9cc-self tmp.s

# 3. 出来た 9cc-self でもう一度セルフビルド
	./9cc-self -o tmp2.s 9cc_pre.i
	$(CC) -o 9cc-self2 tmp2.s

	@echo "セルフコンパイル完了: my-9cc-self2 が生成されました"

.PHONY: test clean

# この書き方だと、すべての .o ファイルがすべての .c ファイルに依存することになる
# $(OBJS): $(SRCS)