#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>

#include <stdio.h>
#include <stdlib.h>
#include <error.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <stdbool.h>
#include <time.h>

static const char* shm_name = "lab_shm_memory" ;
#define SHM_SIZE 4096

#ifndef shm_open
int shm_open(const char *name, int oflag, mode_t mode);
#endif

#ifndef mmap
void *mmap(void *addr, size_t length, int prot, int flags,
    int fd, off_t offset);
int munmap(void *addr, size_t length);
#endif


void format_string(char* buffer, size_t maxcount) {
    time_t current = time(0);
    pid_t pid = getpid();
    snprintf(buffer, maxcount, "[%d] time: %ld", pid, current);
}

static bool run = true;

void handler_int(int sig) {
    (void)(sig);
    printf("\nRECEIVED SIGINT. Stopping...\n");
    run = false;
}

#define BUFFER_SIZE 512
static char buffer[BUFFER_SIZE];