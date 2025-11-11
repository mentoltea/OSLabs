#include "fifo.h"

int main() {
    int fifo_fd;
    int res;
    unlink(fifo_path);
    
    res = mkfifo(fifo_path, O_RDWR | 0666);
    if (res == -1) {
        fprintf(stderr, "Cannot make fifo %s: %s\n", fifo_path, strerror(errno));
        return 1;
    }
    
    fifo_fd = open(fifo_path, O_RDWR);
    if (fifo_fd == -1) {
        fprintf(stderr, "Cannot open fifo %s: %s\n", fifo_path, strerror(errno));
        return 1;
    }
    
    time_t current_time = time(0);
    char* timestr = ctime(&current_time);
    int size = snprintf(buffer, BUFFER_SIZE, "Hello from process %d. Current time is %s", getpid(), timestr);
    if (size == -1) {
        fprintf(stderr, "Cannot format string\n");
        return 1;
    }

    res = write(fifo_fd, buffer, size);
    if (res == -1) {
        fprintf(stderr, "Cannot write to fd\n");
        return 1;
    }

    int sleep_seconds = 12;
    int updates_per_second = 4;
    int sleep_per_update = 1000*1000 / updates_per_second;
    int c = 0;
    int c_lim = 5;

    printf("Successfully written\n");
    for (int i = 0; i < sleep_seconds * updates_per_second; i++) {
        printf("\r");
            printf("\e[K");
            printf("Sleeping before exiting ");
            printf("(%.2f/%d)", (double)(i+1)/(double)(updates_per_second), sleep_seconds);
            for (int j=0; j<c; j++) {
                printf(".");
            }
            fflush(stdout);
            
            c++;
            c = c%c_lim;
            usleep(sleep_per_update);
    }
    printf("\n");

    return 0;
}