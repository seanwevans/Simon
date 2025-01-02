// logging.c

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stddef.h>


void log_message(const char *filename, const char *message) {
    FILE *log_file = fopen(filename, "a");
    if (log_file == NULL) {
        perror("Failed to open log file");
        return;
    }

    time_t now;
    time(&now);
    char *time_str = ctime(&now);
    time_str[strlen(time_str) - 1] = '\0';

    fprintf(log_file, "[%s] %s\r\n", time_str, message);
    fclose(log_file);
}

void log_error(const char *message) {
    log_message("errors.log", message);
}

void log_connection(const char *message) {
    log_message("connect.log", message);
}
