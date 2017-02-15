CC=cc
CFLAGS=-O2 -Wall -g
LDFLAGS=
FILES=smash.h lex.c parser.c string.c util.c vector.c

.PHONY: test all clean

all: smash

test: lex parser

lex: smash.h lex.c string.c util.c
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o lex -DTEST_LEX

parser: smash.h lex.c parser.c string.c util.c vector.c
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o parser -DTEST_PARSER

clean:
	rm -f lex parser

