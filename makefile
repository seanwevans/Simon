# Makefile for Multi-threaded Web Server in C

CC = gcc
CFLAGS = -Wall -pthread
TARGET = webserver

SRCS = main.c \
       server.c \
       queue.c \
       request.c \
       logging.c \
       utils.c \
       parseutf.c

OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

run:
	./$(TARGET) index.html 8080

.PHONY: all clean run

