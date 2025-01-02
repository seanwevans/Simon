// queue.c

#include <stddef.h>

#include "server.h"



void client_queue_init(ClientQueue *q) {
    q->front = 0;
    q->rear = 0;
    sem_init(&q->filled, 0, 0);
    sem_init(&q->empty, 0, MAX_QUEUE_SIZE);
    pthread_mutex_init(&q->mutex, NULL);
}

void client_queue_push(ClientQueue *q, int client_fd) {
    sem_wait(&q->empty);
    pthread_mutex_lock(&q->mutex);

    q->sockets[q->rear] = client_fd;
    q->rear = (q->rear + 1) % MAX_QUEUE_SIZE;

    pthread_mutex_unlock(&q->mutex);
    sem_post(&q->filled);
}

int client_queue_pop(ClientQueue *q) {
    sem_wait(&q->filled);
    pthread_mutex_lock(&q->mutex);

    int client_fd = q->sockets[q->front];
    q->front = (q->front + 1) % MAX_QUEUE_SIZE;

    pthread_mutex_unlock(&q->mutex);
    sem_post(&q->empty);

    return client_fd;
}

void *worker_thread(void *arg) {
    char *filename = (char *)arg;
    while (1) {
        int client_fd = client_queue_pop(&client_queue);
        handle_connection(client_fd, filename);
    }
    return NULL;
}
