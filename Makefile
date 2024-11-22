CC = g++
CFLAGS = -Wall -g

all: server subscriber

.PHONY: clean

server:
	$(CC) $(CFLAGS) server.cpp -o server

subscriber:
	$(CC) $(CFLAGS) subscriber.cpp -o subscriber

clean:
	rm -f server subscriber