#ifndef SERV_CLIENT_H_
#define SERV_CLIENT_H_

#define MAX_CLIENTS 4

struct client {
	char name[10];
	int unique_id;
	int port;
	int IP;
};

#endif
