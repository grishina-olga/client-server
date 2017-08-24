#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <winsock2.h>
#include <windows.h>
#include <unistd.h>
#include <string.h>

#define MAX_CLIENTS 100

DWORD WINAPI Client(LPVOID newsock);

FILE *logfile;
int mycounter = 0;

typedef struct {
	char name[10];
	int unique_id;
	int port;
} client;

client clients[MAX_CLIENTS] = { { "", 0, 0 } };

void display_users(SOCKET sockfd) {
	int counter = 0;
	char s[100];
	s[0] = 0;

	while ((counter < MAX_CLIENTS) && clients[counter].unique_id != 0) {
		if (counter == 0) {
			strcpy(s, clients[counter].name);
		} else {
			strcat(s, clients[counter].name);
		}
		strcat(s, "\n");
		counter++;
	}
	printf("Printing active users:\n");
	printf("%s\n", s);

	int n = send(sockfd, (char*) clients, sizeof(clients), 0);
	if (n < 0) {
		printf("error sending");
	}
}

int unique_name(char* name) {
	int counter = 0;
	while (counter < MAX_CLIENTS && clients[counter].unique_id != 0) {
		if (strcmp(clients[counter].name, name) == 0) {
			printf("User name already exists\n");
			return -1;
		}
		counter++;
	}
	return 0;
}

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
		perror("Error opening socket");
		exit(1);
		WSACleanup();
	}

	sockaddr_in serv_addr;
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[1]));
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(sockfd, (sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		perror("Error on binding");
		exit(2);
		WSACleanup();
	}

	if (listen(sockfd, 5) < 0) {
		perror("Error on listening");
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
			perror("Error on accept\n");
			exit(4);
			WSACleanup();
		}

		printf("\nNew client has connected: %s\n",
				inet_ntoa(cli_addr.sin_addr));

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
	//char port_user[100];
	memset(name, 0, 10);

	int n = recv(my_sock, name, sizeof(name), 0);
	if (n < 0) {
		printf("\nError reading from socket");
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
				send(my_sock, "accepted", sizeof("accepted"), 0);
			} else {
				printf("ERROR: name already in use\n");
				send(my_sock, "exists", sizeof("exists"), 0);
				recv(my_sock, name, sizeof(name), 0);
			}
		} while (result == -1);

		recv(my_sock, (char*) &port, sizeof(port), 0);

		client temp;
		strcpy(temp.name, name);
		temp.unique_id = my_sock;
		temp.port = port;
		clients[mycounter] = temp;
		mycounter++;

		printf("Name: %s   ID: %d   Port: %d\n\n", name, my_sock, port);

		display_users(my_sock);

		char new_user[100] = "New user has connected: ";
		strcat(new_user, name);
		strcat(new_user, "\n");

		fputs(new_user, logfile);

		SOCKET sock;
		for (int i = 0; i <= mycounter; i++) {
			sock = clients[i].unique_id;
			if (sock != my_sock) {
				send(sock, new_user, sizeof(new_user), 0);
				send(sock, (char*) clients, sizeof(clients), 0);
			}

		}
	} else {
		printf("We can not support new client!\n");
		char err[] = "err";
		send(my_sock, err, sizeof(err), 0);
		close(my_sock);
	}

	char msg[100];

	char log_quit[100];
	memset(log_quit, 0, 100);

	while (my_sock > 0) {
		int n = recv(my_sock, msg, sizeof(msg), 0);
		if (n < 0)
			break;

		if (strcmp(msg, "quit") == 0) {
			strcat(log_quit, "User disconnected: ");
			strcat(log_quit, name);
			strcat(log_quit, "\n");
			fputs(log_quit, logfile);

			for (int i = 0; i <= mycounter; i++) {
				if (strcmp(clients[i].name, name) == 0) {
					printf("\nuser removed");
					for (int j = i; j <= mycounter; j++) {
						clients[j] = clients[j + 1];
					}
					send(my_sock, "quit", sizeof("quit"), 0);
					printf("\nclient is leaving: %d\n", my_sock);
				}
			}

			SOCKET sock;
			for (int i = 0; i <= mycounter; i++) {
				sock = clients[i].unique_id;
				if (sock != my_sock) {
					send(sock, log_quit, sizeof(log_quit), 0);
					send(sock, (char*) clients, sizeof(clients), 0);
				}
			}
			break;
		}
	}
	mycounter--;
	close(my_sock);
	//WSACleanup();
	return 0;
}
