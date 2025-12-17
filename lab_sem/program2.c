#include "common.h"

key_t shmkey, semkey; 
int shmid = 0;    
void* shmaddr = NULL;
int semid = 0;

bool have_locked = false;



void sem_lock(int sem_id) {
    struct sembuf sb = {0, -1, 0};
    semop(sem_id, &sb, 1);
    have_locked = true;
}
void sem_unlock(int sem_id) {
    struct sembuf sb = {0, 1, 0};
    semop(sem_id, &sb, 1);
    have_locked = false;
}

void free_at_exit(int sig) {
    (void)sig;
    // if (have_locked) sem_unlock(semid);
    // if (shmaddr) shmdt(shmaddr); 

    exit(0);
}

int main() {
    signal(SIGTERM, free_at_exit);
    signal(SIGINT, free_at_exit);

    if ((shmkey = ftok(SHMNAME, FTOK_ID)) == (key_t) -1) {
        fprintf(stderr, "Cannot get key: %s\n", strerror(errno));
        return 1;
    }
    if ((semkey = ftok(SEMNAME, FTOK_ID)) == (key_t) -1) {
        fprintf(stderr, "Cannot get key %s: %s\n", SEMNAME, strerror(errno));
        return 1;
    }

    semid = semget(semkey, 1, 0666);
    if (semid == -1) {
        fprintf(stderr, "Cannot get semaphore: %s\n", strerror(errno));
        return 1;
    }

    shmid = shmget(shmkey, SHM_SIZE, 0666);    
    if (shmid == -1) {
        fprintf(stderr, "Cannot get shared memory: %s\n", strerror(errno));
        return 1;
    }

    shmaddr = shmat(shmid, NULL, SHM_RDONLY);
    if (shmaddr == (void*)-1) {
        fprintf(stderr, "Cannot attach shared memory: %s\n", strerror(errno));
        return 1;
    }

    //
    while (1) {
        // sem_wait
        sem_lock(semid);
        
        // read
        time_t current = time(0);
        pid_t pid = getpid();
        printf("Received: %s\n", (char*)shmaddr);
        printf("Real    : [%d] time: %ld\n", pid, current);
        
        // sem_post
        sem_unlock(semid);

        sleep(1);
    }
    
    return 0;
}
