CC=gcc
CFLAGS=-O2 -Wall

all: main.o
	$(CC) $(CFLAGS) main.o -o uex

main.o: main.c
	$(CC) $(CFLAGS) -c main.c
