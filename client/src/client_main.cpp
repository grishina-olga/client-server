#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <winsock2.h>
#include <windows.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include "cli_client.h"

extern bool isExit;
extern client clients[MAX_CLIENTS];

DWORD WINAPI Recv_serv(LPVOID newsock);
DWORD WINAPI Recv_cli(LPVOID newsock);
DWORD WINAPI Send(LPVOID newsock);

int my_send(SOCKET s, const char* buf, int size);
int find_user(const char* name);

void trim(char *s);
void quit(int sock);

void ctrl_c(int a) {
	isExit = true;
}

void name_correct(char *s, int size);

char name[NAME_LEN + 2]; //one byte for \n, second byte for \0
char help[] = "Type 'type' 	  to send a message\n"
		"Type 'quit' 	  to quit\n"
		"Type 'user list'  to get a user list\n"
		"Type 'help' 	  to get a help\n\n";

void get_name() {
	do {
		printf("\nEnter your name: ");
		fgets(name, NAME_LEN + 1, stdin);
		name[NAME_LEN] = '\n';
		name[NAME_LEN + 1] = '\0';
		trim(name);
	} while (strcmp(name, "\n") == 0);
}

int main(int argc, char *argv[]) {
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 0), &wsaData);

	if (argc < 3) {
		printf("usage %s hostname port\n", argv[0]);
		return 0;
	}

	struct hostent *server;
	server = gethostbyname(argv[1]);
	if (server == NULL) {
		printf("ERROR, no such host\n");
		WSACleanup();
		exit(1);
	}

	sockaddr_in serv_addr;
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[2]));
	memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);

	SOCKET sock_serv;
	sock_serv = socket(AF_INET, SOCK_STREAM, 0);
	if (sock_serv < 0) {
		printf("Error opening socket: %d\n", WSAGetLastError());
		WSACleanup();
		exit(1);
	}

	if (connect(sock_serv, (struct sockaddr *) &serv_addr, sizeof(serv_addr))
			< 0) {
		printf("\nError connecting: %d", WSAGetLastError());
		WSACleanup();
		exit(2);
	}

	printf("Connected... \n\n");

	signal(SIGINT, ctrl_c);
	get_name();
	name_correct(name, strlen(name) - 1);

	my_send(sock_serv, name, sizeof(name));

	char msg[100];
	do {
		memset(msg, 0, 100);
		int n = recv(sock_serv, msg, sizeof(msg), 0);
		if (n < 0) {
			printf("\nError reading from socket");
			printf("\nServer disconnected!");
			WSACleanup();
			exit(3);
		}
		if (strcmp(msg, "err") == 0) {
			printf("The server is full, please try later!\n");
			close(sock_serv);
			WSACleanup();
			exit(4);
		}

		if (strcmp(msg, "accepted") == 0) {
			printf("\nThe name was successfully submitted!\n");
		} else {
			printf("\nThis name is already in use. Please, choose another\n");
			get_name();
			name_correct(name, strlen(name) - 1);

			my_send(sock_serv, name, sizeof(name));
		}
	} while (strcmp(msg, "exists") == 0);

	SOCKET sock_cli;
	sock_cli = socket(AF_INET, SOCK_STREAM, 0);
	if (sock_cli < 0) {
		printf("Error opening socket: %d\n", WSAGetLastError());
		WSACleanup();
		exit(1);
	}

	int addr = 50000;
	int max_addr = 55000;
	sockaddr_in cli_addr;
	memset(&cli_addr, 0, sizeof(cli_addr));
	cli_addr.sin_family = AF_INET;
	cli_addr.sin_port = htons(addr);
	cli_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	while (bind(sock_cli, (sockaddr *) &cli_addr, sizeof(cli_addr))) {
		if (addr >= max_addr) {
			printf("Error on binding: %d", WSAGetLastError());
			WSACleanup();
			exit(2);
		}
		cli_addr.sin_port = htons(++addr);
	}

	if (listen(sock_cli, 5) < 0) {
		printf("Error on listening: %d", WSAGetLastError());
		WSACleanup();
		exit(3);
	}

	my_send(sock_serv, (char*) &addr, sizeof(addr));

	int n = recv(sock_serv, (char*) clients, sizeof(clients), 0);
	if (n < 0) {
		printf("\nError reading from socket");
		WSACleanup();
		exit(3);
	}

	printf("\nThe active users are: ");
	for (int i = 0; i < MAX_CLIENTS; i++) {
		if (strcmp("\0", clients[i].name) == 0) {
			break;
		}
		printf("\n%s", clients[i].name);
	}

	printf("\n\nYou are ready. \n%s", help);

	DWORD thID_r_s;
	DWORD thID_r_c;
	DWORD thID_send;

	HANDLE hThread_recv_s;
	HANDLE hThread_recv_c;
	HANDLE hThread_send;

	hThread_recv_s = CreateThread(NULL, 0, Recv_serv, (LPVOID) &sock_serv, 0,
			&thID_r_s);
	hThread_recv_c = CreateThread(NULL, 0, Recv_cli, (LPVOID) &sock_cli, 0,
			&thID_r_c);
	hThread_send = CreateThread(NULL, 0, Send, (LPVOID) &sock_serv, 0,
			&thID_send);

	while (isExit == false)
		Sleep(1);

	close(sock_serv);
	close(sock_cli);

	CloseHandle(hThread_recv_s);
	CloseHandle(hThread_recv_c);
	CloseHandle(hThread_send);

	quit(sock_serv);
	return 0;
}

