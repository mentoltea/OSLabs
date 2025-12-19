#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define ARRAY_SIZE 8
static int array[ARRAY_SIZE];
static int index = 0;

pthread_rwlock_t rwlock = PTHREAD_RWLOCK_INITIALIZER;

void write_to_array(int value) {
    pthread_rwlock_wrlock(&rwlock);
    
    array[index] = value;
    index = (index+1)%ARRAY_SIZE;
    
    pthread_rwlock_unlock(&rwlock);
}

void print_array() {
    pthread_rwlock_rdlock(&rwlock);

    pthread_t tid = pthread_self();
    printf("[%ld] ", tid);
    for (int i=0; i<ARRAY_SIZE; i++) {
        printf("%d ", array[i]);
    }
    printf("\n");

    pthread_rwlock_unlock(&rwlock);
}

void *writer(void* arg) {
    (void)arg;
    for (int i=1; ;i++) {
        write_to_array(i);    
        sleep(1);
        // usleep(500 * 100);
    }
    return NULL;
}

void *reader(void* arg) {
    (void)arg;
    for (; ;) {
        print_array();    
        usleep(500 * 1000);
    }
    return NULL;
}


int main() {

    pthread_t writer_thread;
    pthread_t reader_threads[10];

    pthread_create(&writer_thread, NULL, writer, NULL);
    for (int i=0; i<10; i++) {
        pthread_create(&reader_threads[i], NULL, reader, NULL);
        usleep(10);
    }
    
    
    pthread_join(writer_thread, NULL);
    for (int i=0; i<10; i++)
        pthread_join(reader_threads[i], NULL);

    return 0;
}