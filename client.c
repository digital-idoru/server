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



int main(void) {

	struct addrinfo *res;


	/*Set up the address info */
	setAddressInfo(&res);


	freeaddrinfo(res);
	return 0;
}

void setAddressInfo(struct addrinfo **resultList) {

	struct addrinfo hints;
	int status; 

	memset(&hints, 0, sizeof hints);

	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE; 

	if( (status = getaddrinfo(NULL, PORT, &hints, resultList)) != 0) 
		fprintf(stderr, "Could not getaddrinfo~!\n"); exit(EXIT_FAILURE);


	return;
}

