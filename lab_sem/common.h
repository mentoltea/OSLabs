#include <fcntl.h>
#include <sys/stat.h>
#include <pthread.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include <errno.h>
#include <string.h>

#include <unistd.h>
#include <signal.h>
#include <time.h>

#define SHM_SIZE 1024

#define SHMNAME "shared_memory"
#define FTOK_ID 1

#define SEMNAME "shared_semaphore"