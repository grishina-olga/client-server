#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <winsock2.h>
#include <windows.h>
#include <unistd.h>
#include <string.h>
#include "serv_client.h"

extern client clients[MAX_CLIENTS];
int mycounter = 0;
FILE *logfile;

DWORD WINAPI Client(LPVOID newsock);

int my_send(SOCKET s, const char* buf, int size);
int unique_name(char* name);

void display_users(SOCKET sockfd);

int main(int argc, char *argv[]) {
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 0), &wsaData);

	logfile = fopen("logfile.txt", "w");

	if (argc < 2) {
		fprintf(stderr, "ERROR, no port provided\n");
		return 0;
	}

	SOCKET sockfd;
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		printf("Error opening socket: %d\n", WSAGetLastError());
		WSACleanup();
		exit(1);
	}

	sockaddr_in serv_addr;
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[1]));
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(sockfd, (sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		printf("Error on binding: %d\n", WSAGetLastError());
		WSACleanup();
		exit(2);
	}

	if (listen(sockfd, 5) < 0) {
		printf("Error on listening: %d\n", WSAGetLastError());
		WSACleanup();
		exit(3);
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
			WSACleanup();
			exit(4);
		}

		printf("\nNew client has connected: %s\n",
				inet_ntoa(cli_addr.sin_addr));

		clients[mycounter].IP = cli_addr.sin_addr.s_addr;
		mycounter++;

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

	char name[NAME_LEN + 2]; //one byte for \n, second byte for \0
	memset(name, 0, NAME_LEN + 2); // один байт для \n, второй для \0

	int inx = mycounter - 1;

	int n = recv(my_sock, name, sizeof(name), 0);
	if (n < 0) {
		printf("\nError reading from socket: %d\n", WSAGetLastError());
		WSACleanup();
		exit(5);
	}

	int result;
	int port;

	if (mycounter < MAX_CLIENTS) {
		do {
			result = unique_name(name);
			if (result == 0) {
				printf("NEW NAME IS ACCEPTED\n");
				my_send(my_sock, "accepted", strlen("accepted"));
			} else {
				printf("ERROR: name already in use\n");
				my_send(my_sock, "exists", strlen("exists"));
				recv(my_sock, name, sizeof(name), 0);
			}
		} while (result == -1);

		recv(my_sock, (char*) &port, sizeof(port), 0);
		if (n < 0) {
			printf("\nError reading from socket: %d\n", WSAGetLastError());
			WSACleanup();
			exit(5);
		}

		strcpy(clients[inx].name, name);
		clients[inx].unique_id = my_sock;
		clients[inx].port = port;

		printf("Name: %s   ID: %d   Port: %d\n\n", name, my_sock, port);

		display_users(my_sock);

		char new_user[100] = "New user has connected: ";
		strcat(new_user, name);
		strcat(new_user, "\n");
		fputs(new_user, logfile);

		SOCKET sock;
		for (int i = 0; i < mycounter; i++) {
			sock = clients[i].unique_id;
			if (sock != my_sock && clients[i].unique_id != 0) {
				my_send(sock, (char*) clients, sizeof(clients));
				my_send(sock, new_user, sizeof(new_user));
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
		strcat(log_quit, "User disconnected: ");
		strcat(log_quit, name);
		strcat(log_quit, "\n");
		fputs(log_quit, logfile);

		for (int i = 0; i < mycounter; i++) {
			if (strcmp(clients[i].name, name) == 0) {
				printf("\nuser removed");
				for (int j = i; j < mycounter; j++) {
					if (j == MAX_CLIENTS - 1) {
						memset(&clients[j], 0, sizeof(client));
					} else
						clients[j] = clients[j + 1];
				}
				if (n > 0) {
					printf("\nclient is normally leaving: %d\n", my_sock);
				} else {
					printf("\nclient is abnormally leaving: %d\n", my_sock);
				}
				mycounter--;
			}
		}

		SOCKET sock;
		for (int i = 0; i < mycounter; i++) {
			sock = clients[i].unique_id;
			if (sock != my_sock && clients[i].unique_id != 0) {
				my_send(sock, (char*) clients, sizeof(clients));
				my_send(sock, log_quit, sizeof(log_quit));
			}
		}
		break;
	}
	close(my_sock);
	return 0;
}
