#define _XOPEN_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

/**
 * B (WiP)
 *
 * TODO need to resolve writer starvation
 */

// synchronization helpers
pthread_mutex_t rw_mutex;
pthread_mutex_t r_mutex;
time_t timer;

// number of readers currently accessing the resource
int current_readers = 0;

// the resource
int resource = 0;

// routine for reader threads
void* read(void* index);

// routine for writer threads
void* write(void* index);

int main() {
    int indices[5] = { 1, 2, 3, 4, 5 };
    pthread_t readers[5], writers[5];

    srand48(time(NULL));
    pthread_mutex_init(&rw_mutex, NULL);
    pthread_mutex_init(&r_mutex, NULL);

    // will run for 10 seconds
    timer = time(NULL) + 10;

    for (int i = 0; i < 5; i++) {
        pthread_create(&readers[i], NULL, read, (void*)(&indices[i]));
        pthread_create(&writers[i], NULL, write, (void*)(&indices[i]));
    }

    for (int i = 0; i < 5; i++) {
        pthread_join(readers[i], NULL);
        pthread_join(writers[i], NULL);
    }

    pthread_mutex_destroy(&rw_mutex);
    pthread_mutex_destroy(&r_mutex);

    return EXIT_SUCCESS;
}

void* read(void* index) {
    do {
        // prevent modifying reader count by multiple threads at once
        pthread_mutex_lock(&r_mutex);
        current_readers++;
        // if readers present, prevent writers to write
        if (current_readers == 1)
            pthread_mutex_lock(&rw_mutex);
        pthread_mutex_unlock(&r_mutex);

        printf("reader %d: reads resource as %d\n", *((int*) index), resource);

        pthread_mutex_lock(&r_mutex);
        current_readers--;
        if (current_readers == 0)
            pthread_mutex_unlock(&rw_mutex);
        pthread_mutex_unlock(&r_mutex);
    } while (time(NULL) < timer);

    return EXIT_SUCCESS;
}

void* write(void* index) {
    do {
        // shall wait if readers present
        pthread_mutex_lock(&rw_mutex);
        resource = (int) lrand48() % 10;
        printf("writer %d: modifies resource to %d\n", *((int*) index), resource);
        pthread_mutex_unlock(&rw_mutex);
    } while (time(NULL) < timer);

    return EXIT_SUCCESS;
}
