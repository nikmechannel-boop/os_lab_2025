CC=gcc
CFLAGS=-I. -Wall -pthread

all: server client

server: server.o calc_lib.o
	$(CC) -o server server.o calc_lib.o $(CFLAGS)

client: client.o calc_lib.o
	$(CC) -o client client.o calc_lib.o $(CFLAGS)

server.o: server.c calc_lib.h
	$(CC) -c -o server.o server.c $(CFLAGS)

client.o: client.c calc_lib.h
	$(CC) -c -o client.o client.c $(CFLAGS)

calc_lib.o: calc_lib.c calc_lib.h
	$(CC) -c -o calc_lib.o calc_lib.c $(CFLAGS)

clean:
	rm -f *.o server client

.PHONY: all clean