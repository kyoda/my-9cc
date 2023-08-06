CFLAGS=-std=c11 -g -static -Wall
# wildcard関数で、testディレクトリの.cを取得
SRCS=$(wildcard *.c)
OBJS=$(SRCS:.c=.o)

TEST_SRCS=$(wildcard test/*.c)
# .cを.oに置き換える
TEST_OBJS=$(TEST_SRCS:.c=.o)
TEST_AS=$(TEST_SRCS:.c=.s)

9cc: $(OBJS)
		$(CC) -o 9cc $(OBJS) $(LDFLAGS)

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

clean:
	rm -f 9cc *.o *~ tmp* $(TEST_OBJS) $(TEST_AS)

.PHONY: test clean

