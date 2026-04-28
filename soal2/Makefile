CC = gcc
CFLAGS = -Wall -pthread

all: server client

server: orion.c arena.h
	$(CC) $(CFLAGS) orion.c -o orion

client: eternal.c arena.h
	$(CC) $(CFLAGS) eternal.c -o eternal

clean:
	rm -f orion eternal

clear_ipc:
	ipcs -m | grep 0x00001234 | awk '{print $$2}' | xargs -r ipcrm -m
