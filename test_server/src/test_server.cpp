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


int send(SOCKET s, const char* buf, int size, int flag) {
	if (size >= 0)
		return size;
	else
		return -1;
}
int WSAGetLastError(){
	return 1;
}

TEST (UniqueNameTest, NameIsAvailableOrExists) {
	ASSERT_EQ(-1, unique_name(name_test_1));
	ASSERT_EQ(0, unique_name(name_test_2));
}

SOCKET fake_sock;
char msg[] = "Hello!";

TEST (MySend, SendOrNotSend) {
	ASSERT_LT(-1, my_send(fake_sock, msg, sizeof(msg)));
}

int main(int argc, char **argv) {

	char name_0[10] = "eva";
	strcat(clients[0].name, name_0);
	clients[0].unique_id = 235;

	char name_1[10] = "nadya";
	strcat(clients[1].name, name_1);
	clients[1].unique_id = 236;

	char name_2[10] = "lolly";
	strcat(clients[2].name, name_2);
	clients[2].unique_id = 237;

	fake_sock = 126;

	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
