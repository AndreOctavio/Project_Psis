CC = gcc
CFLAGS = -Wall -g 

all: human_client server

human_client: human/human_client.c
	$(CC) $(CFLAGS) -o human_client human/human_client.c -lncurses -lpthread

server: server_game/server.c
	$(CC) $(CFLAGS) -o server server_game/server.c -lncurses -lpthread

clean:
	rm human_client server