bool block_rec = false;
const int size_buf = 1000;
char global_recv_buf[size_buf];

DWORD WINAPI Recv_serv(LPVOID newsock) {
	SOCKET my_sock;
	my_sock = ((SOCKET *) newsock)[0];

	while (1) {
		char msg[100];
		memset(msg, 0, 100);

		int n = recv(my_sock, (char*) clients, sizeof(clients), 0);
		if (n < 0) {
			break;
		}
		int m = recv(my_sock, msg, sizeof(msg), 0);
		if (m < 0) {
			break;
		}

		if (block_rec == true) {
			if ((strlen(global_recv_buf) + strlen(msg) + strlen("\n"))
					< size_buf) {
				strcat(global_recv_buf, msg);
				strcat(global_recv_buf, "\n");
			} else {
				printf(
						"\nThe buffer is overflow!\nYou can`t receive messages!\nPlease, send your message!\n");
			}
		} else {
			printf("%s\n", msg);
		}
	}
	printf("\nServer disconnected!\n");
	close(my_sock);
	WSACleanup();
	isExit = true;
	return 0;
}

DWORD WINAPI Recv_cli(LPVOID newsock) {
	SOCKET my_sock;
	my_sock = ((SOCKET *) newsock)[0];

	char msg[100];
	memset(msg, 0, 100);

	sockaddr_in cli_addr_1;
	int cli_len1 = sizeof(cli_addr_1);

	SOCKET newsockfd;

	while (1) {
		newsockfd = accept(my_sock, (struct sockaddr *) &cli_addr_1, &cli_len1);
		if (newsockfd < 0) {
			printf("Error on accept: %d\n", WSAGetLastError());
			WSACleanup();
			exit(4);
		}
		int n = recv(newsockfd, msg, sizeof(msg), 0);
		if (n < 0) {
			break;
		}

		if (block_rec == true) {
			if ((strlen(global_recv_buf) + strlen(msg) + strlen("\n"))
					< size_buf) {
				strcat(global_recv_buf, msg);
				strcat(global_recv_buf, "\n");
			} else {
				printf(
						"\nThe buffer is overflow!\nYou can`t receive messages!\nPlease, send your message!\n");
			}
		} else {
			printf("%s\n", msg);
		}
	}
	close(my_sock);
	WSACleanup();
	return 0;
}

DWORD WINAPI Send(LPVOID newsock) {
	SOCKET my_sock;
	my_sock = ((SOCKET *) newsock)[0];

	char msg[100];
	char mes[100];
	char receiver[10];
	char del[] = ": ";
	char cmd[100];

	while (1) {
		fgets(cmd, sizeof(cmd), stdin);

		if (strcmp(cmd, "quit\n") == 0) {
			quit(my_sock);
		}

		if (strcmp(cmd, "user list\n") == 0) {
			for (int i = 0; i < MAX_CLIENTS; i++) {
				if (strcmp("\0", clients[i].name) == 0) {
					break;
				}
				printf("\n'%s'", clients[i].name);
			}
			printf("\n");
		}

		if (strcmp(cmd, "help\n") == 0) {
			printf("\n%s", help);
		}

		if (strcmp(cmd, "type\n") == 0) {
			block_rec = true;
			printf("Enter your message: ");
			fgets(mes, sizeof(mes), stdin);

			int size = strlen(mes) - 1;
			mes[size] = '\0';

			printf("Enter receiver: ");
			fgets(receiver, 10, stdin);

			size = strlen(receiver) - 1;
			receiver[size] = '\0';

			strcpy(msg, name);
			strcat(msg, del);
			strcat(msg, mes);
			strcat(msg, "\n");

			int cnt;
			cnt = find_user(receiver);

			int ip;
			int port;

			if (cnt == -1) {
				printf("\nUser is not found!\n");
			} else {
				port = clients[cnt].port;
				ip = clients[cnt].IP;

				sockaddr_in cli_addr;
				memset(&cli_addr, 0, sizeof(cli_addr));
				cli_addr.sin_family = AF_INET;
				cli_addr.sin_port = htons(port);
				cli_addr.sin_addr.s_addr = ip; //IP äðóãîãî êëèåíòà

				SOCKET sock;
				sock = socket(AF_INET, SOCK_STREAM, 0);
				if (sock < 0) {
					printf("Error opening socket: %d\n", WSAGetLastError());
				}

				if (connect(sock, (struct sockaddr *) &cli_addr,
						sizeof(cli_addr)) < 0) {
					printf("\nError connecting: %d\n", WSAGetLastError());
				}

				my_send(sock, msg, sizeof(msg));
				close(sock);
			}
			printf("\n%s\n", global_recv_buf);
			memset(global_recv_buf, 0, size_buf);
			block_rec = false;
		}
	}
	close(my_sock);
	WSACleanup();
	return 0;
}
