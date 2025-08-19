// server.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <signal.h>
#include <errno.h>
#include "server.h"

const char *http_200 = "HTTP/1.1 200 OK\r\nContent-Type: %s\r\n\r\n";
const char *http_400 = "HTTP/1.1 400 BAD REQUEST\r\nContent-Type: text/html\r\n\r\n";
const char *http_404 = "HTTP/1.1 404 NOT FOUND\r\nContent-Type: text/html\r\n\r\n";
const char *http_500 = "HTTP/1.1 500 INTERNAL SERVER ERROR\r\nContent-Type: text/html\r\n\r\n";

const char *body_400 = "<html><body><h1>400 Bad Request</h1></body></html>";
const char *body_404 = "<html><body><h1>404 Not Found</h1></body></html>";
const char *body_500 = "<html><body><h1>500 Internal Server Error</h1></body></html>";

ClientQueue client_queue;

static pthread_t *thread_handles = NULL;
static volatile sig_atomic_t running = 1;
static int server_fd_global = -1;

static void handle_sigint(int sig) {
    (void)sig;
    running = 0;
    if (server_fd_global != -1) {
        close(server_fd_global);
    }
}


int create_server(int port) {
    int server_fd;
    struct sockaddr_in address;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    int reuse = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
        perror("setsockopt");
        log_error("setsockopt(SO_REUSEADDR) failed");
    }

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    return server_fd;
}

void start_server(Server* config) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_sigint;
    sigaction(SIGINT, &sa, NULL);

    int server_fd = create_server(config->port);
    server_fd_global = server_fd;

    thread_handles = malloc(sizeof(pthread_t) * config->num_threads);
    if (thread_handles == NULL) {
        perror("malloc failed");
        log_error("Failed to allocate thread handles");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    client_queue_init(&client_queue);

    for (int i = 0; i < config->num_threads; i++) {
        int rc = pthread_create(&thread_handles[i], NULL, worker_thread, config->file);
        if (rc != 0) {
            perror("pthread_create");
            log_error("pthread_create failed");
            for (int j = 0; j < i; j++) {
                pthread_cancel(thread_handles[j]);
                pthread_join(thread_handles[j], NULL);
            }
            free(thread_handles);
            close(server_fd);
            exit(EXIT_FAILURE);
        }
    }

    struct sockaddr_in address;
    int addrlen = sizeof(address);

    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    // If port was dynamically assigned (e.g., 0), query the bound port
    socklen_t len = sizeof(address);
    if (getsockname(server_fd, (struct sockaddr *)&address, &len) == 0) {
        config->port = ntohs(address.sin_port);
    }
    printf("Server listening on port %d\r\n", config->port);

    while (running) {
        int client_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        if (client_fd < 0) {
            if (errno == EINTR && !running) {
                break;
            }
            perror("accept");
            continue;
        }

        client_queue_push(&client_queue, client_fd);
    }

    for (int i = 0; i < config->num_threads; i++) {
        pthread_cancel(thread_handles[i]);
        pthread_join(thread_handles[i], NULL);
    }
    free(thread_handles);
    close(server_fd);
}

void handle_connection(int client_fd, char *filename) {
    char buffer[BUFFER_SIZE] = {0};

    int bytes_read = read(client_fd, buffer, BUFFER_SIZE - 1);
    if (bytes_read < 0) {
        perror("read");
        close(client_fd);
        return;
    }

    buffer[bytes_read] = '\0';

    if (!is_valid_request(buffer)) {
        write(client_fd, http_400, strlen(http_400));
        write(client_fd, body_400, strlen(body_400));
        close(client_fd);
        return;
    }

    char requested_path[BUFFER_SIZE] = {0};
    if (sscanf(buffer, "GET %2047s", requested_path) != 1) {
        write(client_fd, http_400, strlen(http_400));
        write(client_fd, body_400, strlen(body_400));
        close(client_fd);
        return;
    }

    char *query = strchr(requested_path, '?');
    if (query) {
        *query = '\0';
    }

    char *path = requested_path;
    if (path[0] == '/') {
        path++;
    }

    if (strstr(path, "..") != NULL) {
        write(client_fd, http_400, strlen(http_400));
        write(client_fd, body_400, strlen(body_400));
        close(client_fd);
        return;
    }

    char filepath[BUFFER_SIZE];
    if (*path == '\0') {
        strncpy(filepath, filename, sizeof(filepath));
        filepath[sizeof(filepath) - 1] = '\0';
    } else {
        strncpy(filepath, path, sizeof(filepath));
        filepath[sizeof(filepath) - 1] = '\0';
    }

    char response_header[512];
    const char *mime_type = get_mime_type(filepath);
    snprintf(response_header, sizeof(response_header), http_200, mime_type);

    FILE *fp = fopen(filepath, "rb");
    if (fp == NULL) {
        write(client_fd, http_404, strlen(http_404));
        write(client_fd, body_404, strlen(body_404));
    } else {
        int send_status = send_file(fp, client_fd, response_header);
        fclose(fp);

        if (send_status < 0) {
            write(client_fd, http_500, strlen(http_500));
            write(client_fd, body_500, strlen(body_500));
        }
    }
    close(client_fd);
}

double get_one_minute_load() {
    double load[1];
    if (getloadavg(load, 1) == -1) {
        perror("Failed to get load average");
        return -1.0;
    }
    return load[0];
}

double get_one_minute_load_from_server(Server server) {
    double load = get_one_minute_load();
    if (load < 0) {
        return -1.0;
    }
    // Normalize load based on server core count
    return load / server.core_count;
}


ServerPriority determine_priority(double one_min_load, int core_count) {
    if (one_min_load < 0.5 * core_count) {
        return HIGH_PRIORITY;
    } else if (one_min_load < core_count) {
        return MEDIUM_PRIORITY;
    } else {
        return LOW_PRIORITY;
    }
}

Server select_server(Server servers[], int num_servers) {
    Server *high_priority_servers = malloc(sizeof(Server) * num_servers);
    Server *medium_priority_servers = malloc(sizeof(Server) * num_servers);
    if (high_priority_servers == NULL || medium_priority_servers == NULL) {
        log_error("Failed to allocate memory for server selection");
        free(high_priority_servers);
        free(medium_priority_servers);
        return servers[rand() % num_servers];
    }
    int high_count = 0;
    int medium_count = 0;

    for (int i = 0; i < num_servers; i++) {
        double load = get_one_minute_load_from_server(servers[i]);
        ServerPriority priority = determine_priority(load, servers[i].core_count);
        if (priority == HIGH_PRIORITY) {
            high_priority_servers[high_count++] = servers[i];
        } else if (priority == MEDIUM_PRIORITY) {
            medium_priority_servers[medium_count++] = servers[i];
        }
    }

    /*
     * Select a server based on priority and copy it to a local variable
     * before freeing the priority arrays. This avoids returning a pointer
     * to freed memory.
     */
    Server selected_server;
    if (high_count > 0) {
        selected_server = high_priority_servers[rand() % high_count];
    } else if (medium_count > 0) {
        selected_server = medium_priority_servers[rand() % medium_count];
    } else {
        selected_server = servers[rand() % num_servers];
    }
    free(medium_priority_servers);
    free(high_priority_servers);

    return selected_server;
}
