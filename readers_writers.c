#define _XOPEN_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

/**
 * B (WiP)
 *
 * TODO need to resolve writer starvation
 */

#define RED  "\x1B[31m"
#define GREEN  "\x1B[32m"

// synchronization helpers
pthread_mutex_t rw_mutex;
pthread_mutex_t r_mutex;
pthread_attr_t rw_attr;
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
    pthread_attr_init(&rw_attr);
    pthread_attr_setschedpolicy(&rw_attr, SCHED_FIFO);

    // will run for 10 seconds
    timer = time(NULL) + 10;

    for (int i = 0; i < 5; i++) {
        pthread_create(&readers[i], &rw_attr, read, (void*) (&indices[i]));
        pthread_create(&writers[i], NULL, write, (void*) (&indices[i]));
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

        pthread_mutex_lock(&r_mutex); // prevent modifying reader count by multiple threads at once
        if (++current_readers == 1) // if readers present, prevent writers to write
            pthread_mutex_lock(&rw_mutex);
        pthread_mutex_unlock(&r_mutex);

        printf("%sreader %d: reads resource as %d\n", RED, *((int*) index), resource);

        pthread_mutex_lock(&r_mutex);
        if (--current_readers == 0)
            pthread_mutex_unlock(&rw_mutex);
        pthread_mutex_unlock(&r_mutex);

        printf("%sreader %d: exits\n", RED, *((int*) index));

    } while (time(NULL) < timer);

    return EXIT_SUCCESS;
}

void* write(void* index) {
    do {

        pthread_mutex_lock(&rw_mutex); // shall wait if readers present
        resource = (int) lrand48() % 10;
        printf("%swriter %d: modifies resource to %d\n", GREEN, *((int*) index), resource);
        pthread_mutex_unlock(&rw_mutex);

        printf("%swriter %d: exits\n", GREEN, *((int*) index));

    } while (time(NULL) < timer);

    return EXIT_SUCCESS;
}
