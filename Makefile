.PHONY: all clean
SRCS := server.c client.c circular-buffer.c

all: Server/server Client/client

Server/server: Server/server.c circular-buffer.o
	gcc -o Server/server Server/server.c circular-buffer.o

Client/client: Client/client.c circular-buffer.o
	gcc -o Client/client Client/client.c circular-buffer.o

circular-buffer: circular-buffer.c 
	gcc -o circular-buffer circular-buffer.c

clean:
	rm -f server client

