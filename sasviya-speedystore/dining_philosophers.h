#ifndef DINING_PHILO
#define DINING_PHILO

#include <pthread.h>
#include <stdbool.h>

// macros
#define NUM_PHILOSOPHERS 5

// data types
typedef struct {
    int index;
    pthread_mutex_t mutex;
    pthread_mutex_t cvMutex;
    pthread_cond_t condVar;
} Utensil;

// variables
static pthread_t philosophers[NUM_PHILOSOPHERS] = {0};
static Utensil utensils[NUM_PHILOSOPHERS] = {0};
static int maxWaitTimeSec = 5;
static bool runThreads = true;

static void* threadFunc(void* arg);

static bool acquireUtensil(int philosopherIndex, int utensilIndex);
static void freeUtensil(int philosopherIndex, int utensilIndex);

static void signalHandler(int signal);

int main();

#endif /*DINING_PHILO*/