#define _XOPEN_SOURCE

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

/**
 * A (Cancelled)
 */

// synchronization helpers
pthread_mutex_t r_mutex;
pthread_cond_t can_produce, can_consume;
time_t timer;

// the bucket
int array[10];

// routine for producer threads
void* produce(void* index);

// routine for producer threads
void* consume(void* index);

int main() {
    int indices[5] = { 1, 2, 3, 4, 5 };
    pthread_t producer[5];
    pthread_t consumer[5];
    int producer_return, consumer_return;

    srand48(time(NULL));
    pthread_mutex_init(&r_mutex, NULL);
    pthread_cond_init(&can_produce, NULL);
    pthread_cond_init(&can_consume, NULL);

    // will run for 10 seconds
    timer = time(NULL) + 10;

    for (int i = 0; i < 5; i++) {
        producer_return = pthread_create(&producer[i], NULL, produce, (void*) (&indices[i]));
        consumer_return = pthread_create(&consumer[i], NULL, consume, (void*) (&indices[i]));
    }

    // first signal to produce from the main thread
    pthread_cond_signal(&can_produce);

    for (int i = 0; i < 5; i++) {
        pthread_join(producer[i], NULL);
        pthread_join(consumer[i], NULL);
    }

    printf("producer returns: %d\n", producer_return);
    printf("consumer returns: %d\n", consumer_return);

    pthread_mutex_destroy(&r_mutex);
    pthread_cond_destroy(&can_produce);
    pthread_cond_destroy(&can_consume);

    return EXIT_SUCCESS;
}

// array index variables, temporary, to be replaced with a random number generator
int j = 0;
int l = 0;

void* produce(void* index) {
    int i = 0;
    do {
        pthread_mutex_lock(&r_mutex);
        pthread_cond_wait(&can_produce, &r_mutex);

        j = l % 10;
        array[j] = (int) lrand48() % 10;
        printf("producer %d: produces array[%d] as %d\n", *((int*) index), j, array[j]);
        ++i;

        pthread_cond_signal(&can_consume);
        pthread_mutex_unlock(&r_mutex);
    } while (time(NULL) < timer);

    return EXIT_SUCCESS;
}

void* consume(void* index) {
    int i = 0;
    do {
        pthread_mutex_lock(&r_mutex);
        pthread_cond_wait(&can_consume, &r_mutex);

        printf("consumer %d: consumes array[%d] as %d\n", *((int*) index), j, array[j]);
        ++i;
        ++l;

        pthread_cond_signal(&can_produce);
        pthread_mutex_unlock(&r_mutex);
    } while (time(NULL) < timer);

    return EXIT_SUCCESS;
}
