#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT_NUM 1004
#define NUMROOMS 10

void error(const char *msg)
{
	perror(msg);
	exit(1);
}

typedef struct _USR {
	int clisockfd;		// socket file descriptor
	struct _USR* next;	// for linked list queue
	char username[32];
	int r, g, b;
	int roomNum;
	int fileFlag;
} USR;

typedef struct _room {
	USR* head;
	USR* tail;
	int numUsers;
} room;

struct room {
    USR* head;
    USR* tail;
    int numUsers;
};

USR *temp = NULL;
USR *victimNode = NULL;

struct room rooms[NUMROOMS];

void print(int roomNum) {
	if(roomNum >= 0 && roomNum < NUMROOMS && rooms[roomNum].numUsers > 0) {
		if(rooms[roomNum].head == NULL) {
			printf("No clients connected in Room %d.\n", roomNum + 1);
			return;
		}
		temp = rooms[roomNum].head;
		printf("%d connected clients in Room %d:\n", rooms[roomNum].numUsers - 10, roomNum + 1);
		while(temp != NULL) {
			struct sockaddr_in cliaddr;
			socklen_t clen = sizeof(cliaddr);
			if(getpeername(temp->clisockfd / 100, (struct sockaddr*)&cliaddr, &clen) < 0) error ("ERROR Unknown sended!");
			//printf("[%s] %s\n", inet_ntoa(cliaddr.sin_addr), temp->username);
			printf("[%s]\n", inet_ntoa(cliaddr.sin_addr));
			temp = temp->next;
		}
	}
	return;
}

void delete_cli(int victim) {
	int roomIndex = victim % NUMROOMS;

	rooms[roomIndex].numUsers--;

	if(rooms[roomIndex].head->clisockfd == victim) {
		temp = rooms[roomIndex].head;
		rooms[roomIndex].head = rooms[roomIndex].head->next;
		free(temp);
		return;
	}
	
	temp = rooms[roomIndex].head;
	
	while(temp->next != NULL) {
		if(temp->next->clisockfd == victim) {
			victimNode = temp->next;
			if(temp->next->next == NULL) {
				rooms[roomIndex].tail = temp;
			}
			temp->next = temp->next->next;
			free(victimNode);
			return;
		}
		temp = temp->next;
	} return;
}

void add_tail(int newclisockfd, int roomNum)
{
	if(roomNum == -2) {
		int nsen = send(newclisockfd, "No new rooms available", strlen("No new rooms available"), 0);
		if (nsen != strlen("No new rooms available")) error("ERROR send() failed");
		return;
	}
	else if(roomNum == -1) {
		int nsen = send(newclisockfd, "Room does not exist", strlen("Room does not exist"), 0);
		if (nsen != strlen("Room does not exist")) error("ERROR send() failed");
		return;
	}
	else if(rooms[roomNum].numUsers == 15) {
		int nsen = send(newclisockfd, "Room is full, sorry :/", strlen("Room is full, sorry :/"), 0);
		if (nsen != strlen("Room is full, sorry :/")) error("ERROR send() failed");
		return;
	}

	if (rooms[roomNum].head == NULL) {
		rooms[roomNum].head = (USR*) malloc(sizeof(USR));
		rooms[roomNum].head->clisockfd = newclisockfd * 100 + roomNum;
		rooms[roomNum].head->next = NULL;
		memset(rooms[roomNum].head->username, 0, sizeof(rooms[roomNum].head->username));
		rooms[roomNum].head->r = rand() % 25 * 10 + 6;
		rooms[roomNum].head->g = rand() % 25 * 10 + 6;
		rooms[roomNum].head->b = rand() % 25 * 10 + 6;
		rooms[roomNum].head->fileFlag = 0;
		rooms[roomNum].tail = rooms[roomNum].head;
	} else {
		rooms[roomNum].tail->next = (USR*) malloc(sizeof(USR));
		rooms[roomNum].tail->next->clisockfd = newclisockfd * 100 + roomNum;
		rooms[roomNum].tail->next->next = NULL;
		memset(rooms[roomNum].tail->next->username, 0, sizeof(rooms[roomNum].tail->next->username));
		rooms[roomNum].tail->next->r = rand() % 25 * 10 + 6;
		rooms[roomNum].tail->next->g = rand() % 25 * 10 + 6;
		rooms[roomNum].tail->next->b = rand() % 25 * 10 + 6;
		rooms[roomNum].tail->next->fileFlag = 0;
		rooms[roomNum].tail = rooms[roomNum].tail->next;
	}
	char message[64];
	memset(message, 0, sizeof(message));
	if(rooms[roomNum].numUsers == 0) {
		rooms[roomNum].numUsers += 10;
		sprintf(message, "Connected to room %d\n", roomNum + 1);
	}
	else {
		sprintf(message, "Connected to room %d\n", roomNum + 1);
	}
	int nsen = send(rooms[roomNum].tail->clisockfd / 100, message, strlen(message), 0);
	if (nsen != strlen(message)) error("ERROR send() failed");
	rooms[roomNum].numUsers++;
	
	print(roomNum);
}

