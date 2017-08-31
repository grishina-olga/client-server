#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <winsock2.h>
#include <windows.h>
#include <unistd.h>
#include <string.h>
#include "serv_client.h"

extern client clients[MAX_CLIENTS];

int my_send(SOCKET s, const char* buf, int size){
	int n = send(s, buf, size, 0);
	if (n < 0) {
		printf("Error sending: %d\n", WSAGetLastError());
	}
	return n;
}

int display_users(SOCKET sockfd) {
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

	int n = my_send(sockfd, (char*) clients, sizeof(clients));
	return n;
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
