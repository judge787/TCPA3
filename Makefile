# Makefile for the CIS*3210 Assignment 3
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2
TARGETS = server client

all: $(TARGETS)

server: TCPserver.c
	$(CC) $(CFLAGS) -o server TCPserver.c

client: TCPclient.c
	$(CC) $(CFLAGS) -o client TCPclient.c

clean:
	rm -f $(TARGETS) *.o
