CC=gcc
CFLAGS=-Wall -pedantic

main: main.c
	$(CC) $(CFLAGS) main.c -o main 
