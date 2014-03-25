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
#include <unistd.h>
#include <fcntl.h>

#ifndef DEFINES
#define DEFINES

#define true 1
#define false 1 

#define PORT "40000" //Server listens on port 40000
#define EXIT "EXIT"

#define CODELENGTH 3

#endif

/*function prototypes */
void setAddressInfo(struct addrinfo**);
int setSocket(struct addrinfo*);
int connectToServer(int, struct addrinfo*);
void communicate(int, char*);
void getResponse(int, char*);
void readLine(char*, int);

int main(void) {

	struct addrinfo *res;
	int sock = 0;
	char* recvBuffer; /* recieve msg from the server */
	char* command; /*send commands to the server */


	/*Need to allocate it this way to make sure we can realloc*/
	command = (char*)malloc(sizeof(char)*1024);
	if(command == NULL) {
		fprintf(stderr,"Could not allocate heap memory.\n"); exit(EXIT_FAILURE);
	}

	recvBuffer = (char*)malloc(sizeof(char)*1024);
	if(recvBuffer == NULL) {
		fprintf(stderr, "Could not allocate heap memory.\n"); exit(EXIT_FAILURE);
	}

	/* Make sure the buffers have no junk data */
	memset(recvBuffer, 0,  1024);
	memset(command, 0, 1024);

	/*Set up the address info */
	setAddressInfo(&res);	
	
	/* Connect to the server */
	sock = connectToServer(sock, res);

	/*The server should send a greeting on connection, lets catch it and print it out */
	recv(sock, (void*)recvBuffer, 1024, 0);
	printf("%s", recvBuffer);

	
	while(strncasecmp(command, EXIT, strlen(EXIT)) != 0) {

		memset(command, 0, 1024);
		fgets(command, 1024, stdin);			       

		/*Send the message to the server */
		communicate(sock, command); 

		/*Get the response from the server */
		memset(recvBuffer, 0, 1024);
		getResponse(sock, recvBuffer); 
	}

	freeaddrinfo(res); //free the results linked list when we're done 
	free(command);
	free(recvBuffer);
	close(sock);
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

	printf("Connecting to server... \n");

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

		break; //If we get to this point then everything worked, so break. 

	}

	if(probe == NULL) {
		fprintf(stderr, "Couldn't connect.\n");
		exit(EXIT_FAILURE);
	} else {

		printf("connect complete!\n"); 
	}

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

/*Read the response from the server */ 
void getResponse(int socket, char* recieved) {

	readLine(recieved, socket);
	printf("%s", recieved);

	return;
}

/*Read from the file specified by fd until there is no data left in it to be read */
void readLine(char* buffer, int fd) {

	int flags;

	/*set the socket to nonblocking */
	flags = fcntl(fd, F_GETFL);
	flags = flags | O_NONBLOCK;
	fcntl(fd, flags);

	/*Read the data from the socket*/
	while(read(fd, (void*)buffer, 1024)) {      	

		/* Instead just get the number of bytes in the file and then read until some counter with the total number of bytes read so far reaches the number of bytes
		   in the file */

		//This isn't correct 
		if(memchr(buffer, '\n', 1024) != NULL) {
			break;
		} else {
			buffer = realloc((void*)buffer, 256);
			if(buffer == NULL) {
				fprintf(stderr, "Something went wrong allocating more space!!\n"); exit(EXIT_FAILURE);
			}
		}
	}

	/*Reset original flags */
	flags = flags & ~O_NONBLOCK;
	fcntl(fd, flags);

	return;
}
