CC = gcc
CFLAGS = -Wall -g 

all: human_client bot_client prize_client server

human_client: human/human_client.c
	$(CC) $(CFLAGS) -o human_client human/human_client.c -lncurses -lpthread

bot_client: bots_prizes/bot_client.c
	$(CC) $(CFLAGS) -o bot_client bots_prizes/bot_client.c -lncurses -lpthread

server: server_game/server.c
	$(CC) $(CFLAGS) -o server server_game/server.c -lncurses -lpthread

prize_client: prizes/prize_client.c
	$(CC) $(CFLAGS) -o prize_client prizes/prize_client.c -lncurses -lpthread


clean:
	rm human_client bot_client prize_client server
