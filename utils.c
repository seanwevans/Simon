// utils.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "server.h"

void parse_arguments(int argc, char *argv[], Server *config) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <filename> [port] [core_count] [num_threads]\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    config->file = argv[1];
    config->port = (argc > 2) ? atoi(argv[2]) : DEFAULT_PORT;
    config->core_count = (argc > 3) ? atoi(argv[3]) : 16;
    config->num_threads = (argc > 4) ? atoi(argv[4]) : NUM_THREADS;
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
