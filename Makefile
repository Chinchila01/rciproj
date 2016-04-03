CC=gcc
CFLAGS=-Wall -pedantic

snp: snp.c
	$(CC) $(CFLAGS) -o snp snp.c helper.c communications.c

schat: schat.c
	$(CC) $(CFLAGS) -o schat schat.c helper.c communications.c

unreguser: unreguser.c
	$(CC) $(CFLAGS) unreguser.c -o unreguser

unregister: unregister.c
	$(CC) $(CFLAGS) unregister.c -o unregister

tcp_connect: tcp_connect.c
	$(CC) $(CFLAGS) tcp_connect.c -o tcp_connect

all: snp schat unreguser unregister tcp_connect
