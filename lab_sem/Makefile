CFLAGS = -Wall -Wextra

all: t1 program1 program2

t1: t1.c
	gcc $(CFLAGS) t1.c -o t1 -lphread

program1: program1.c common.h
	gcc $(CFLAGS) program1.c -o program1 -lphread

program2: program2.c common.h
	gcc $(CFLAGS) program2.c -o program2 -lphread