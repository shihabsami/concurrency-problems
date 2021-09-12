#define _XOPEN_SOURCE

#define NO_COLOUR    "\u001b[0m"

#ifdef COLOUR_PRINT
    #define COLOUR_PURPLE    "\u001b[35m"
    #define COLOUR_GREEN     "\u001b[32m"
#else
    #define COLOUR_PURPLE    NO_COLOUR
    #define COLOUR_GREEN     NO_COLOUR
#endif

#ifdef READERS_WRITERS

/**
 * B: The Readers and Writers Problem.
 */

#define _XOPEN_SOURCE

#include <iostream>
#include <string>
#include <unistd.h>
#include <pthread.h>

// Parameters as specified in the problem statement
constexpr unsigned int TOTAL_READERS = 5;       // Number of reader threads
constexpr unsigned int TOTAL_WRITERS = 5;       // Number of writer threads
constexpr unsigned int RUNTIME_SECONDS = 10;    // Total runtime for the simulation
constexpr unsigned int TOTAL_THREADS = TOTAL_READERS + TOTAL_WRITERS;

long resource = 0;                                      // The resource
unsigned int queue_signalled = 1, queue_waiting = 0;    // Variables to simulate the queue
unsigned int current_readers = 0;                       // Number of readers currently accessing the resource
bool can_write = false;                                 // Flag on whether the writer threads can write
bool can_exit = false;                                  // Flag on whether the threads can terminate

pthread_mutex_t readers_mutex;     // To be acquired to modify the 'current_readers' variable
pthread_cond_t queue_cond;         // To wait for a thread's turn in the queue
pthread_mutex_t queue_mutex;       // To be acquired to wait on the conditional variable above
pthread_cond_t resource_cond;      // To provide exclusive access to the resource for either the readers or the writers
pthread_mutex_t resource_mutex;    // To be acquired to wait on the conditional variable above
pthread_mutex_t exit_mutex;        // To be acquired to access the 'can_exit' variable

void* read(void* reader_id);                  // Routine for the reader threads
void* write(void* writer_id);                 // Routine for the writer threads
void init_pthread_constructs();               // Initialise the pthread constructs used throughout (mutex, cond etc.)
void destroy_pthread_constructs();            // Destroy the pthread constructs used
void print_reader(const std::string& str);    // Utility function to print for the reader threads
void print_writer(const std::string& str);    // Utility function to print for the writer threads
bool should_exit();                           // Utility function to check whether threads should exit

int main() {
    init_pthread_constructs();
    pthread_t reader_threads[TOTAL_READERS], writer_threads[TOTAL_WRITERS];

    // Integer id to help identify threads better
    unsigned int reader_id[TOTAL_READERS], writer_id[TOTAL_WRITERS];
    for (unsigned int i = 0; i < TOTAL_READERS; i++)
        reader_id[i] = i + 1;
    for (unsigned int i = 0; i < TOTAL_WRITERS; i++)
        writer_id[i] = i + 1;

    for (unsigned int& id: reader_id)
        pthread_create(&reader_threads[id - 1], nullptr, read, (void*) &id);
    for (unsigned int& id: writer_id)
        pthread_create(&writer_threads[id - 1], nullptr, write, (void*) &id);

    // Exit flag is set after specified number of seconds
    sleep(RUNTIME_SECONDS);
    pthread_mutex_lock(&exit_mutex);
    can_exit = true;
    pthread_mutex_unlock(&exit_mutex);

    for (const pthread_t& thread: reader_threads)
        pthread_join(thread, nullptr);
    for (const pthread_t& thread: writer_threads)
        pthread_join(thread, nullptr);

    destroy_pthread_constructs();
    return EXIT_SUCCESS;
}

