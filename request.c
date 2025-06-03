// request.c

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "server.h"

int is_valid_request(const char *request) {
    return (strstr(request, "GET") == request) && (strstr(request, "HTTP/1.1") || strstr(request, "HTTP/1.0"));
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

        bzero(data, BUFFER_SIZE);
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

        bzero(data, BUFFER_SIZE);
    }

    if (send(sockfd, "0\r\n\r\n", 5, 0) == -1) {
        perror("Failed to send end of chunked data");
        return -1;
    }

    return 0;
}
