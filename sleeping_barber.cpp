#include <iostream>
#include <unistd.h>
#include <pthread.h>

/**
 * D (WiP)
 *
 * TODO need to resolve customer starvation (possibly using std::queue)
 */

// ANSI color escape codes to better identify threads
#define RED    "\x1B[31m"
#define GREEN  "\x1B[32m"
#define PURPLE "\x1B[35m"

// attributes to suppress warnings
#define NO_RETURN [[noreturn]]
#define NO_USE [[maybe_unused]]

pthread_mutex_t seats_mutex; // mutex to access the 'number_of_seats' variable
pthread_mutex_t barber_mutex; // mutex for the barber condition
pthread_mutex_t customer_mutex; // mutex for the customer condition

pthread_cond_t barber_cond;
pthread_cond_t customer_cond;

bool barber_ready = false;
bool customer_ready = false;

int number_of_seats = 5;

NO_RETURN void* barber(NO_USE void* arg);

void* customer(NO_USE void* arg);

int main() {
    pthread_t barber_thread;
    pthread_t customer_thread;

    // initalise the mutexes
    pthread_mutex_init(&seats_mutex, nullptr);
    pthread_mutex_init(&barber_mutex, nullptr);
    pthread_mutex_init(&customer_mutex, nullptr);

    // initialise the conditions
    pthread_cond_init(&barber_cond, nullptr);
    pthread_cond_init(&customer_cond, nullptr);

    // create the threads
    pthread_create(&barber_thread, nullptr, barber, nullptr);
    while (true) {
        sleep(random() % 3 + 2); // a customer arrives every 2-5 seconds
        pthread_create(&customer_thread, nullptr, customer, nullptr);
    }

    // TODO anyhting afterwards is redundant if ran in an infinite loop

    // wait for the threads to finish
    pthread_join(barber_thread, nullptr);
    pthread_join(customer_thread, nullptr);

    pthread_mutex_destroy(&seats_mutex);
    pthread_mutex_destroy(&barber_mutex);
    pthread_mutex_destroy(&customer_mutex);

    pthread_cond_destroy(&barber_cond);
    pthread_cond_destroy(&customer_cond);

    return EXIT_SUCCESS;
}

NO_RETURN void* barber(NO_USE void* arg) {
    while (true) {
        pthread_mutex_lock(&customer_mutex); // acquire the lock before waiting on the condition

        printf("%swaiting for customers to arrive\n", GREEN);
        while (!customer_ready)
            pthread_cond_wait(&customer_cond, &customer_mutex); // wait for customers to arrive
        customer_ready = false;
        printf("%sa customer has arrived\n", GREEN);

        pthread_mutex_lock(&seats_mutex);
        number_of_seats++; // increment the number of available seats
        pthread_mutex_unlock(&seats_mutex);

        pthread_mutex_lock(&barber_mutex);
        barber_ready = true;
        pthread_mutex_unlock(&barber_mutex);
        pthread_cond_signal(&barber_cond); // signal the customer that barber is ready

        printf("%scutting hair\n", GREEN); // service the customer
        sleep(random() % 3 + 2); // a customer is serviced every 2-5 seconds
        pthread_mutex_unlock(&customer_mutex); // release the lock for the next customer to be able to signal
    }
}

void* customer(NO_USE void* arg) {
    printf("%s%d seats are available\n", PURPLE, number_of_seats);
    pthread_mutex_lock(&seats_mutex);
    if (number_of_seats > 0) { // if there are empty seats in the waiting room
        number_of_seats--; // take a seat
        pthread_mutex_unlock(&seats_mutex);
        printf("%scustomer has taken a seat\n", RED);
        printf("%s%d seats are available\n\n", PURPLE, number_of_seats);

        printf("%ssignalling the barber that customer has arrived\n", RED);
        pthread_mutex_lock(&customer_mutex); // acquire the lock before signalling
        customer_ready = true;
        pthread_mutex_unlock(&customer_mutex);
        pthread_cond_signal(&customer_cond); // signal the barber that customer is ready

        pthread_mutex_lock(&barber_mutex);
        printf("%swaiting for barber\n", RED);
        while (!barber_ready)
            pthread_cond_wait(&barber_cond, &barber_mutex); // wait for the barber
        barber_ready = false;

        printf("%shaving hair cut\n\n", RED); // service the customer

        pthread_mutex_unlock(&barber_mutex);
    } else {
        pthread_mutex_unlock(&seats_mutex);
        printf("%scustomer has left\n\n", RED);
    }

    return EXIT_SUCCESS;
}
