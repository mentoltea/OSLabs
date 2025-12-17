#include <fcntl.h>
#include <sys/stat.h>
#include <pthread.h>
#include <semaphore.h>

#include <stdio.h>
#include <stdlib.h>

#include <errno.h>
#include <string.h>

#include <unistd.h>

#define ARRAY_SIZE 8
static int shared_array[8];
static int array_index = 0;
static sem_t sem;

void write_to_array(int value) {
    sem_wait(&sem);

    shared_array[array_index] = value;
    array_index = (array_index+1)%ARRAY_SIZE;

    sem_post(&sem);
}

void print_array() {
    sem_wait(&sem);

    pthread_t tid = pthread_self();
    printf("[%ld] ", tid);
    for (int i=0; i<ARRAY_SIZE; i++) {
        printf("%d ", shared_array[i]);
    }
    printf("\n");

    sem_post(&sem);
}

void *writer(void* arg) {
    (void)arg;
    for (int i=0; ;i++) {
        write_to_array(i);    
        usleep(20*1000);
    }
    return NULL;
}

void *reader(void* arg) {
    (void)arg;
    for (; ;) {
        print_array();    
        usleep(20*1000);
    }
    return NULL;
}

int main() {
    if (sem_init(&sem, 0, 0) == -1) {
        fprintf(stderr, "Unable to open semaphore: %s\n", strerror(errno));
        return 1;
    }
    // printf("Created\n");

    pthread_t writer_thread;
    pthread_t reader_thread;

    pthread_create(&reader_thread, NULL, reader, NULL);
    pthread_create(&writer_thread, NULL, writer, NULL);

    sem_post(&sem);

    pthread_join(writer_thread, NULL);
    pthread_join(reader_thread, NULL);

    sem_destroy(&sem);

    return 0;
}