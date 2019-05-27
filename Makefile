.PHONY: all clean
SRCS := server.c client.c

all: Server/server Client/client

Server/server: Server/server.c
	gcc -o Server/server Server/server.c

Client/client: Client/client.c
	gcc -o Client/client Client/client.c

clean:
	rm -f server client

