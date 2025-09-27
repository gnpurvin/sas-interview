#include "dining_philosophers.h"

#include <errno.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

void* threadFunc(void* arg) {
    if (arg == NULL) {
        fprintf(pLogFile, "Error: NULL argument passed to threadFunc\n");
        return NULL;
    }
    int philosopherIndex = *(int*)arg;
    int left = philosopherIndex;
    int right = (philosopherIndex + 1) % NUM_PHILOSOPHERS;
    // for case where left > right, swap the order of picking up utensils to avoid deadlock
    int first = left < right ? left : right;
    int second = left < right ? right : left;

    while (runThreads) {
        // simulate thinking
        int thinkTime = rand() % maxThinkTimeSec + 1;
        fprintf(pLogFile, "Philosopher %d is thinking for %d seconds\n", philosopherIndex, thinkTime);
        sleep(thinkTime);
        fprintf(pLogFile, "Philosopher %d is hungry and wants to eat\n", philosopherIndex);

        if (!runThreads) {
            // if we received signal to stop, don't acquire utensils and exit
            fprintf(pLogFile, "Philosopher %d is exiting without acquiring utensil %d\n", philosopherIndex, first);
            break;
        }

        // wait for first utensil
        if (!acquireUtensil(philosopherIndex, first)) {
            fprintf(pLogFile, "A Philosopher %d failed to acquire utensil %d\n", philosopherIndex, first);
            // failed to acquire first utensil, just restart loop
            continue;
        }
        fprintf(pLogFile, "A Philosopher %d has acquired utensil %d\n", philosopherIndex, first);

        if (!runThreads) {
            // if we received signal to stop, release utensil and exit
            fprintf(pLogFile, "Philosopher %d is exiting and releasing utensil %d\n", philosopherIndex, first);
            freeUtensil(philosopherIndex, first);
            break;
        }

        // wait for second utensil
        if (!acquireUtensil(philosopherIndex, second)) {
            // failed to acquire second utensil, free first and restart loop
            fprintf(pLogFile, "A Philosopher %d failed to acquire utensil %d\n", philosopherIndex, second);
            freeUtensil(philosopherIndex, first);
            continue;
        }
        fprintf(pLogFile, "A Philosopher %d has utensils %d and %d\n", philosopherIndex, first, second);

        if (!runThreads) {
            // if we received signal to stop, release utensils and exit
            fprintf(pLogFile, "Philosopher %d is exiting and releasing utensils %d and %d\n", philosopherIndex, first,
                    second);
            freeUtensil(philosopherIndex, first);
            freeUtensil(philosopherIndex, second);
            break;
        }

        // we have both utensils now, time to eat
        int eatTime = rand() % maxEatTimeSec + 1;
        printf("Philosopher %d has started eating with utensils %d and %d for %d seconds\n", philosopherIndex, first,
               second, eatTime);
        fflush(stdout);
        fprintf(pLogFile, "Philosopher %d has started eating with utensils %d and %d for %d seconds\n",
                philosopherIndex, first, second, eatTime);
        // TODO: do we need to interrupt sleep if we receive signal to stop?
        sleep(eatTime);

        // done eating, put down utensils
        freeUtensil(philosopherIndex, first);
        freeUtensil(philosopherIndex, second);
        fprintf(pLogFile, "Philosopher %d has finished eating and put down utensils %d and %d\n", philosopherIndex,
                first, second);
    }
    return NULL;
}

bool acquireUtensil(int philosopherIndex, int utensilIndex) {
    fprintf(pLogFile, "A Philosopher %d is trying to acquire utensil %d\n", philosopherIndex, utensilIndex);
    // wait for utensil to be free
    pthread_mutex_lock(&utensils[utensilIndex].cvMutex);
    int status = pthread_mutex_trylock(&utensils[utensilIndex].mutex);
    if (status != EBUSY) {
        // acquired the utensil
        pthread_mutex_unlock(&utensils[utensilIndex].cvMutex);
        return true;
    }
    // wait to be signaled and release cvMutex
    pthread_cond_wait(&utensils[utensilIndex].condVar, &utensils[utensilIndex].cvMutex);
    // re-acquired the cvMutex
    // acquire the utensil after waiting
    status = pthread_mutex_trylock(&utensils[utensilIndex].mutex);
    pthread_mutex_unlock(&utensils[utensilIndex].cvMutex);
    return status != EBUSY;
}

void freeUtensil(int philosopherIndex, int utensilIndex) {
    printf("Philosopher %d is releasing utensil %d\n", philosopherIndex, utensilIndex);
    fflush(stdout);
    fprintf(pLogFile, "Philosopher %d is releasing utensil %d\n", philosopherIndex, utensilIndex);
    pthread_mutex_unlock(&utensils[utensilIndex].mutex);
    pthread_cond_signal(&utensils[utensilIndex].condVar);
}

void signalHandler(int signal) {
    printf(" received signal %d\n", signal);
    fprintf(pLogFile, "received signal %d\n", signal);
    runThreads = false;
}

int main() {
    printf("Enter\n");
    // register signal handler
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    srand(time(NULL));  // use current time as seed for random generator
    pLogFile = fopen("output.log", "w");
    if (pLogFile == NULL) {
        printf("Error: Unable to open log file\n");
        return -1;
    }

    for (int i = 0; i < NUM_PHILOSOPHERS; i++) {
        // TODO: do we need to set any attributes related to scheduling to avoid starvation?
        utensils[i].index = i;
        utensils[i].mutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
        utensils[i].cvMutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
        utensils[i].condVar = (pthread_cond_t)PTHREAD_COND_INITIALIZER;
        pthread_create(&philosophers[i], NULL, threadFunc, &utensils[i].index);
    }

    // wait for threads to finish
    for (int i = 0; i < NUM_PHILOSOPHERS; i++) {
        pthread_join(philosophers[i], NULL);
    }

    // free resources
    for (int i = 0; i < NUM_PHILOSOPHERS; i++) {
        pthread_mutex_destroy(&utensils[i].mutex);
        pthread_mutex_destroy(&utensils[i].cvMutex);
        pthread_cond_destroy(&utensils[i].condVar);
    }
    fclose(pLogFile);
    printf("Exiting\n");
    return 0;
}