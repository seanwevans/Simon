// server.h

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h> 

#ifndef SWE_SERVER_H
#define SWE_SERVER_H

#define DEFAULT_PORT 8080
#define BUFFER_SIZE 2048
#define MAX_QUEUE_SIZE 1024
#define NUM_THREADS 8


typedef struct {
    char *file;
    int port;
    int core_count;
    int num_threads;    
} Server;

typedef enum {
    HIGH_PRIORITY,
    MEDIUM_PRIORITY,
    LOW_PRIORITY
} ServerPriority;

typedef struct {
    int sockets[MAX_QUEUE_SIZE];
    int front;
    int rear;
    sem_t filled;
    sem_t empty;
    pthread_mutex_t mutex;
} ClientQueue;

extern const char *http_200;

extern const char *http_400;
extern const char *http_404;
extern const char *http_500;

extern const char *body_400;
extern const char *body_404;
extern const char *body_500;

extern ClientQueue client_queue;

// server
int create_server(int port);
void start_server(Server* config);
void handle_connection(int client_fd, char *filename);
double get_one_minute_load();
ServerPriority determine_priority(double one_min_load, int core_count);
Server select_server(Server servers[], int num_servers);

// request
int is_valid_request(const char *request);
int send_file(FILE *fp, int sockfd);
int send_chunked_file(FILE *fp, int sockfd);

// logging
void log_message(const char *filename, const char *message);
void log_error(const char *message);
void log_connection(const char *message);

// queue
void client_queue_init(ClientQueue *q);
void client_queue_push(ClientQueue *q, int client_fd);
int client_queue_pop(ClientQueue *q);
void *worker_thread(void *arg);
void handle_connection(int client_fd, char *filename);

// utils
void parse_arguments(int argc, char *argv[], Server *config);
const char* get_mime_type(const char *filename);


#endif  // SWE_SERVER_H