void* read(void* reader_id) {
    int id = *(int*) reader_id;
    std::string prefix = "Reader [" + std::to_string(id) + "]";

    do {
        // Enqueue and wait until the resource is available to access
        pthread_mutex_lock(&queue_mutex);
        queue_waiting = (queue_waiting + 1) % TOTAL_THREADS;
        unsigned int queue_position = queue_waiting;
        bool in_queue = false;

        while (queue_signalled != queue_position) {
            if (!in_queue) {
                print_reader(prefix + " enters queue to access the resource");
                in_queue = true;
            }
            pthread_cond_wait(&queue_cond, &queue_mutex);
        }
        // Reader has been unblocked and may access the resource now
        print_reader(prefix + "'s turn");
        pthread_mutex_unlock(&queue_mutex);

        pthread_mutex_lock(&readers_mutex);
        // If first reader, prevent writers to write by setting the flag
        if (++current_readers == 1) {
            pthread_mutex_lock(&resource_mutex);
            can_write = false;
            pthread_mutex_unlock(&resource_mutex);
        }
        // Dequeue the next waiting thread
        pthread_mutex_lock(&queue_mutex);
        queue_signalled = (queue_signalled + 1) % TOTAL_THREADS;
        pthread_cond_broadcast(&queue_cond);
        pthread_mutex_unlock(&queue_mutex);
        pthread_mutex_unlock(&readers_mutex);

        // Read operation on the resource
        print_reader(prefix + " reads the resource as " + std::to_string(resource));

        pthread_mutex_lock(&readers_mutex);
        // If last reader, notify writers that resource is now available to write
        if (--current_readers == 0) {
            pthread_mutex_lock(&resource_mutex);
            can_write = true;
            pthread_cond_broadcast(&resource_cond);
            pthread_mutex_unlock(&resource_mutex);
        }
        print_reader(prefix + " is done reading the resource");
        pthread_mutex_unlock(&readers_mutex);
    } while (!should_exit());

    print_reader(prefix + " exits the simulation");
    return EXIT_SUCCESS;
}

void* write(void* writer_id) {
    int id = *(int*) writer_id;
    std::string prefix = "Writer [" + std::to_string(id) + "]";

    do {
        // Enqueue and wait until the resource is available to access
        pthread_mutex_lock(&queue_mutex);
        queue_waiting = (queue_waiting + 1) % TOTAL_THREADS;
        unsigned int queue_position = queue_waiting;
        bool in_queue = false;

        while (queue_signalled != queue_position) {
            if (!in_queue) {
                print_writer(prefix + " enters queue to access the resource");
                in_queue = true;
            }
            pthread_cond_wait(&queue_cond, &queue_mutex);
        }
        pthread_mutex_unlock(&queue_mutex);

        // Wait if there are readers reading
        pthread_mutex_lock(&resource_mutex);
        while (!can_write)
            pthread_cond_wait(&resource_cond, &resource_mutex);

        print_writer(prefix + "'s turn");
        resource = random() % 100;
        print_writer(prefix + " modifies the resource to " + std::to_string(resource));
        print_writer(prefix + " is done writing to the reasource");

        // Dequeue the next waiting thread
        pthread_mutex_lock(&queue_mutex);
        queue_signalled = (queue_signalled + 1) % TOTAL_THREADS;
        pthread_cond_broadcast(&queue_cond);
        pthread_mutex_unlock(&queue_mutex);
        pthread_mutex_unlock(&resource_mutex);
    } while (!should_exit());

    print_writer(prefix + " exits the simulation");
    return EXIT_SUCCESS;
}

void init_pthread_constructs() {
    pthread_cond_init(&queue_cond, nullptr);
    pthread_mutex_init(&queue_mutex, nullptr);
    pthread_mutex_init(&readers_mutex, nullptr);
    pthread_cond_init(&resource_cond, nullptr);
    pthread_mutex_init(&resource_mutex, nullptr);
    pthread_mutex_init(&exit_mutex, nullptr);
}

void destroy_pthread_constructs() {
    pthread_cond_destroy(&queue_cond);
    pthread_mutex_destroy(&queue_mutex);
    pthread_mutex_destroy(&readers_mutex);
    pthread_cond_destroy(&resource_cond);
    pthread_mutex_destroy(&resource_mutex);
    pthread_mutex_destroy(&exit_mutex);
}

void print_reader(const std::string& str) {
    std::string formatted = std::string(COLOUR_GREEN) + str + NO_COLOUR + "\n";
    std::cout << formatted;
}

void print_writer(const std::string& str) {
    std::string formatted = std::string(COLOUR_PURPLE) + str + NO_COLOUR + "\n";
    std::cout << formatted;
}

bool should_exit() {
    // Atomically accesses and returns the value of the 'can_exit' variable
    pthread_mutex_lock(&exit_mutex);
    bool value = can_exit;
    pthread_mutex_unlock(&exit_mutex);
    return value;
}

#elif defined(SLEEPING_BARBERS)

/**
 * D: The Sleeping Barbers Problem.
 */

#include <iostream>
#include <string>
#include <array>
#include <vector>
#include <algorithm>
#include <unistd.h>
#include <pthread.h>

