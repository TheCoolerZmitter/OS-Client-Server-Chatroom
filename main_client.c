#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h> 
#include <pthread.h>
#include <ifaddrs.h>
#include <fcntl.h>

#define PORT_NUM 1004

void error(const char *msg)
{
	perror(msg);
	exit(0);
}

typedef struct _ThreadArgs {
	int clisockfd;
} ThreadArgs;

void* thread_main_recv(void* args)
{
	pthread_detach(pthread_self());

	int sockfd = ((ThreadArgs*) args)->clisockfd;
	free(args);

	// keep receiving and displaying message from server
	char buffer[512];
	int n;
	n = 1;//recv(sockfd, buffer, 512, 0);
	//printf("%s\n", buffer);
	while (n > 0) {
		memset(buffer, 0, 512);
		n = recv(sockfd, buffer, 512, 0);
		if (n < 0) error("ERROR recv() failed");

		if(strlen(buffer) > 3 && buffer[0] == 'S' && buffer[1] == 'E' && buffer[2] == 'N' && buffer[3] == 'D') {
			memset(buffer, 0, 512);
			n = recv(sockfd, buffer, 512, 0);
			if (n < 0) error("ERROR recv() failed");
			printf("%s\n", buffer);
			
			
		}
		else {
			printf("%s\n", buffer);
		}
	}

	return NULL;
}

char* roomNum;

void* thread_main_send(void* args)
{
	struct ifaddrs * ifAddr = NULL;
	getifaddrs(&ifAddr);

	pthread_detach(pthread_self());

	int sockfd = ((ThreadArgs*) args)->clisockfd;
	free(args);

	// keep sending messages to the server
	char buffer[256];
	char username[16];
	int n;

	while (1) {
		// You will need a bit of control on your terminal
		// console or GUI to have a nice input window.
		memset(buffer, 0, 256);
		fgets(buffer, 255, stdin);

		if (strlen(buffer) == 1) {
			n = send(sockfd, buffer, strlen(buffer), 0);
			if (n < 0) error("ERROR writing to socket");
			break; // we stop transmission when user type empty string
		}
		n = send(sockfd, buffer, strlen(buffer), 0);
		if (n < 0) error("ERROR writing to socket");
		printf("\n");
	}
	free(ifAddr);
	
	return NULL;
}

int main(int argc, char *argv[])
{
	if (argc < 2) error("Please specify hostname");

	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) error("ERROR opening socket");

	struct sockaddr_in serv_addr;
	socklen_t slen = sizeof(serv_addr);
	memset((char*) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
	serv_addr.sin_port = htons(PORT_NUM);
	
	if(argc < 3) roomNum = "-1";
	else roomNum = argv[2];

	int status = connect(sockfd, 
			(struct sockaddr *) &serv_addr, slen);
	if (status < 0) error("ERROR connecting");
	
	char buffer[1000];
	char username[16];
	int n;
	
	n = send(sockfd, roomNum, strlen(roomNum), 0);
	if (n < 0) error("ERROR writing to socket");
	
	memset(buffer, 0, 1000);
	n = recv(sockfd, buffer, 1000, 0);
	printf("%s\n", buffer);
	if(strcmp(buffer, "Room is full, sorry :/") == 0 || strcmp(buffer, "Room does not exist") == 0 || strcmp(buffer, "No new rooms available") == 0) {
		close(sockfd);
		return 0;
	}
	else if(buffer[0] == 'S') {
		printf("Choose the room number or type \"new\" to create a new room: ");
		memset(buffer, 0, 1000);
		fgets(buffer, 1000, stdin);
		roomNum = buffer;
		
		n = send(sockfd, roomNum, strlen(roomNum) - 1, 0);
		if (n < 0) error("ERROR writing to socket");
	}
	
	int flag;
	
	printf("Enter your username: ");
	
	do {
		flag = 0;
		memset(username, 0, 16);
		fgets(username, 16, stdin);
		
		n = send(sockfd, username, strlen(username), 0);
		if (n < 0) error("ERROR writing to socket");
		
		memset(buffer, 0, 1000);
		n = recv(sockfd, buffer, 1000, 0);
		
		if(strcmp(buffer, "Username cannot be blank, enter new username: ") == 0 || strcmp(buffer, "Username already taken, enter new username: ") == 0) {
			printf("%s", buffer);
			flag = 1;
		}
	} while(flag == 1);

	pthread_t tid1;
	pthread_t tid2;

	ThreadArgs* args;
	
	args = (ThreadArgs*) malloc(sizeof(ThreadArgs));
	args->clisockfd = sockfd;
	pthread_create(&tid1, NULL, thread_main_send, (void*) args);

	args = (ThreadArgs*) malloc(sizeof(ThreadArgs));
	args->clisockfd = sockfd;
	pthread_create(&tid2, NULL, thread_main_recv, (void*) args);

	// parent will wait for sender to finish (= user stop sending message and disconnect from server)
	pthread_join(tid1, NULL);

	close(sockfd);

	return 0;
}

