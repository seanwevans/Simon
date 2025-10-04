// request.c

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
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
#if !defined(MSG_NOSIGNAL) && defined(SO_NOSIGPIPE)
    int set = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_NOSIGPIPE, &set, sizeof(set)) < 0) {
        if (errno != EINVAL && errno != ENOPROTOOPT
#ifdef ENOTSUP
            && errno != ENOTSUP
#endif
        ) {
            perror("setsockopt(SO_NOSIGPIPE)");
        }
    }
#endif

    size_t total = 0;
    while (total < len) {
        int flags = 0;
#ifdef MSG_NOSIGNAL
        flags = MSG_NOSIGNAL;
#endif
        ssize_t sent = send(sockfd, buf + total, len - total, flags);
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
        int send_errno = errno;
        if (send_errno == EPIPE) {
            log_error("Client disconnected while sending HTTP header");
        } else {
            perror("Failed to send HTTP header");
            log_error("Failed to send HTTP header");
        }
        errno = send_errno;
        return -1;
    }

    while ((n = fread(data, sizeof(char), BUFFER_SIZE, fp)) > 0) {
        if (send_all(sockfd, data, n) == -1) {
            int send_errno = errno;
            if (send_errno == EPIPE) {
                log_error("Client disconnected while sending file data");
            } else {
                perror("Failed to send file");
                log_error("Failed to send file");
            }
            errno = send_errno;
            return -1;
        }
    }
    return 0;
}

int send_chunked_file(FILE *fp, int sockfd, const char *header) {
    char data[BUFFER_SIZE];
    int n;

    if (send_all(sockfd, header, strlen(header)) == -1) {
        int send_errno = errno;
        if (send_errno == EPIPE) {
            log_error("Client disconnected while sending chunked header");
        } else {
            perror("Failed to send HTTP header");
            log_error("Failed to send HTTP header");
        }
        errno = send_errno;
        return -1;
    }

    while ((n = fread(data, sizeof(char), BUFFER_SIZE, fp)) > 0) {
        char chunk_size[10];
        snprintf(chunk_size, sizeof(chunk_size), "%x\r\n", n);
        if (send_all(sockfd, chunk_size, strlen(chunk_size)) == -1) {
            int send_errno = errno;
            if (send_errno == EPIPE) {
                log_error("Client disconnected while sending chunk size");
            } else {
                perror("Failed to send chunk size");
                log_error("Failed to send chunk size");
            }
            errno = send_errno;
            return -1;
        }

        if (send_all(sockfd, data, n) == -1) {
            int send_errno = errno;
            if (send_errno == EPIPE) {
                log_error("Client disconnected while sending chunk data");
            } else {
                perror("Failed to send chunk data");
                log_error("Failed to send chunk data");
            }
            errno = send_errno;
            return -1;
        }

        if (send_all(sockfd, "\r\n", 2) == -1) {
            int send_errno = errno;
            if (send_errno == EPIPE) {
                log_error("Client disconnected before CRLF could be sent");
            } else {
                perror("Failed to send CRLF after chunk");
                log_error("Failed to send CRLF after chunk");
            }
            errno = send_errno;
            return -1;
        }
    }

    if (send_all(sockfd, "0\r\n\r\n", 5) == -1) {
        int send_errno = errno;
        if (send_errno == EPIPE) {
            log_error("Client disconnected before end of chunked data");
        } else {
            perror("Failed to send end of chunked data");
            log_error("Failed to send end of chunked data");
        }
        errno = send_errno;
        return -1;
    }

    return 0;
}
