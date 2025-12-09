#include <pthread.h>

#include <unistd.h>
#include <sys/types.h>

#include <stdio.h>
#include <time.h>

#define MAX_STRING_SIZE 256 
static char shared_string[MAX_STRING_SIZE];
static pthread_mutex_t shared_mutex = PTHREAD_MUTEX_INITIALIZER;

#define READER_COUNT 10

void print_shared() {
    if (pthread_mutex_trylock(&shared_mutex) != 0) return;
    
    pthread_t tid = pthread_self();
    printf("[%ld] %s\n", tid, shared_string);

    pthread_mutex_unlock(&shared_mutex);
}

void write_to_shared(int value) {
    if (pthread_mutex_lock(&shared_mutex) != 0) return;
    
    snprintf(shared_string, MAX_STRING_SIZE, "Counter: %d", value);

    pthread_mutex_unlock(&shared_mutex);
}

void* writer_task(void*) {
    useconds_t dt = 20;
    for (int i=0 ; ; i++) {
        write_to_shared(i);
        usleep(dt);
    }
}

void* reader_task(void*) {
    useconds_t dt = 2 * 1000 * 1000 /  READER_COUNT;
    while (1) {
        print_shared();
        usleep(dt);
    }
}

int main() {
    pthread_t writer_thread;
    pthread_t reader_threads[READER_COUNT];

    pthread_create(&writer_thread, NULL, writer_task, NULL);
    for (int i=0; i<READER_COUNT; i++) {
        pthread_create(&reader_threads[i], NULL, reader_task, NULL);
        usleep(80);
    }
    
    // Lock навсегда - поток в бесконечном цикле
    pthread_join(writer_thread, NULL);
    for (int i=0; i<READER_COUNT; i++) {
        pthread_join(reader_threads[i], NULL);
    }

    return 0;
}