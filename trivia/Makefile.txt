# Compiler and compiler flags
CC=gcc
CFLAGS=-Wall -g

# Default target executed when no arguments are given to make.
default: server client

# Build server executable.
server: server.o
	$(CC) $(CFLAGS) -o server server.o

# Build client executable.
client: client.o
	$(CC) $(CFLAGS) -o client client.o

# Compile server.c to server.o
server.o: server.c
	$(CC) $(CFLAGS) -c server.c

# Compile client.c to client.o
client.o: client.c
	$(CC) $(CFLAGS) -c client.c

# Clean up
clean:
	rm -f server client server.o client.o

# Declare the "phony" targets
.PHONY: default clean