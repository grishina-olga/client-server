#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <winsock2.h>
#include <windows.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

//#include <pthread.h>

DWORD WINAPI Recv_serv(LPVOID newsock);
DWORD WINAPI Recv_cli(LPVOID newsock);
DWORD WINAPI Send(LPVOID newsock);

char name[10];
bool isExit = false;

void quit(int sock) {
	send(sock, "quit", sizeof("quit"), 0);
	printf("\nYou are now quitting!\n");
	close(sock);
	WSACleanup();
	exit(1);
}

void ctrl_c(int a) {
	isExit = true;
}

int main(int argc, char *argv[]) {
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	if (argc < 3) {
		printf("usage %s hostname port\n", argv[0]);
		return 0;
	}

	struct hostent *server;
	server = gethostbyname(argv[1]);
	if (server == NULL) {
		printf("ERROR, no such host\n");
		exit(2);
		WSACleanup();
	}

	sockaddr_in serv_addr;
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(atoi(argv[2]));
	memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);

	SOCKET sock_serv;
	sock_serv = socket(AF_INET, SOCK_STREAM, 0);
	if (sock_serv < 0) {
		printf("Error opening socket");
		exit(1);
		WSACleanup();
	}

	if (connect(sock_serv, (struct sockaddr *) &serv_addr, sizeof(serv_addr))
			< 0) {
		printf("\nError connecting");
		exit(2);
		WSACleanup();
	}

	printf("Connected... \n\n");

	char active[1000];
	memset(active, 0, 1000);
	int n = recv(sock_serv, active, sizeof(active), 0);
	if (n < 0) {
		printf("\nError reading from socket");
		exit(3);
		WSACleanup();
	}
	printf("The active users are:\n%s", active);

	printf("\nEnter your name: ");

	signal(SIGINT, ctrl_c);

	fgets(name, 10, stdin);

	int size = strlen(name) - 1;
	name[size] = '\0';
	send(sock_serv, name, sizeof(name), 0);

	char msg[100];
	do {
		memset(msg, 0, 1000);
		int n = recv(sock_serv, msg, sizeof(msg), 0);
		if (n < 0) {
			printf("\nError reading from socket");
			printf("\nServer disconnected!");
			exit(3);
			WSACleanup();
		}

		if (strcmp(msg, "err") == 0) {
			printf("The server is full, please try later!\n");
			close(sock_serv);
			exit(1);
		}

		if (strcmp(msg, "accepted") == 0) {
			printf("\nThe name was successfully submitted!\n");
		} else {
			printf("\nThis name is already in use. Please, choose another\n");
			fgets(name, 10, stdin);
			int size = strlen(name) - 1;
			name[size] = '\0';
			send(sock_serv, name, sizeof(name), 0);
		}
	} while (strcmp(msg, "exists") == 0);

	SOCKET sock_cli;
	sock_cli = socket(AF_INET, SOCK_STREAM, 0);
	if (sock_cli < 0) {
		perror("Error opening socket");
		exit(1);
		WSACleanup();
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
			printf("Error on binding");
			exit(2);
			WSACleanup();
		}
		cli_addr.sin_port = htons(++addr);
	}

	send(sock_serv, (char*) &addr, sizeof((char*) &addr), 0);

	if (listen(sock_cli, 5) < 0) {
		perror("Error on listening");
		exit(3);
		WSACleanup();
	}

	printf("\nYou are ready. \n"
			"Type something to send a message\n"
			"Type 'quit' 	  to quit\n"
			"Type 'user list'  to get a user list\n\n");

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

	quit(sock_serv);

	close(sock_serv);
	close(sock_cli);

	CloseHandle(hThread_recv_s);
	CloseHandle(hThread_recv_c);
	CloseHandle(hThread_send);
	return 0;
}

DWORD WINAPI Recv_serv(LPVOID newsock) {
	SOCKET my_sock;
	my_sock = ((SOCKET *) newsock)[0];

	while (1) {
		char msg[100];
		memset(msg, 0, 100);
		int n = recv(my_sock, msg, sizeof(msg), 0);
		if (n < 0)
			break;

		if (strcmp(msg, "quit") == 0) {
			close(my_sock);
			WSACleanup();
			exit(1);
		}
		printf("\n%s\n", msg);
	}

	printf("\nServer disconnected!");
	isExit = true;
	close(my_sock);
	WSACleanup();
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
			perror("Error on accept\n");
			exit(4);
			WSACleanup();
		}


		int n =	recv(newsockfd, msg, sizeof(msg), 0);
		if (n < 0)
			break;
		printf("\n%s\n", msg);

	}
	printf("\nServer disconnected!");
	isExit = true;
	close(my_sock);
	WSACleanup();
	return 0;
}

DWORD WINAPI Send(LPVOID newsock) {
	signal(SIGINT, ctrl_c);

	SOCKET my_sock;
	my_sock = ((SOCKET *) newsock)[0];

	char msg[100];
	char mes[100];
	char receiver[10];
	char del[] = ":";

	signal(SIGINT, ctrl_c);

	while (1) {

		fgets(mes, sizeof(mes), stdin);

		if (strcmp(mes, "quit\n") == 0)
			quit(my_sock);

		if (strcmp(mes, "user list\n") == 0)
			send(my_sock, "user list", sizeof("user list"), 0);

		int size = strlen(mes) - 1;
		mes[size] = '\0';

		printf("to(port):");
		fgets(receiver, 10, stdin);

		size = strlen(receiver) - 1;
		receiver[size] = '\0';

		strcpy(msg, name);
		strcat(msg, del);
		strcat(msg, mes);

		sockaddr_in cli_addr;
		memset(&cli_addr, 0, sizeof(cli_addr));
		cli_addr.sin_family = AF_INET;
		cli_addr.sin_port = htons(atoi(receiver));
		cli_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

		SOCKET sock;
		sock = socket(AF_INET, SOCK_STREAM, 0);
		if (sock < 0) {
			printf("Error opening socket");
			exit(1);
			WSACleanup();
		}

		if (connect(sock, (struct sockaddr *) &cli_addr, sizeof(cli_addr))
				< 0) {
			printf("\nError connecting");
			//exit(2);
			//WSACleanup();
		}

		int n = send(sock, msg, sizeof(msg), 0);
		if (n < 0) {
			printf("\nError sending message");
		}
		close(sock);
	}
	close(my_sock);
	WSACleanup();
	return 0;
}

