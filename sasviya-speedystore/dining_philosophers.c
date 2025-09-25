#include "dining_philosophers.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void* threadFunc(void* arg) {
    if (arg == NULL) {
        printf("Error: NULL argument passed to threadFunc\n");
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
        int thinkTime = rand() % maxWaitTimeSec + 1;
        printf("Philosopher %d is thinking for %d seconds\n", philosopherIndex, thinkTime);
        sleep(thinkTime);

        // try to pick up utensils

        // wait for first utensil
        if (!acquireUtensil(philosopherIndex, first)) {
            printf("A Philosopher %d failed to acquire utensil %d\n", philosopherIndex, first);
            // failed to acquire first utensil, just restart loop
            continue;
        }

        printf("A Philosopher %d has acquired utensil %d\n", philosopherIndex, first);

        // wait for second utensil
        if (!acquireUtensil(philosopherIndex, second)) {
            // failed to acquire second utensil, free first and restart loop
            printf("A Philosopher %d failed to acquire utensil %d\n", philosopherIndex, second);
            freeUtensil(philosopherIndex, first);
            continue;
        }

        printf("A Philosopher %d has utensils %d and %d\n", philosopherIndex, first, second);

        // we have both utensils now, time to eat
        int eatTime = rand() % maxWaitTimeSec + 1;
        printf("Philosopher %d is eating for %d seconds\n", philosopherIndex, eatTime);
        sleep(eatTime);

        // done eating, put down utensils
        freeUtensil(philosopherIndex, first);
        freeUtensil(philosopherIndex, second);
    }
    return NULL;
}

bool acquireUtensil(int philosopherIndex, int utensilIndex) {
    printf("A Philosopher %d is trying to acquire utensil %d\n", philosopherIndex, utensilIndex);
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
    pthread_mutex_unlock(&utensils[utensilIndex].mutex);
    pthread_cond_signal(&utensils[utensilIndex].condVar);
}

void signalHandler(int signal) {
    printf(" received signal %d\n", signal);
    runThreads = false;
}

int main() {
    printf("Enter\n");
    // register signal handler
    signal(SIGINT, signalHandler);
    srand(time(NULL));  // use current time as seed for random generator

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
    printf("Exiting\n");
    return 0;
}