void broadcast(int fromfd, char* message)
{
	// figure out sender address
	struct sockaddr_in cliaddr;
	socklen_t clen = sizeof(cliaddr);
	if (getpeername(fromfd / 100, (struct sockaddr*)&cliaddr, &clen) < 0) error("ERROR Unknown sender!");
	
	USR* sender = rooms[fromfd % NUMROOMS].head;
	while(sender != NULL) {
		if(sender->clisockfd == fromfd) {
			char buffer[512];
			memset(buffer, 0, sizeof(buffer));
			if(strlen(sender->username) == 0) {
				if(strlen(message) == 1) {
					int nsen = send(fromfd / 100, "Username cannot be blank, enter new username: ", strlen("Username cannot be blank, enter new username: "), 0);
					if (nsen != strlen("Username cannot be blank, enter new username: ")) error("ERROR send() failed");
					return;
				}
				int size = strlen(message);
				message[size - 1] = '\0';
				temp = rooms[fromfd % NUMROOMS].head;
				while(temp != NULL) {
					if(strcmp(temp->username, message) == 0) {
						int nsen = send(fromfd / 100, "Username already taken, enter new username: ", strlen("Username already taken, enter new username: "), 0);
						if (nsen != strlen("Username already taken, enter new username: ")) error("ERROR send() failed");
						return;
					}
					temp = temp->next;
				}
				
				int nsen = send(fromfd / 100, " ", 2, 0);
				if (nsen != 2) error("ERROR send() failed");
				strcpy(sender->username, message);
				sprintf(buffer, "\x1b[0m%s(%s) has joined the chat!\n", sender->username, inet_ntoa(cliaddr.sin_addr));
				int nmsg = strlen(buffer) - 1;
			}
			else if(sender->fileFlag == 1) {
				if(strcmp(message, "Y\n") == 0 || strcmp(message, "N\n") == 0) {
					sender->fileFlag = 0;
					
				}
				return;
			}
			else if(strlen(message) == 1) {
				sprintf(buffer, "\x1b[0m%s(%s) has left the room!\n", sender->username, inet_ntoa(cliaddr.sin_addr));
			}
				sprintf(buffer, "User does not exist\n");
				int nmsg = strlen(buffer) - 1;
				int nsen = send(fromfd / 100, buffer, nmsg, 0);
				if (nsen != nmsg) error("ERROR send() failed");
				return;
			}
			else {
				sprintf(buffer, "\x1b[38;2;%d;%d;%dm[%s(%s)]: %s\x1b[0m ", sender->r, sender->g, sender->b, sender->username, inet_ntoa(cliaddr.sin_addr), message);
			}
			// traverse through all connected clients
			USR* cur = rooms[fromfd % NUMROOMS].head;
			while (cur != NULL) {
				// check if cur is not the one who sent the message
				if (cur->clisockfd != fromfd && strlen(cur->username) != 0 && cur->fileFlag == 0) {
					// prepare message
					int nmsg = strlen(buffer) - 1;

					// send!
					int nsen = send(cur->clisockfd / 100, buffer, nmsg, 0);
					if (nsen != nmsg) error("ERROR send() failed");
				}

				cur = cur->next;
			}
			break;
		}
		sender = sender->next;
	}
}

typedef struct _ThreadArgs {
	int clisockfd;
	int roomNum;
} ThreadArgs;

