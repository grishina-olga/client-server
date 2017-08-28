#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <winsock2.h>
#include <windows.h>
#include <unistd.h>
#include <string.h>
#include "serv_client.h"

int mycounter = 0;
FILE *logfile;

DWORD WINAPI Client(LPVOID newsock);

int my_send(SOCKET s, const char* buf, int size);
int unique_name(char* name);

void display_users(SOCKET sockfd);

extern client clients[MAX_CLIENTS];

int main(int argc, char *argv[]) {
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	logfile = fopen("logfile.txt", "w");

	if (argc < 2) {
		fprintf(stderr, "ERROR, no port provided\n");
		return 0;
	}

	SOCKET sockfd;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		printf("Error opening socket: %d\n", WSAGetLastError());
		exit(1);
		WSACleanup();
	}

	sockaddr_in serv_addr;
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[1]));
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(sockfd, (sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		printf("Error on binding: %d\n", WSAGetLastError());
		exit(2);
		WSACleanup();
	}

	if (listen(sockfd, 5) < 0) {
		printf("Error on listening");
		exit(3);
		WSACleanup();
	}

	printf("The server is now running... %d\n\n", sockfd);
	HANDLE hThread;
	while (1) {
		sockaddr_in cli_addr;
		int cli_len = sizeof(cli_addr);
		SOCKET newsockfd;
		newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &cli_len);
		if (newsockfd < 0) {
			printf("Error on accept: %d\n", WSAGetLastError());
			exit(4);
			WSACleanup();
		}

		printf("\nNew client has connected: %s\n",
				inet_ntoa(cli_addr.sin_addr));

		clients[mycounter].IP = cli_addr.sin_addr.s_addr;

		DWORD thID;
		hThread = CreateThread(NULL, 0, Client, (LPVOID) &newsockfd, 0, &thID);
	}

	close(sockfd);
	CloseHandle(hThread);
	return 0;
}


DWORD WINAPI Client(LPVOID newsock) {
	SOCKET my_sock;
	my_sock = ((SOCKET *) newsock)[0];

	const int namesize = 10;
	char name[namesize];
	memset(name, 0, 10);

	int n = recv(my_sock, name, sizeof(name), 0);
	if (n < 0) {
		printf("\nError reading from socket: %d\n", WSAGetLastError());
		exit(5);
		WSACleanup();
	}

	int result;
	int port;

	if (mycounter < MAX_CLIENTS) {
		do {
			result = unique_name(name);

			if (result == 0) {
				printf("NEW NAME IS ACCEPTED\n");
				//send(my_sock, "accepted", strlen("accepted"), 0);
				my_send(my_sock, "accepted", strlen("accepted"));
			} else {
				printf("ERROR: name already in use\n");
				//send(my_sock, "exists", strlen("exists"), 0);
				my_send(my_sock, "exists", strlen("exists"));
				recv(my_sock, name, sizeof(name), 0);
			}
		} while (result == -1);

		//ÏÎÐÒ
		recv(my_sock, (char*) &port, sizeof(port), 0);
		if (n < 0) {
			printf("\nError reading from socket: %d\n", WSAGetLastError());
			exit(5);
			WSACleanup();
		}

		strcpy(clients[mycounter].name, name);
		clients[mycounter].unique_id = my_sock;
		clients[mycounter].port = port;

		mycounter++;

		printf("Name: %s   ID: %d   Port: %d\n\n", name, my_sock, port);

		display_users(my_sock);

		char new_user[100] = "New user has connected: ";
		strcat(new_user, name);
		strcat(new_user, "\n");

		fputs(new_user, logfile);

		SOCKET sock;
		for (int i = 0; i < mycounter; i++) {
			sock = clients[i].unique_id;
			if (sock != my_sock) {
				my_send(sock, new_user, sizeof(new_user));
				my_send(sock, (char*) clients, sizeof(clients));
			}
		}
	} else {
		printf("We can not support new client!\n");
		char err[] = "err";
		my_send(my_sock, err, sizeof(err));
		close(my_sock);
	}

	char msg[100];
	char log_quit[100];
	memset(log_quit, 0, 100);

	while (my_sock > 0) {
		int n = recv(my_sock, msg, sizeof(msg), 0);
		if ( (n > 0) || (n < 0)) {
		//if (strcmp(msg, "quit") == 0) {
			strcat(log_quit, "User disconnected: ");
			strcat(log_quit, name);
			strcat(log_quit, "\n");
			fputs(log_quit, logfile);

			for (int i = 0; i < mycounter; i++) {
				if (strcmp(clients[i].name, name) == 0) {
					printf("\nuser removed");
					for (int j = i; j < mycounter; j++) {
						clients[j] = clients[j + 1];
					}
					if(n > 0) {
					//	my_send(my_sock, "quit", strlen("quit"));
						printf("\nclient is normally leaving: %d\n", my_sock);
					}
					else
						printf("\nclient is abnormally leaving: %d\n", my_sock);
					mycounter--;
				}

				//printf("\n'%s'\n", clients[i].name);

			}
			SOCKET sock;
			for (int i = 0; i < mycounter; i++) {
				sock = clients[i].unique_id;
				if (sock != my_sock) {
					my_send(sock, log_quit, sizeof(log_quit));
					my_send(sock, (char*) clients, sizeof(clients));
				}
			}
		}
		break;
	}
	//mycounter--;
	close(my_sock);
	return 0;
}
