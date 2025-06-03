// server.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "server.h"

const char *http_200 = "HTTP/1.1 200 OK\r\nContent-Type: %s\r\n\r\n";
const char *http_400 = "HTTP/1.1 400 BAD REQUEST\r\nContent-Type: text/html\r\n\r\n";
const char *http_404 = "HTTP/1.1 404 NOT FOUND\r\nContent-Type: text/html\r\n\r\n";
const char *http_500 = "HTTP/1.1 500 INTERNAL SERVER ERROR\r\nContent-Type: text/html\r\n\r\n";

const char *body_400 = "<html><body><h1>400 Bad Request</h1></body></html>";
const char *body_404 = "<html><body><h1>404 Not Found</h1></body></html>";
const char *body_500 = "<html><body><h1>500 Internal Server Error</h1></body></html>";

ClientQueue client_queue;


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

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    return server_fd;
}

void start_server(Server* config) {
    int server_fd = create_server(config->port);

    pthread_t threads[NUM_THREADS];

    client_queue_init(&client_queue);

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_create(&threads[i], NULL, worker_thread, config->file);
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

    while (1) {
        int client_fd;
        if ((client_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        client_queue_push(&client_queue, client_fd);
    }
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

    char response_header[512];
    const char *mime_type = get_mime_type(filename);    
    snprintf(response_header, sizeof(response_header), http_200, mime_type);

    FILE *fp = fopen(filename, "rb");
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

    Server *server;
    if (high_count > 0) {
        server = &high_priority_servers[rand() % high_count];
    } else if (medium_count > 0) {        
        server = &medium_priority_servers[rand() % medium_count];
    } else {
        server = &servers[rand() % num_servers];
    }
    free(medium_priority_servers);
    free(high_priority_servers);

    return *server;
}
