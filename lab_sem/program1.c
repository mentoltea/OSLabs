#include "common.h"

key_t shmkey, semkey; 
int shmid = 0;    
void* shmaddr = NULL;

int semid = 0;

void free_at_exit(int sig) {
    (void)sig;
    // if (shmaddr) shmdt(shmaddr); 
    if (shmid)  shmctl(shmid, IPC_RMID, NULL);
    if (semid)  semctl(semid, 0, IPC_RMID);
    unlink(SHMNAME);
    unlink(SEMNAME);

    exit(0);
}

void sem_lock(int sem_id) {
    struct sembuf sb = {0, -1, 0};
    semop(sem_id, &sb, 1);
}
void sem_unlock(int sem_id) {
    struct sembuf sb = {0, 1, 0}; 
    semop(sem_id, &sb, 1);
}

int main() {
    signal(SIGTERM, free_at_exit);
    signal(SIGINT, free_at_exit);

    int shmfd = open(SHMNAME, O_CREAT | O_EXCL | O_WRONLY);
    if (shmfd == -1) {
        fprintf(stderr, "Cannot create %s: %s\n", SHMNAME, strerror(errno));
        return 1;
    }
    close(shmfd);

    int semmfd = open(SEMNAME, O_CREAT | O_EXCL | O_RDWR);
    if (semmfd == -1) {
        fprintf(stderr, "Cannot create %s: %s\n", SEMNAME, strerror(errno));
        return 1;
    }
    close(semmfd);

    if ((shmkey = ftok(SHMNAME, FTOK_ID)) == (key_t) -1) {
        fprintf(stderr, "Cannot get key %s: %s\n", SHMNAME, strerror(errno));
        return 1;
    }

    if ((semkey = ftok(SEMNAME, FTOK_ID)) == (key_t) -1) {
        fprintf(stderr, "Cannot get key %s: %s\n", SEMNAME, strerror(errno));
        return 1;
    }

    semid = semget(semkey, 1, IPC_CREAT | IPC_EXCL | 0666);
    if (semid == -1) {
        fprintf(stderr, "Cannot get semaphore: %s\n", strerror(errno));
        return 1;
    }
    
    shmid = shmget(shmkey, SHM_SIZE, IPC_CREAT | IPC_EXCL | 0666);    
    if (shmid == -1) {
        fprintf(stderr, "Cannot get shared memory: %s\n", strerror(errno));
        return 1;
    }

    shmaddr = shmat(shmid, NULL, 0);
    if (shmaddr == (void*)-1) {
        fprintf(stderr, "Cannot attach shared memory: %s\n", strerror(errno));
        return 1;
    }

    semctl(semid, 0, SETVAL, 1);
    memset(shmaddr, 0, SHM_SIZE);

    //
    while (1) {
        // sem_wait
        sem_lock(semid);
        
        // write
        time_t current = time(0);
        pid_t pid = getpid();
        snprintf((char*)shmaddr, SHM_SIZE, "[%d] time: %ld", pid, current);
        
        // sem_post
        sem_unlock(semid);

        sleep(2);
    }
    
    return 0;
}
