#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <winsock2.h>
#include <windows.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include "cli_client.h"

extern client clients[MAX_CLIENTS];

int my_send(SOCKET s, const char* buf, int size){
	int n = send(s, buf, size, 0);
	if (n < 0) {
		printf("Error sending\n");
	}
	return n;
}

void trim(char *s) {
	unsigned int i = 0, j;
	while ((s[i] == ' ') || (s[i] == '\t')) {
		i++;
	}
	if (i > 0) {
		for (j = 0; j < strlen(s); j++) {
			s[j] = s[j + i];
		}
		s[j] = '\0';
	}
	i = strlen(s) - 2;
	while ((s[i] == ' ') || (s[i] == '\t')) {
		i--;
	}
	if (i < (strlen(s) - 2 )) {
		s[i + 2] = '\0';
	}
}

int find_user(char* name) {
	int counter = 0;
	while (counter < MAX_CLIENTS && clients[counter].unique_id != 0) {
		if (strcmp(clients[counter].name, name) == 0) {
			return counter;
		}
		counter++;
	}
	return -1;
}

void quit(int sock) {
	my_send(sock, "quit", strlen("quit"));
	printf("\nYou are now quitting!\n");
	close(sock);
	WSACleanup();
	exit(1);
}
