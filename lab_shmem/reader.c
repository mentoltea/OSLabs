#include "common.h"

int main() {
    signal(SIGINT, handler_int);

    int shm_fd = shm_open(shm_name, O_RDONLY, 0666);
    if (shm_fd == -1) {
        fprintf(stderr, "Cannot open memory region `%s`: %s\n", shm_name, strerror(errno));
        return 1;
    }

    void* shm_ptr = mmap(0, SHM_SIZE, PROT_READ, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) {
        fprintf(stderr, "Cannot map memory region: %s\n", strerror(errno));
        return 1;
    }

    while (run) {
        // update shm
        format_string(buffer, BUFFER_SIZE);
        //

        printf("Mine: %s\n", buffer);
        printf("Shared: %s\n", (char*)shm_ptr);
        putchar('\n');

        usleep(500*1000);
    }
    printf("\n");
    
    munmap(shm_ptr, SHM_SIZE);
                                                                           
    return 0;
}