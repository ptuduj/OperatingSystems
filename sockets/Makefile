CC=gcc
LIBS=-pthread

all: clean server client

server:
	$(CC) -o server server.c $(LIBS)

client:
	$(CC) -o client client.c $(LIBS)

clean:
	rm -f server client
