#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#define ERROR_CREATE_THREAD 1
#define ERROR_JOIN_THREAD 2
#define SUCCESS 0

typedef struct {
    pthread_cond_t cond;
    pthread_mutex_t lock;
    bool ready;
    int value;
} Monitor;

Monitor monitor = {
    .cond = PTHREAD_COND_INITIALIZER,
    .lock = PTHREAD_MUTEX_INITIALIZER,
    .ready = false,
    .value = 0
};

void* produce(void* arg) {
    for (;;) {
        int value = rand();
        pthread_mutex_lock(&monitor.lock);

        while (monitor.ready) {
            pthread_cond_wait(&monitor.cond, &monitor.lock);
        }

        monitor.value = value;
        monitor.ready = true;
        printf("produce value: %d\n", monitor.value);

        pthread_cond_signal(&monitor.cond);
        pthread_mutex_unlock(&monitor.lock);

        usleep(100000); // 100 ms delay
    }
}

void* consume(void* arg) {
    for (;;) {
        pthread_mutex_lock(&monitor.lock);

        while (!monitor.ready) {
            pthread_cond_wait(&monitor.cond, &monitor.lock);
        }

        monitor.ready = false;
        printf("consume value: %d\n", monitor.value);

        pthread_cond_signal(&monitor.cond);
        pthread_mutex_unlock(&monitor.lock);

        usleep(100000); // 100 ms delay
    }
}

int main() {
    pthread_t provider;
    pthread_t consumer;
    int status = 0;

    status = pthread_create(&provider, NULL, produce, NULL);
    if (status != 0) {
        fprintf(stderr, "Error creating thread\n");
        exit(ERROR_CREATE_THREAD);
    }

    status = pthread_create(&consumer, NULL, consume, NULL);
    if (status != 0) {
        fprintf(stderr, "Error creating thread\n");
        exit(ERROR_CREATE_THREAD);
    }

    pthread_join(provider, NULL);
    if (status != SUCCESS) {
        fprintf(stderr, "Error joining thread\n");
        exit(ERROR_JOIN_THREAD);
    }

    pthread_join(consumer, NULL);
    if (status != SUCCESS) {
        fprintf(stderr, "Error joining thread\n");
        exit(ERROR_JOIN_THREAD);
    }

    return 0;
}
