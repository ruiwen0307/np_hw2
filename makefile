all:
	gcc -pthread server.c -o server
	gcc -pthread client.c -o client
clean:
	rm client server