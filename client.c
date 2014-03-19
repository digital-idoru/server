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

#define PORT "40000" //Server listens on port 40000
#define EXIT "EXIT"

#endif

/*function prototypes */
void setAddressInfo(struct addrinfo**);
int setSocket(struct addrinfo*);
int connectToServer(int, struct addrinfo*);
void communicate(int, char*);


int main(void) {

	struct addrinfo *res;
	int sock = 0;
 
	char recvBuffer[1024]; /* recieve msg from the server */
	char command[1024]; /*send commands to the server */

	/* Make sure the buffers have no junk data */
	memset(recvBuffer, 0, sizeof recvBuffer);
	memset(command, 0, sizeof command);

	/*Set up the address info */
	setAddressInfo(&res);	
	
	/* Connect to the server */
	sock = connectToServer(sock, res);

	/*The server should send a greeting on connection, lets catch it and print it out */
	recv(sock, (void*)recvBuffer, 1024, 0);
	printf("%s", recvBuffer);

	
	while(strncasecmp(command, EXIT, strlen(EXIT)) != 0) {

		memset(command, 0, sizeof command);
		scanf("%s", command); //get the command from stdin. The problem is that a command can be two strings. 


		communicate(sock, command); //send the command to the server.
		       
		memset(recvBuffer, 0, sizeof recvBuffer);
		if(recv(sock, (void*)recvBuffer, sizeof recvBuffer, 0) == 0) {
			fprintf(stderr, "Connection shutdown");
			exit(EXIT_FAILURE);
		} 

		printf("%s", recvBuffer); 
 
	}

	freeaddrinfo(res); //free the results linked list when we're done 
	return 0;
}

/* Sets a linked list of address results to the resultlist */
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

/*Returns a socket file descriptor to be used in the network communication */
int setSocket(struct addrinfo* info) {

	return socket(info->ai_family, info->ai_socktype, info->ai_protocol);
}

/*Connect to the server */ 
int connectToServer(int socketFileDescriptor, struct addrinfo *info) {


	struct addrinfo *probe;

	printf("Connecting to server... ");

	/*Connect to the first valid one */
	for(probe = info; probe != NULL; probe = probe->ai_next) {

		if ( (socketFileDescriptor = setSocket(probe)) == -1) {
			fprintf(stderr, "Could not get socket!\n");
			continue;
		}

		if(connect(socketFileDescriptor, probe->ai_addr, probe->ai_addrlen) != 0) {
			fprintf(stderr, "Could not connect to remote host.\n");
			continue;
		}

		if(probe == NULL) {
			fprintf(stderr, "Couldn't connect.\n");
			exit(EXIT_FAILURE);
		}

		break; //If we get to this point then everything worked, so break. 

	}

	printf("connect complete!\n"); 
	return socketFileDescriptor; 
}

/*communicate with the server while the connection is up */
void communicate(int socket, char* command) {

	int msgLength = strlen(command)+1;

       	if(send(socket, (void*)command, msgLength, 0) != msgLength) {
		printf("Error sending message.\n");
	}
       
	return;
}

