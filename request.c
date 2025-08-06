// request.c

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "server.h"

int is_valid_request(const char *request) {
    const char *ptr;
    char version[16];

    if (strncmp(request, "GET ", 4) != 0) {
        return 0;
    }

    ptr = strchr(request + 4, ' ');
    if (!ptr) {
        return 0;
    }

    if (sscanf(ptr + 1, "%15s", version) != 1) {
        return 0;
    }

    if (strcmp(version, "HTTP/1.1") != 0 && strcmp(version, "HTTP/1.0") != 0) {
        return 0;
    }

    return 1;
}

int send_file(FILE *fp, int sockfd, const char *header) {
    char data[BUFFER_SIZE] = {0};
    int n;

    if (send(sockfd, header, strlen(header), 0) == -1) {
        perror("Failed to send HTTP header");
        return -1;
    }

    while ((n = fread(data, sizeof(char), BUFFER_SIZE, fp)) > 0) {
        if (send(sockfd, data, n, 0) == -1) {
            perror("Failed to send file");
            return -1;
        }

        memset(data, 0, BUFFER_SIZE);
    }
    return 0;
}

int send_chunked_file(FILE *fp, int sockfd, const char *header) {
    char data[BUFFER_SIZE] = {0};
    int n;

    if (send(sockfd, header, strlen(header), 0) == -1) {
        perror("Failed to send HTTP header");
        return -1;
    }

    while ((n = fread(data, sizeof(char), BUFFER_SIZE, fp)) > 0) {
        char chunk_size[10];
        snprintf(chunk_size, sizeof(chunk_size), "%x\r\n", n);
        if (send(sockfd, chunk_size, strlen(chunk_size), 0) == -1) {
            perror("Failed to send chunk size");
            return -1;
        }

        if (send(sockfd, data, n, 0) == -1) {
            perror("Failed to send chunk data");
            return -1;
        }

        if (send(sockfd, "\r\n", 2, 0) == -1) {
            perror("Failed to send CRLF after chunk");
            return -1;
        }

        memset(data, 0, BUFFER_SIZE);
    }

    if (send(sockfd, "0\r\n\r\n", 5, 0) == -1) {
        perror("Failed to send end of chunked data");
        return -1;
    }

    return 0;
}
