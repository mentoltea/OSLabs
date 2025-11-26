CFLAGS = -Wall -Wextra -ggdb

.PHONY: all clean

all: writer reader

writer: writer.c common.h
	gcc $(CFLAGS) writer.c -o writer

reader: reader.c common.h
	gcc $(CFLAGS) reader.c -o reader

clean:
	rm writer reader