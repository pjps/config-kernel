
CFLAGS:=$(CFLAGS)

configk: configk.c configk.h tree.c lex.yy.c parser.tab.c \
	lex.ee.c eparse.tab.c lex.cc.c cparse.tab.c
	cc $(CFLAGS) -xc -o configk \
	 configk.c tree.c lex.yy.c parser.tab.c \
	 lex.ee.c eparse.tab.c \
	 lex.cc.c cparse.tab.c -ly

lex.yy.c: lexer.l parser.tab.c
	flex -F lexer.l

lex.ee.c: elexer.l eparse.tab.c
	flex -F elexer.l

lex.cc.c: clexer.l cparse.tab.c
	flex -F clexer.l

parser.tab.c: parser.y
	bison -d parser.y

eparse.tab.c: eparse.y
	bison -d eparse.y

cparse.tab.c: cparse.y
	bison -d cparse.y

clean:
	rm -f configk *.tab.[ch] lex.*.c *.o
