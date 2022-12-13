all: chase-client


chase-client: chase-client.c chase.h
	gcc -Wall -pedantic chase-client.c -g -o chase-client -lncurses

clean:
	rm chase-client