// Parameters as specified in the problem statement, can be varied
constexpr unsigned int TOTAL_BARBERS = 5;
constexpr unsigned int TOTAL_SEATS = 5;
constexpr unsigned int TOTAL_THREADS = TOTAL_BARBERS + TOTAL_SEATS;
constexpr unsigned int RUNTIME_SECONDS = 10;

bool can_exit = false;                                           // Flag to denote whether the threads can terminate
unsigned int current_customers = 0;                              // Number of customers currently in the shop
unsigned int queue_waiting = 1, queue_signalled = 0;             // Variables to simulate the queue
std::array<int, TOTAL_BARBERS> service_counts, service_times;    // To collect statistics data

pthread_cond_t queue_cond;         // To wait/signal for a thread's turn in the queue
pthread_mutex_t queue_mutex;       // To modify the two variables simulating the queue and to wait on 'queue_cond'
pthread_cond_t customer_cond;      // To wait if no customers are available
pthread_mutex_t customer_mutex;    // To be acquired to wait on the conditional variable above
pthread_mutex_t service_mutex;     // To modify the two variables that keep track of service statistics data
pthread_mutex_t exit_mutex;        // To be acquired to access the 'can_exit' variable

void* barber_routine(void* barber_id);          // Routine for the barber threads
void* customer_routine(void* customer_id);      // Routine for the customer threads
void init_pthread_constructs();                 // Initialise the pthread constructs used throughout (mutex, cond etc.)
void destroy_pthread_constructs();              // Destroy the pthread constructs used
void print_barber(const std::string& str);      // Utility function to print for the barber threads
void print_customer(const std::string& str);    // Utility function to print for the customer threads
bool should_exit();                             // Utility function to check whether threads should exit

int main() {
    std::vector<pthread_t*> barber_threads;
    std::vector<pthread_t*> customer_threads;
    unsigned int barber_ids[TOTAL_BARBERS], customer_id = 0;
    for (unsigned int i = 0; i < TOTAL_BARBERS; i++)
        barber_ids[i] = i + 1;

    for (unsigned int& barber_id: barber_ids) {
        auto* barber_thread = new pthread_t();
        barber_threads.push_back(barber_thread);
        pthread_create(barber_thread, nullptr, barber_routine, &barber_id);
    }

    time_t timer = time(nullptr) + RUNTIME_SECONDS;
    while (true) {
        pthread_mutex_lock(&exit_mutex);
        // Exit flag is set after specified number of seconds
        can_exit = time(nullptr) > timer;
        if (can_exit) {
            pthread_mutex_unlock(&exit_mutex);
            break;
        } else {
            pthread_mutex_unlock(&exit_mutex);
        }

        // A customer arrives every 10-30 milliseconds
        usleep(random() % 20000 + 10000);

        // All created threads are kept track of to be able to join later
        auto* customer_thread = new pthread_t();
        customer_threads.push_back(customer_thread);
        customer_id = customer_threads.size();
        pthread_create(customer_thread, nullptr, customer_routine, &(customer_id));
    }

    // Send signal to any customer threads currently waiting on barbers
    for (unsigned int i = 0; i < TOTAL_SEATS + 1; i++) {
        pthread_mutex_lock(&queue_mutex);
        queue_signalled = (queue_signalled + 1) % TOTAL_SEATS;
        pthread_cond_broadcast(&queue_cond);
        pthread_mutex_unlock(&queue_mutex);
    }

    for (pthread_t* thread: customer_threads) {
        pthread_join(*thread, nullptr);
        delete thread;
    }

    // Send signal to any barber threads currently waiting on customers
    for (unsigned int i = 0; i < TOTAL_BARBERS; i++) {
        pthread_mutex_lock(&customer_mutex);
        current_customers++;
        pthread_cond_broadcast(&customer_cond);
        pthread_mutex_unlock(&customer_mutex);
    }

    for (pthread_t* thread: barber_threads) {
        pthread_join(*thread, nullptr);
        delete thread;
    }

    // Print statistics
    for (unsigned int i = 0; i < TOTAL_BARBERS; i++) {
        std::cout
            << "Barber [" << i + 1 << "] had "
            << service_counts[i]
            << " services, accumulated work time of "
            << service_times[i] / 1000 << " milliseconds, on average "
            << (float) service_counts[i] / ((float) service_times[i] / 1000)
            << " services per millisecond"
            << std::endl;
    }

    destroy_pthread_constructs();
    return EXIT_SUCCESS;
}

