// request.c

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
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


static int send_all(int sockfd, const char *buf, size_t len) {
    size_t total = 0;
    while (total < len) {
        ssize_t sent = send(sockfd, buf + total, len - total, 0);
        if (sent < 0) {
            if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
                continue;
            }
            return -1;
        }
        total += (size_t)sent;
    }
    return 0;
}

int send_file(FILE *fp, int sockfd, const char *header) {
    char data[BUFFER_SIZE];
    int n;

    if (send_all(sockfd, header, strlen(header)) == -1) {
        perror("Failed to send HTTP header");
        return -1;
    }

    while ((n = fread(data, sizeof(char), BUFFER_SIZE, fp)) > 0) {
        if (send_all(sockfd, data, n) == -1) {
            perror("Failed to send file");
            return -1;
        }
    }
    return 0;
}

int send_chunked_file(FILE *fp, int sockfd, const char *header) {
    char data[BUFFER_SIZE];
    int n;

    if (send_all(sockfd, header, strlen(header)) == -1) {
        perror("Failed to send HTTP header");
        return -1;
    }

    while ((n = fread(data, sizeof(char), BUFFER_SIZE, fp)) > 0) {
        char chunk_size[10];
        snprintf(chunk_size, sizeof(chunk_size), "%x\r\n", n);
        if (send_all(sockfd, chunk_size, strlen(chunk_size)) == -1) {
            perror("Failed to send chunk size");
            return -1;
        }

        if (send_all(sockfd, data, n) == -1) {
            perror("Failed to send chunk data");
            return -1;
        }

        if (send_all(sockfd, "\r\n", 2) == -1) {
            perror("Failed to send CRLF after chunk");
            return -1;
        }
    }

    if (send_all(sockfd, "0\r\n\r\n", 5) == -1) {
        perror("Failed to send end of chunked data");
        return -1;
    }

    return 0;
}
