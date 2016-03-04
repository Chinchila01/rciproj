CC=gcc
CFLAGS=-Wall -pedantic

snp: snp.c
	$(CC) $(CFLAGS) snp.c -o snp 

schat: schat.c
	$(CC) $(CFLAGS) schat.c -o schat 

test: test.c
	$(CC) $(CFLAGS) test.c -o test 

all: snp schat test

