#include "dining_philosophers.h"

#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

void* thread_func(void* arg) {
    if (arg == NULL) {
        fprintf(p_log_file, "Error: NULL argument passed to thread_func\n");
        return NULL;
    }
    int philosopher_index = *(int*)arg;
    int left = philosopher_index;
    int right = (philosopher_index + 1) % NUM_PHILOSOPHERS;
    // for case where left > right, swap the order of picking up utensils to avoid deadlock
    int first = left < right ? left : right;
    int second = left < right ? right : left;

    while (run_threads) {
        // simulate thinking
        int think_time = rand() % max_think_time_sec + 1;
        printf("Philosopher %d is thinking for %d seconds\n", philosopher_index, think_time);
        fflush(stdout);
        fprintf(p_log_file, "Philosopher %d is thinking for %d seconds\n", philosopher_index, think_time);
        wait_for_time_or_signal(philosopher_index, think_time);
        fprintf(p_log_file, "Philosopher %d is hungry and wants to eat\n", philosopher_index);

        if (!run_threads) {
            // if we received signal to stop, don't acquire utensils and exit
            fprintf(p_log_file, "Philosopher %d is exiting without acquiring utensil %d\n", philosopher_index, first);
            break;
        }

        // wait for first utensil
        if (!acquire_utensil(philosopher_index, first, true /*wait*/)) {
            fprintf(p_log_file, "A Philosopher %d failed to acquire utensil %d\n", philosopher_index, first);
            // failed to acquire first utensil, just restart loop
            continue;
        }
        fprintf(p_log_file, "A Philosopher %d has acquired utensil %d\n", philosopher_index, first);

        if (!run_threads) {
            // if we received signal to stop, release utensil and exit
            fprintf(p_log_file, "Philosopher %d is exiting and releasing utensil %d\n", philosopher_index, first);
            free_utensil(philosopher_index, first);
            break;
        }

        // wait for second utensil
        if (!acquire_utensil(philosopher_index, second, false /*don't wait*/)) {
            // failed to acquire second utensil, free first and restart loop
            fprintf(p_log_file, "A Philosopher %d failed to acquire utensil %d\n", philosopher_index, second);
            free_utensil(philosopher_index, first);
            continue;
        }
        fprintf(p_log_file, "A Philosopher %d has utensils %d and %d\n", philosopher_index, first, second);

        if (!run_threads) {
            // if we received signal to stop, release utensils and exit
            fprintf(p_log_file, "Philosopher %d is exiting and releasing utensils %d and %d\n", philosopher_index,
                    first, second);
            free_utensil(philosopher_index, first);
            free_utensil(philosopher_index, second);
            break;
        }

        // we have both utensils now, time to eat
        int eat_time = rand() % max_eat_time_sec + 1;
        printf("Philosopher %d has started eating with utensils %d and %d for %d seconds\n", philosopher_index, first,
               second, eat_time);
        fflush(stdout);
        fprintf(p_log_file, "Philosopher %d has started eating with utensils %d and %d for %d seconds\n",
                philosopher_index, first, second, eat_time);
        wait_for_time_or_signal(philosopher_index, eat_time);

        // done eating, put down utensils
        free_utensil(philosopher_index, first);
        free_utensil(philosopher_index, second);
        fprintf(p_log_file, "Philosopher %d has finished eating and put down utensils %d and %d\n", philosopher_index,
                first, second);
    }
    return NULL;
}

void wait_for_time_or_signal(int philosopher_index, int time) {
    struct timespec time_spec;
    clock_gettime(CLOCK_REALTIME, &time_spec);
    time_spec.tv_sec += time;
    pthread_cond_timedwait(&utensils[philosopher_index].shutdown_cond_var, &utensils[philosopher_index].shutdown_mutex,
                           &time_spec);
    pthread_mutex_unlock(&utensils[philosopher_index].shutdown_mutex);
}

bool acquire_utensil(int philosopher_index, int utensil_index, bool wait) {
    fprintf(p_log_file, "A Philosopher %d is trying to acquire utensil %d\n", philosopher_index, utensil_index);
    // wait for utensil to be free
    pthread_mutex_lock(&utensils[utensil_index].cv_mutex);
    int status = pthread_mutex_trylock(&utensils[utensil_index].mutex);
    if (status != EBUSY) {
        // acquired the utensil
        pthread_mutex_unlock(&utensils[utensil_index].cv_mutex);
        return true;
    }
    if (!wait) {
        // don't wait, just return failure
        pthread_mutex_unlock(&utensils[utensil_index].cv_mutex);
        return false;
    }
    // wait to be signaled and release cvMutex
    pthread_cond_wait(&utensils[utensil_index].cond_var, &utensils[utensil_index].cv_mutex);
    // re-acquired the cvMutex
    // acquire the utensil after waiting
    status = pthread_mutex_trylock(&utensils[utensil_index].mutex);
    pthread_mutex_unlock(&utensils[utensil_index].cv_mutex);
    return status != EBUSY;
}

void free_utensil(int philosopher_index, int utensil_index) {
    printf("Philosopher %d is releasing utensil %d\n", philosopher_index, utensil_index);
    fflush(stdout);
    fprintf(p_log_file, "Philosopher %d is releasing utensil %d\n", philosopher_index, utensil_index);
    pthread_mutex_unlock(&utensils[utensil_index].mutex);
    pthread_cond_signal(&utensils[utensil_index].cond_var);
}

void signal_handler(int signal) {
    printf(" received signal %d\n", signal);
    fprintf(p_log_file, "received signal %d\n", signal);
    run_threads = false;

    for (int i = 0; i < NUM_PHILOSOPHERS; i++) {
        // tell all philosophers to stop waiting and exit
        pthread_cond_broadcast(&utensils[i].shutdown_cond_var);
    }
}

int main() {
    printf("Enter\n");
    // register signal handler
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    srand(time(NULL));  // use current time as seed for random generator
    p_log_file = fopen("output.log", "w");
    if (p_log_file == NULL) {
        printf("Error: Unable to open log file\n");
        return -1;
    }

    for (int i = 0; i < NUM_PHILOSOPHERS; i++) {
        utensils[i].index = i;
        utensils[i].mutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
        utensils[i].shutdown_mutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
        utensils[i].shutdown_cond_var = (pthread_cond_t)PTHREAD_COND_INITIALIZER;
        utensils[i].cv_mutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
        utensils[i].cond_var = (pthread_cond_t)PTHREAD_COND_INITIALIZER;
    }

    for (int i = 0; i < NUM_PHILOSOPHERS; i++) {
        pthread_create(&philosophers[i], NULL, thread_func, &utensils[i].index);
    }

    // wait for threads to finish
    for (int i = 0; i < NUM_PHILOSOPHERS; i++) {
        pthread_join(philosophers[i], NULL);
    }

    // free resources
    for (int i = 0; i < NUM_PHILOSOPHERS; i++) {
        pthread_mutex_destroy(&utensils[i].mutex);
        pthread_mutex_destroy(&utensils[i].shutdown_mutex);
        pthread_cond_destroy(&utensils[i].shutdown_cond_var);
        pthread_mutex_destroy(&utensils[i].cv_mutex);
        pthread_cond_destroy(&utensils[i].cond_var);
    }
    fclose(p_log_file);
    printf("Exiting\n");
    return 0;
}