#ifndef DINING_PHILO
#define DINING_PHILO

#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>

// macros
#define NUM_PHILOSOPHERS 5

// data types
typedef struct {
    int index;
    // mutex for the utensil
    pthread_mutex_t mutex;
    // mutex & condvar to wait on while eating, so we can be interrupted to stop
    pthread_mutex_t shutdown_mutex;
    pthread_cond_t shutdown_cond_var;
    // for waiting for the main mutex to become available
    pthread_mutex_t cv_mutex;
    pthread_cond_t cond_var;
} Utensil;

// variables
static pthread_t philosophers[NUM_PHILOSOPHERS] = {0};
static Utensil utensils[NUM_PHILOSOPHERS] = {0};
static int max_think_time_sec = 5;
static int max_eat_time_sec = 5;
static bool run_threads = true;
static FILE* p_log_file = NULL;

static void* thread_func(void* arg);
static void wait_for_time_or_signal(int philosopher_index, int time);
static bool acquire_utensil(int philosopher_index, int utensil_index);
static void free_utensil(int philosopher_index, int utensil_index);
static void signal_handler(int signal);

int main();

#endif /*DINING_PHILO*/