CFLAGS=-Wall -Wextra -ggdb

all: mycat mygrep

.PHONY: clear

mycat: mycat.c
	gcc $(CFLAGS) mycat.c -o mycat

mygrep: mygrep.c
	gcc $(CFLAGS) mygrep.c -o mygrep

clear:
	rm mycat mygrep
	rm mycat.exe mygrep.exe