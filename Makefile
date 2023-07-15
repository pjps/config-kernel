
CFLAGS=-O2 -fstack-protector-all -Werror

configk: configk.c tree.c lex.yy.c parser.tab.c
	cc $(CFLAGS) -xc -o configk \
	 configk.c tree.c parser.tab.c lex.yy.c -lfl -ly

lex.yy.c: lexer.l
	flex -F lexer.l

parser.tab.c: parser.y
	bison -d parser.y

clean:
	rm -f configk *.tab.[ch] lex.yy.c *.o
