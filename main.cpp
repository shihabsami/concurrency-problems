#ifdef READERS_WRITERS

/**
 * B (WiP)
 *
 * TODO resolve writer starvation(?)
 */

#define _XOPEN_SOURCE

#include <iostream>
#include <random>
#include <unistd.h>
#include <pthread.h>

#define COLOR_RED   "\u001b[31m"
#define COLOR_GREEN "\u001b[32m"
#define COLOR_RESET "\u001b[0m"

using std::cout;
using std::endl;

int resource = 0; // the resource
int current_readers = 0; // number of readers currently accessing the resource
bool should_exit = false;

pthread_mutex_t rw_mutex; // mutex to block access to the resource for both readers and writers
pthread_mutex_t r_mutex; // mutex to be acquired to modify the 'current_readers' variable

void* read(void* id); // routine for the reader threads

void* write(void* id); // routine for the writer threads

int main() {
    int thread_id[5] = { 1, 2, 3, 4, 5 }; // helps identify threads better
    pthread_t readers[5], writers[5];

    pthread_mutex_init(&rw_mutex, nullptr);
    pthread_mutex_init(&r_mutex, nullptr);

    for (int i = 0; i < 5; i++) {
        pthread_create(&writers[i], nullptr, write, (void*) (&thread_id[i]));
        pthread_create(&readers[i], nullptr, read, (void*) (&thread_id[i]));
    }

    usleep(10000000); // notify after 10 seconds
    should_exit = true;

    for (int i = 0; i < 5; i++) {
        pthread_join(writers[i], nullptr);
        pthread_join(readers[i], nullptr);
    }

    pthread_mutex_destroy(&rw_mutex);
    pthread_mutex_destroy(&r_mutex);

    cout << COLOR_RESET;
    return EXIT_SUCCESS;
}

void* read(void* id) {
    do {
        pthread_mutex_lock(&r_mutex); // prevent modifying the 'current_readers' variable by multiple threads at once
        if (++current_readers == 1) // if first reader, prevent writers to write by acquiring the lock
            pthread_mutex_lock(&rw_mutex);
        pthread_mutex_unlock(&r_mutex);

        // FIXME
        //  - multiple readers outputting to stdout at once causing half written outputs
        //  - but using a mutex would violate(?) the rule of multiple readers reading at once
        int reader_id = *((int*) id);
        cout << COLOR_RED << "reader [" << reader_id << "] reads resource as " << resource << endl;

        pthread_mutex_lock(&r_mutex);
        if (--current_readers == 0) // if last reader, notify writers that resource is available to write
            pthread_mutex_unlock(&rw_mutex);
        cout << COLOR_RED << "reader [" << reader_id << "] exits" << endl;
        pthread_mutex_unlock(&r_mutex);
    } while (!should_exit);

    return EXIT_SUCCESS;
}

void* write(void* id) {
    do {
        pthread_mutex_lock(&rw_mutex); // shall wait if readers present

        int writer_id = *((int*) id);
        resource = (int) random() % 10; // perform write
        cout << COLOR_GREEN << "writer [" << writer_id << "] modifies resource to " << resource << endl;

        cout << COLOR_GREEN << "writer [" << writer_id << "] exits" << endl;
        pthread_mutex_unlock(&rw_mutex);
    } while (!should_exit);

    return EXIT_SUCCESS;
}

#elif defined(SLEEPING_BARBERS)

/**
 * D (WiP)
 *
 * TODO create a second barber thread
 * TODO if simulation is to be run for a certain amount of time, ensure memory is freed
 */

#include <iostream>
#include <unistd.h>
#include <pthread.h>
#include <deque>

// ANSI color escape codes to better identify threads
#define COLOR_RED    "\u001b[31m"
#define COLOR_GREEN  "\u001b[32m"
#define COLOR_BLUE   "\u001b[34m"
#define COLOR_PURPLE "\u001b[35m"
#define COLOR_RESET  "\u001b[0m"

// attributes to suppress warnings
#define NO_RETURN [[noreturn]]
#define NO_USE    [[maybe_unused]]

using std::cout;
using std::endl;

#define NUMBER_OF_SEATS 5
int available_seats = NUMBER_OF_SEATS;
int customer_id = 0;
std::deque<int>* wait_queue;

// conditional variables to wait/signal threads
pthread_cond_t barber_ready;
pthread_cond_t customer_ready;

