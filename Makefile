CC=gcc
CFLAGS=-Wall -pedantic

snp: snp.c
	$(CC) $(CFLAGS) snp.c -o snp 

schat: schat.c
	$(CC) $(CFLAGS) schat.c -o schat 

unreguser:	unreguser.c
	$(CC) $(CFLAGS) unreguser.c -o unreguser

unregister: unregister.c
	$(CC) $(CFLAGS) unregister.c -o unregister

all: snp schat unreguser unregister

