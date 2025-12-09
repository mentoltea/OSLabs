#include "fifo.h"

int main() {
    int fifo_fd;

    fifo_fd = open(fifo_path, O_RDWR);
    if (fifo_fd == -1) {
        fprintf(stderr, "Cannot open fifo %s: %s\n", fifo_path, strerror(errno));
        return 1;
    }

    int size = read(fifo_fd, buffer, BUFFER_SIZE);
    if (size == -1) {
        fprintf(stderr, "Cannot write to fd\n");
        return 1;
    }
    buffer[size-1] = '\0';

    printf("[fifo] %s\n", buffer);

    time_t current_time = time(0);
    char* timestr = ctime(&current_time);
    printf("Process's pid is %d. Current time is %s", getpid(), timestr);

    close(fifo_fd);

    return 0;
}