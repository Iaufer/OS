#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#define ERROR_CREATE_THREAD 1
#define ERROR_JOIN_THREAD 2
#define SUCCESS 0

typedef struct {
    pthread_cond_t condition;
    pthread_mutex_t mutex;
    bool is_ready;
    int data;
} MyMonitor;

MyMonitor my_monitor = {
    .condition = PTHREAD_COND_INITIALIZER,
    .mutex = PTHREAD_MUTEX_INITIALIZER,
    .is_ready = false,
    .data = 0
};

void* producer_function(void* arg) {
    for (;;) {
        int value = rand();
        pthread_mutex_lock(&my_monitor.mutex);

        if(my_monitor.is_ready){
            pthread_mutex_unlock(&my_monitor.mutex);
            continue;
        }

        my_monitor.data = value;
        my_monitor.is_ready = true;
        printf("Producer produces data: %d\n", my_monitor.data);

        pthread_cond_signal(&my_monitor.condition);
        pthread_mutex_unlock(&my_monitor.mutex);

        usleep(100000); // 100 ms delay
    }
}

void* consumer_function(void* arg) {
    for (;;) {
        pthread_mutex_lock(&my_monitor.mutex);

        while (!my_monitor.is_ready) {
            pthread_cond_wait(&my_monitor.condition, &my_monitor.mutex);
        }

        my_monitor.is_ready = false;
        printf("Consumer consumes data: %d\n", my_monitor.data);

        pthread_mutex_unlock(&my_monitor.mutex);

        usleep(100000); // 100 ms delay
    }
}

int main() {
    pthread_t producer_thread;
    pthread_t consumer_thread;
    int status = 0;

    status = pthread_create(&producer_thread, NULL, producer_function, NULL);
    if (status != 0) {
        fprintf(stderr, "Error creating producer thread\n");
        exit(ERROR_CREATE_THREAD);
    }

    status = pthread_create(&consumer_thread, NULL, consumer_function, NULL);
    if (status != 0) {
        fprintf(stderr, "Error creating consumer thread\n");
        exit(ERROR_CREATE_THREAD);
    }

    pthread_join(producer_thread, NULL);
    if (status != SUCCESS) {
        fprintf(stderr, "Error joining producer thread\n");
        exit(ERROR_JOIN_THREAD);
    }

    pthread_join(consumer_thread, NULL);
    if (status != SUCCESS) {
        fprintf(stderr, "Error joining consumer thread\n");
        exit(ERROR_JOIN_THREAD);
    }

    return 0;
}
