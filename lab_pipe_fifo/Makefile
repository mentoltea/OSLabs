CFLAGS = -Wall -Wextra

.PHONY: clean all

all: pipe fifo_read fifo_write

pipe: pipe.c
	gcc $(CFLAGS) pipe.c -o pipe

fifo_read: fifo_read.c fifo.h
	gcc $(CFLAGS) fifo_read.c -o fifo_read 
	
fifo_write: fifo_write.c fifo.h
	gcc $(CFLAGS) fifo_write.c -o fifo_write 

clean:
	rm pipe fifo_read fifo_write