void* thread_main(void* args)
{
	// make sure thread resources are deallocated upon return
	pthread_detach(pthread_self());

	if(((ThreadArgs*) args)->roomNum >= 0 && ((ThreadArgs*) args)->roomNum < NUMROOMS && rooms[((ThreadArgs*) args)->roomNum].numUsers > 0) {
		// get socket descriptor from argument
		int clisockfd = ((ThreadArgs*) args)->clisockfd * 100 + ((ThreadArgs*) args)->roomNum;
		free(args);

		//-------------------------------
		// Now, we receive/send messages
		char buffer[256];
		int nsen, nrcv;
		struct sockaddr_in cliaddr;
		socklen_t clen = sizeof(cliaddr);

		nrcv = recv(clisockfd / 100, buffer, sizeof(buffer), 0);
		if (nrcv < 0) error("ERROR recv() failed");

		while (nrcv > 0) {
			// we send the message to everyone except the sender
			broadcast(clisockfd, buffer);
			memset(buffer, 0, sizeof(buffer));

			nrcv = recv(clisockfd / 100, buffer, 255, 0);
			if(nrcv == 0) {
				if(getpeername(clisockfd / 100, (struct sockaddr*)&cliaddr, &clen) < 0) error("ERROR Unknown sender!");
				printf("Disconnected: [%s]\n", inet_ntoa(cliaddr.sin_addr));
				delete_cli(clisockfd);
				print(clisockfd % 100);
			}
			if (nrcv < 0) error("ERROR recv() faild");
		}

		close(clisockfd);
		//-------------------------------
	}
	return NULL;
}

int findNew() {
	for(int i = 0; i < NUMROOMS; ++i) {
		if(rooms[i].numUsers == 0)  {
			return i + 1;
		}
	}
	return -1;
}

void getRoom(int socket) {
     	char roomsString[1000];
    	strcpy(roomsString, "Server says following options are available:\n");
	for (int i = 0; i < NUMROOMS; ++i) {
		char str1[1000];
		if (rooms[i].numUsers == 0) {}
		else if (rooms[i].numUsers == 11) {
		     	sprintf(str1, "Room %d: %d  person\n", (i+1), rooms[i].numUsers - 10);
		     	strcat(roomsString, str1);
		}
		else {
		     	sprintf(str1, "Room %d: %d people\n", (i+1), rooms[i].numUsers - 10);
		     	strcat(roomsString, str1);
		}
     	}
     	send(socket, roomsString, sizeof(roomsString), 0);
}


int main(int argc, char *argv[])
{
	for(int i = 0; i < NUMROOMS; ++i) {
		rooms[i].head = NULL;
		rooms[i].tail = NULL;
		rooms[i].numUsers = 0;
	}

	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) error("ERROR opening socket");

	struct sockaddr_in serv_addr;
	socklen_t slen = sizeof(serv_addr);
	memset((char*) &serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;	
	//serv_addr.sin_addr.s_addr = inet_addr("192.168.1.171");	
	serv_addr.sin_port = htons(PORT_NUM);

	int status = bind(sockfd, 
			(struct sockaddr*) &serv_addr, slen);
	if (status < 0) error("ERROR on binding");

	listen(sockfd, 5); // maximum number of connections = 5

	while(1) {
		struct sockaddr_in cli_addr;
		socklen_t clen = sizeof(cli_addr);
		int newsockfd = accept(sockfd, 
			(struct sockaddr *) &cli_addr, &clen);
		if (newsockfd < 0) error("ERROR on accept");
		
		char buffer[32];
		memset(buffer, 0, sizeof(buffer));
		int nrcv = recv(newsockfd, buffer, sizeof(buffer), 0);
		if (nrcv < 0) error("ERROR recv() failed");
		int roomNum;
		//printf("%s\n", buffer);
		if(strcmp(buffer, "new") == 0 || (findNew() == 1)) roomNum = findNew();
		else if(atoi(buffer) == -1) {
			getRoom(newsockfd);
				
			memset(buffer, 0, sizeof(buffer));
			nrcv = recv(newsockfd, buffer, sizeof(buffer), 0);
			if (nrcv < 0) error("ERROR recv() failed");
			if(strcmp(buffer, "new") == 0) roomNum = findNew();
			else roomNum = atoi(buffer);
		}
		else {
			roomNum = atoi(buffer);
			if(roomNum > NUMROOMS || roomNum <= 0 || rooms[roomNum - 1].numUsers == 0) {
				roomNum = 0;
			}
		}
		
		add_tail(newsockfd, roomNum - 1); // add this new client to the client list
		// prepare ThreadArgs structure to pass client socket
		ThreadArgs* args = (ThreadArgs*) malloc(sizeof(ThreadArgs));
		if (args == NULL) error("ERROR creating thread argument");
		
		args->clisockfd = newsockfd;
		args->roomNum = roomNum - 1;

		pthread_t tid;
		if (pthread_create(&tid, NULL, thread_main, (void*) args) != 0) error("ERROR creating a new thread");
	}

	return 0; 
}

