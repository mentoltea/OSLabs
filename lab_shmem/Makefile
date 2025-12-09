CFLAGS = -Wall -Wextra -ggdb

.PHONY: all clean

all: writer reader

writer: writer.c common.h
	gcc $(CFLAGS) writer.c -o writer -lrt

reader: reader.c common.h
	gcc $(CFLAGS) reader.c -o reader -lrt

clean:
	rm writer reader