all: server client_display client_send

server: server.c
	gcc -o server server.c

client_display: client_display.c
	gcc -o client_display client_display.c

client_send: client_send.c
	gcc -o client_send client_send.c

clean:
	rm -f server client_display client_send