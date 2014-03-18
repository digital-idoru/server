/******************************
Daniel S. Hono II
Client 
*******************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#ifndef DEFINES
#define DEFINES

#define true 1
#define false 1 

#define PORT "40000"

#endif

/*function prototypes */
void setAddressInfo(struct addrinfo**);
int setSocket(struct addrinfo*);
void connectToServer(int, struct addrinfo*);

int main(void) {

	struct addrinfo *res;
	int sock = 0;

	/*Set up the address info */
	setAddressInfo(&res);

	/* Set up the socket file descriptor */
	sock = setSocket(res);
	
	/* Connect to the server */
	connectToServer(sock, res);


	freeaddrinfo(res); //free the results linked list when we're done 
	return 0;
}

void setAddressInfo(struct addrinfo **resultList) {

	struct addrinfo hints; //hints structure 
	int status; 

	memset(&hints, 0, sizeof hints);

	hints.ai_family = AF_UNSPEC; //v4 or v5
	hints.ai_socktype = SOCK_STREAM; //TCP
	hints.ai_flags = AI_PASSIVE; //fill in my ip for me. 

	if( (status = getaddrinfo(NULL, PORT, &hints, resultList)) != 0) { 
		fprintf(stderr, "Could not getaddrinfo~! %s\n", gai_strerror(status)); exit(EXIT_FAILURE);
	}

	return;
}

int setSocket(struct addrinfo* info) {

	return socket(info->ai_family, info->ai_socktype, info->ai_protocol);
}

void connectToServer(int socketFileDescriptor, struct addrinfo *info) {

	if(connect(socketFileDescriptor, info->ai_addr, info->ai_addrlen) != 0) {
		fprintf(stderr, "Could not connect to remote host.\n"); exit(EXIT_FAILURE);
	}

	return;
}
