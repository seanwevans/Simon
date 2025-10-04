// utils.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <sys/stat.h>
#include "server.h"

void parse_arguments(int argc, char *argv[], Server *config) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <filename> [port] [core_count] [num_threads] [docroot]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *default_file_arg = argv[1];
    while (*default_file_arg == '/') {
        default_file_arg++;
    }

    if (*default_file_arg == '\0') {
        fprintf(stderr, "Default file must not be empty\n");
        exit(EXIT_FAILURE);
    }

    size_t default_len = strlen(default_file_arg);
    config->file = malloc(default_len + 1);
    if (config->file == NULL) {
        perror("Failed to allocate memory for default file path");
        exit(EXIT_FAILURE);
    }
    memcpy(config->file, default_file_arg, default_len + 1);

    if (argc > 2) {
        char *endptr;
        errno = 0;
        long port = strtol(argv[2], &endptr, 10);
        if (errno != 0 || *endptr != '\0' || port < 1 || port > 65535) {
            fprintf(stderr, "Invalid port number: %s\n", argv[2]);
            exit(EXIT_FAILURE);
        }
        config->port = (int)port;
    } else {
        config->port = DEFAULT_PORT;
    }

    if (argc > 3) {
        char *endptr;
        errno = 0;
        long cores = strtol(argv[3], &endptr, 10);
        if (errno != 0 || *endptr != '\0' || cores <= 0) {
            fprintf(stderr, "Invalid core count: %s\n", argv[3]);
            exit(EXIT_FAILURE);
        }
        config->core_count = (int)cores;
    } else {
        config->core_count = 16;
    }

    if (argc > 4) {
        char *endptr;
        errno = 0;
        long threads = strtol(argv[4], &endptr, 10);
        if (errno != 0 || *endptr != '\0' || threads <= 0) {
            fprintf(stderr, "Invalid thread count: %s\n", argv[4]);
            exit(EXIT_FAILURE);
        }
        config->num_threads = (int)threads;
    } else {
        config->num_threads = NUM_THREADS;
    }

    const char *docroot_arg = (argc > 5) ? argv[5] : ".";
    char resolved_docroot[PATH_MAX];
    if (realpath(docroot_arg, resolved_docroot) == NULL) {
        fprintf(stderr, "Invalid document root: %s\n", docroot_arg);
        exit(EXIT_FAILURE);
    }

    struct stat docroot_info;
    if (stat(resolved_docroot, &docroot_info) != 0 || !S_ISDIR(docroot_info.st_mode)) {
        fprintf(stderr, "Document root must be a directory: %s\n", resolved_docroot);
        exit(EXIT_FAILURE);
    }

    size_t docroot_len = strlen(resolved_docroot);
    config->docroot = malloc(docroot_len + 1);
    if (config->docroot == NULL) {
        perror("Failed to allocate memory for document root");
        exit(EXIT_FAILURE);
    }
    memcpy(config->docroot, resolved_docroot, docroot_len + 1);
}

const char* get_mime_type(const char *filename) {
    const char *ext = strrchr(filename, '.');
    if (!ext) return "application/octet-stream";    
    ext++;
    
    if (strcmp(ext, "html") == 0 || strcmp(ext, "htm") == 0) return "text/html";
    if (strcmp(ext, "css") == 0) return "text/css";
    if (strcmp(ext, "js") == 0) return "application/javascript";
    if (strcmp(ext, "jpg") == 0 || strcmp(ext, "jpeg") == 0) return "image/jpeg";
    if (strcmp(ext, "png") == 0) return "image/png";
    if (strcmp(ext, "gif") == 0) return "image/gif";
    if (strcmp(ext, "txt") == 0) return "text/plain";
    // ...

    return "application/octet-stream";
}