void* barber_routine(void* barber_id) {
    int id = *(int*) barber_id;
    std::string prefix = "Barber [" + std::to_string(id) + "]";

    do {
        pthread_mutex_lock(&customer_mutex);
        // Function to deduce if the waiting thread is the least worked
        auto is_least_worked = [&] {
            pthread_mutex_lock(&service_mutex);
            bool value = *std::min_element(service_times.begin(), service_times.end())
                == service_times[id - 1];
            pthread_mutex_unlock(&service_mutex);
            return value;
        };

        // If no customer yet, sleep
        bool is_asleep = false;
        while ((current_customers == 0 || !is_least_worked()) && !should_exit()) {
            // Wait for customers to arrive
            if (!is_asleep) {
                // If already asleep, only waits till it is the/one of the least worked barber(s)
                print_barber(prefix + " is sleeping");
                is_asleep = true;
            }
            pthread_cond_wait(&customer_cond, &customer_mutex);
        }

        // Free up a seat
        current_customers--;
        if (should_exit()) {
            pthread_mutex_unlock(&customer_mutex);
            break;
        } else {
            print_barber(prefix + " received a customer");
            pthread_mutex_unlock(&customer_mutex);
        }

        // Signal next in queue
        pthread_mutex_lock(&queue_mutex);
        queue_signalled = (queue_signalled + 1) % TOTAL_SEATS;
        pthread_cond_broadcast(&queue_cond);
        pthread_mutex_unlock(&queue_mutex);

        // Each service takes 10-30 milliseconds
        pthread_mutex_lock(&service_mutex);
        int service_duration = (int) random() % 20000 + 10000;
        service_counts[id - 1]++;
        service_times[id - 1] += service_duration;
        pthread_mutex_unlock(&service_mutex);

        print_barber(prefix + " is cutting hair");
        usleep(service_duration);
    } while (!should_exit());

    print_barber(prefix + " is leaving shop");
    return EXIT_SUCCESS;
}

void* customer_routine(void* customer_id) {
    int id = *(int*) customer_id;
    std::string prefix = "Customer [" + std::to_string(id) + "]";

    pthread_mutex_lock(&customer_mutex);
    // If there are empty seats
    if (current_customers < TOTAL_SEATS) {
        print_customer(prefix + " entered waiting room");

        // Take up a seat and wake up a barber if need be
        current_customers++;
        pthread_cond_broadcast(&customer_cond);
        pthread_mutex_unlock(&customer_mutex);
        print_customer(prefix + " is waiting for barber");

        // Waits for its turn to be serviced
        pthread_mutex_lock(&queue_mutex);
        queue_waiting = (queue_waiting + 1) % TOTAL_SEATS;
        unsigned int queue_position = queue_waiting;
        while (queue_signalled != queue_position && !should_exit())
            pthread_cond_wait(&queue_cond, &queue_mutex);

        if (!should_exit()) {
            print_customer(prefix + " is having hair cut");
            pthread_mutex_unlock(&queue_mutex);
        } else {
            pthread_mutex_unlock(&queue_mutex);
        }
    } else {
        pthread_mutex_unlock(&customer_mutex);
    }

    print_customer(prefix + " is leaving shop");
    return EXIT_SUCCESS;
}

void init_pthread_constructs() {
    pthread_cond_init(&queue_cond, nullptr);
    pthread_cond_init(&customer_cond, nullptr);
    pthread_mutex_init(&queue_mutex, nullptr);
    pthread_mutex_init(&customer_mutex, nullptr);
    pthread_mutex_init(&service_mutex, nullptr);
    pthread_mutex_init(&exit_mutex, nullptr);
}

void destroy_pthread_constructs() {
    pthread_cond_destroy(&queue_cond);
    pthread_cond_destroy(&customer_cond);
    pthread_mutex_destroy(&queue_mutex);
    pthread_mutex_destroy(&customer_mutex);
    pthread_mutex_destroy(&service_mutex);
    pthread_mutex_destroy(&exit_mutex);
}

void print_barber(const std::string& str) {
    std::string formatted = std::string(COLOUR_GREEN) + str + NO_COLOUR + "\n";
    std::cout << formatted;
}

void print_customer(const std::string& str) {
    std::string formatted = std::string(COLOUR_PURPLE) + str + NO_COLOUR + "\n";
    std::cout << formatted;
}

bool should_exit() {
    pthread_mutex_lock(&exit_mutex);
    bool value = can_exit;
    pthread_mutex_unlock(&exit_mutex);
    return value;
}

#endif
