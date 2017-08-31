#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <winsock2.h>
#include <windows.h>
#include <unistd.h>
#include <string.h>
#include <gtest/gtest.h>

client clients[MAX_CLIENTS] = { { "", 0, 0, 0 } };

char name_test_1[10] = "nadya";
char name_test_2[10] = "olga";

TEST (UniqueNameTest, NameIsAvailableOrExists) {
	ASSERT_EQ(-1, unique_name(name_test_1));
	ASSERT_EQ(0, unique_name(name_test_2));
}


SOCKET sock_serv;
SOCKET newsockfd;

TEST (DisplayUsersTest, SendOrNotSend) {
	ASSERT_LT(-1, display_users(newsockfd));
}

int main(int argc, char **argv) {
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	char name_0[10] = "eva";
	strcat(clients[0].name, name_0);
	clients[0].unique_id = 235;

	char name_1[10] = "nadya";
	strcat(clients[1].name, name_1);
	clients[1].unique_id = 236;

	char name_2[10] = "lolly";
	strcat(clients[2].name, name_2);
	clients[2].unique_id = 237;

	sock_serv = socket(AF_INET, SOCK_STREAM, 0);
	if (sock_serv < 0) {
		printf("Error opening socket");
	}

	sockaddr_in serv_addr;
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(50400);
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	if (bind(sock_serv, (sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		printf("Error on binding");
	}
	if (listen(sock_serv, 5) < 0) {
		printf("Error on listening");
	}

	sockaddr_in cli_addr;
	int cli_len = sizeof(cli_addr);

	newsockfd = accept(sock_serv, (struct sockaddr *) &cli_addr, &cli_len);
	if (newsockfd < 0) {
		printf("Error on accept\n");
		exit(1);
		WSACleanup();
	}

	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
