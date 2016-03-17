CC=gcc
CFLAGS=-Wall -pedantic

<<<<<<< HEAD
main: main.c
	$(CC) $(CFLAGS) main.c -o main 
=======
snp: snp.c
	$(CC) $(CFLAGS) snp.c -o snp 

schat: schat.c
	$(CC) $(CFLAGS) schat.c -o schat 

unreguser:	unreguser.c
	$(CC) $(CFLAGS) unreguser.c -o unreguser

unregister: unregister.c
	$(CC) $(CFLAGS) unregister.c -o unregister

all: snp schat unreguser unregister

>>>>>>> a45d83981e113dd7683507d41115f4b1133821e0
