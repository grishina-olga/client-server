#ifndef SERV_CLIENT_H_
#define SERV_CLIENT_H_

#define MAX_CLIENTS 100
#define NAME_LEN 10

struct client {
	char name[NAME_LEN + 2]; //one byte for \n, second byte for \0
	int unique_id;
	int port;
	int IP;
};

#endif
