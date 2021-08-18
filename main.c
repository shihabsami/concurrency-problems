#define _XOPEN_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

/**
 * Q1 (WiP)
 */

/// synchronization helpers
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t can_produce, can_consume;

/// the bucket
long array[10];

/// routine for producer threads
void* produce();

/// routine for consumer threads
void* consume();

int main() {
    pthread_t producer[5];
    pthread_t consumer[5];
    int producer_return, consumer_return;
    srand48(time(NULL));

    for (int i = 0; i < 5; i++) {
        producer_return = pthread_create(&producer[i], NULL, produce, NULL);
        consumer_return = pthread_create(&consumer[i], NULL, consume, NULL);
    }

    // first signal to produce from the main thread
    pthread_cond_signal(&can_produce);

    for (int i = 0; i < 5; i++) {
        pthread_join(producer[i], NULL);
        pthread_join(consumer[i], NULL);
    }

    printf("producer returns: %d\n", producer_return);
    printf("consumer returns: %d\n", consumer_return);

    return EXIT_SUCCESS;
}

/// array index variables, temporary, to be replaced with a random number generator
int j = 0;
int l = 0;

/// number of times ran, temporary, to be replaced with a timer
int k = 10;

void* produce() {
    int i = 0;
    while (i < k) {
        pthread_mutex_lock(&mutex);
        pthread_cond_wait(&can_produce, &mutex);

        j = l % 10;
        array[j] = lrand48();
        printf("producer: array[%d]: %ld\n", j, array[j]);
        ++i;

        pthread_cond_signal(&can_consume);
        pthread_mutex_unlock(&mutex);
    }

    return EXIT_SUCCESS;
}

void* consume() {
    int i = 0;
    while (i < k) {
        pthread_mutex_lock(&mutex);
        pthread_cond_wait(&can_consume, &mutex);

        printf("consumer: array[%d]: %ld\n", j, array[j]);
        ++i;
        ++l;

        pthread_cond_signal(&can_produce);
        pthread_mutex_unlock(&mutex);
    }

    return EXIT_SUCCESS;
}
