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
	int sock;

	/*Set up the address info */
	setAddressInfo(&res);


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

	if( (status = getaddrinfo(NULL, PORT, &hints, resultList)) != 0) 
		fprintf(stderr, "Could not getaddrinfo~! %s\n", gai_strerror(status)); exit(EXIT_FAILURE);


	return;
}

