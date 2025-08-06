// logging.c

#include <stdio.h>
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

    struct tm tm_info;
    if (localtime_r(&now, &tm_info) == NULL) {
        perror("Failed to convert time");
        fclose(log_file);
        return;
    }

    char time_str[64];
    if (strftime(time_str, sizeof(time_str), "%a %b %e %H:%M:%S %Y", &tm_info) == 0) {
        perror("Failed to format time");
        fclose(log_file);
        return;
    }

    fprintf(log_file, "[%s] %s\r\n", time_str, message);
    fclose(log_file);
}

void log_error(const char *message) {
    log_message("errors.log", message);
}

void log_connection(const char *message) {
    log_message("connect.log", message);
}
