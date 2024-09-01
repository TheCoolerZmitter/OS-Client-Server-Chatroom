all:
	gcc -o main_server main_server.c -lpthread
	gcc -o main_client main_client.c -lpthread

clean:
	rm chat_server chat_client
	rm main_server main_client

