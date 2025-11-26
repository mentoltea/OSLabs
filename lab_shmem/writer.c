#include "common.h"

int main() {
    (void)(buffer);
    signal(SIGINT, handler_int);

    int shm_fd = shm_open(shm_name, O_RDWR | O_CREAT | O_EXCL, 0666);
    if (shm_fd == -1) {
        fprintf(stderr, "Cannot create memory region `%s`: %s\n", shm_name, strerror(errno));
        return 1;
    }

    if (ftruncate(shm_fd, SHM_SIZE) == -1) {
        fprintf(stderr, "Cannot truncate memory region: %s\n", strerror(errno));
        shm_unlink(shm_name);
        return 1;
    }

    void* shm_ptr = mmap(0, SHM_SIZE, PROT_WRITE | PROT_READ, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) {
        fprintf(stderr, "Cannot map memory region: %s\n", strerror(errno));
        shm_unlink(shm_name);
        return 1;
    }

    int c = 0;
    int c_lim = 5;
    while (run) {
        // update shm
        format_string(shm_ptr, SHM_SIZE);
        //
        printf("\r");
        printf("\e[K");
        printf("Writting");
        for (int j=0; j<c; j++) {
            putchar('.');
        }
        fflush(stdout);
        
        usleep(100*1000);
        c = (c+1)%c_lim;
    }
    printf("\n");
    
    munmap(shm_ptr, SHM_SIZE);
    shm_unlink(shm_name); 
                                                                           
    return 0;
}