#ifndef SERV_CLIENT_H_
#define SERV_CLIENT_H_

#define MAX_CLIENTS 100
#define NAME_LEN 10

struct client {
	char name[NAME_LEN];
	int unique_id;
	int port;
	int IP;
};

#endif
