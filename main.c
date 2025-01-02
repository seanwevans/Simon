// main.c

#include "server.h"

int main(int argc, char *argv[]) {
    Server config;    
    parse_arguments(argc, argv, &config);

    start_server(&config);

    return 0;
}
