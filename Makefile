CC = gcc
# CC=gcc -Wall

server: server.c
	$(CC) -g -o server server.c

client: client.c
	$(CC) -g -o client client.c
clean:
	rm -rf server client out.txt