pthread_cond_t seats_ready[NUMBER_OF_SEATS];
pthread_mutex_t wait_mutex; // mutex to access the 'wait_queue' variable
pthread_mutex_t seats_mutex; // mutex to access the 'available_seats' variable
pthread_mutex_t barber_mutex; // mutex for the barber condition
pthread_mutex_t customer_mutex; // mutex for the customer condition

NO_RETURN void* barber(NO_USE void* arg); // routine for the barber thread

void* customer(NO_USE void* arg); // routine for the customer threads

NO_RETURN void run();

int main() {
    pthread_t barber_thread;
    wait_queue = new std::deque<int>();

    pthread_cond_init(&barber_ready, nullptr);
    pthread_cond_init(&customer_ready, nullptr);
    for (auto& seat: seats_ready)
        pthread_cond_init(&seat, nullptr);

    pthread_mutex_init(&wait_mutex, nullptr);
    pthread_mutex_init(&seats_mutex, nullptr);
    pthread_mutex_init(&barber_mutex, nullptr);
    pthread_mutex_init(&customer_mutex, nullptr);

    pthread_create(&barber_thread, nullptr, barber, nullptr);
    run();
}

NO_RETURN void run() {
    pthread_t customer_thread;
    while (true) {
        usleep(random() % 200000); // a customer arrives every 100-300 milliseconds
        pthread_create(&customer_thread, nullptr, customer, nullptr);
    }
}

void print_queue() {
    cout << COLOR_PURPLE << "wait queue:";
    for (int& it: *wait_queue)
        cout << COLOR_PURPLE << " [" << char(it + 65) << "]"; // format as an alphabet for readability

    cout << endl << endl;
}

NO_RETURN void* barber(NO_USE void* arg) {
    while (true) {
        pthread_mutex_lock(&customer_mutex); // acquire the lock before waiting on the conditional variable
        if (available_seats == NUMBER_OF_SEATS) { // if no customer yet, sleep
            cout << COLOR_GREEN << "waiting for customer" << COLOR_RESET << endl;
            pthread_cond_wait(&customer_ready, &customer_mutex); // wait for a customer thread to signal
        }
        pthread_mutex_unlock(&customer_mutex);

        cout << COLOR_GREEN << "a customer has entered the room" << COLOR_RESET << endl;
        pthread_mutex_lock(&seats_mutex);
        available_seats++; // increment the number of available seats
        pthread_mutex_unlock(&seats_mutex);

        pthread_mutex_lock(&wait_mutex);
        pthread_cond_signal(&seats_ready[wait_queue->front()]); // signal the first customer thread in queue
        wait_queue->pop_front();
        pthread_mutex_unlock(&wait_mutex);
        cout << COLOR_GREEN << "cutting hair" << COLOR_RESET << endl; // service the customer thread
        usleep(random() % 100000 + 200000); // a customer is serviced every 100-300 milliseconds
    }
}


void* customer(NO_USE void* arg) {
    cout << COLOR_BLUE << available_seats << " seats are available" << COLOR_RESET << endl;
    print_queue();
    pthread_mutex_lock(&seats_mutex); // mutual exclusion to the available_seats variable

    if (available_seats > 0) { // take a seat if there are empty seats in the waiting room
        pthread_mutex_lock(&wait_mutex);
        int serial = customer_id++ % NUMBER_OF_SEATS; // recurring id to help better identify customer threads
        wait_queue->emplace_back(serial);
        pthread_mutex_unlock(&wait_mutex);

        if (available_seats-- == NUMBER_OF_SEATS) { // if first customer, wake up the barber thread
            cout << COLOR_RED << "customer [" << char(serial + 65) << "] has taken a seat" << COLOR_RESET << endl;
            pthread_cond_signal(&customer_ready);
        }
        pthread_mutex_unlock(&seats_mutex);

        pthread_mutex_lock(&barber_mutex);
        cout << COLOR_RED << "waiting for barber to be available" << COLOR_RESET << endl;
        pthread_cond_wait(&seats_ready[serial], &barber_mutex); // wait for the barber
        cout << COLOR_PURPLE << "customer [" << char(serial + 65) << "] is done" << COLOR_RESET << endl << endl;
        pthread_mutex_unlock(&barber_mutex); // release the lock for the next customer thread
    } else {
        pthread_mutex_unlock(&seats_mutex); // leave if no seats are available
        cout << COLOR_RED << "customer has left finding no seat available" << COLOR_RESET << endl;
    }

    return EXIT_SUCCESS;
}

#endif
