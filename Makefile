CC = gcc
CFLAGS = -Wall -g 

all: human_client bot_client server

human_client: human/human_client.c
	$(CC) $(CFLAGS) -o human/human_client human/human_client.c -lncurses

bot_client: bots_prizes/bot_client.c
	$(CC) $(CFLAGS) -o bots_prizes/bot_client bots_prizes/bot_client.c -lncurses

server: server_game/server.c
	$(CC) $(CFLAGS) -o server_game/server server_game/server.c -lncurses

clean:
	rm human/human_client bots_prizes/bot_client server_game/server
