# Makefile
FLAGS = -std=gnu99 -Wall -lpthread -O1 -D_XOPEN_SOURCE>=700
chat: client.c server.c
	gcc ${FLAGS} -o client client.c
	gcc ${FLAGS} -o server server.c
clean:
	rm -f client
	rm -f server



