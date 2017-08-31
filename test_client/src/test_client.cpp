#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <winsock2.h>
#include <windows.h>
#include <unistd.h>
#include <string.h>
#include <gtest/gtest.h>

#define MAX_CLIENTS 100

typedef struct {
	char name[10];
	int unique_id;
	int port;
	int IP;
} client;

//extern client clients[MAX_CLIENTS];
client clients[MAX_CLIENTS] = { { "", 0, 0, 0 } };

int find_user(char* name);

char name_test_1[10] = "nadya";
char name_test_2[10] = "lolly";

TEST (FindUserTest, FindOrNotFind) {
	ASSERT_EQ(2, find_user(name_test_1));
	ASSERT_EQ(-1, find_user(name_test_2));
}

int main(int argc, char **argv) {

	char name_0[10] = "eva";
	strcat(clients[0].name, name_0);
	clients[0].unique_id = 235;

	char name_1[10] = "olga";
	strcat(clients[1].name, name_1);
	clients[1].unique_id = 236;

	char name_2[10] = "nadya";
	strcat(clients[2].name, name_2);
	clients[2].unique_id = 237;


	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}
