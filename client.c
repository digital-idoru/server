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
#define BLOCKSIZE 512


#endif

/*function prototypes */
void setAddressInfo(struct addrinfo**);
int setSocket(struct addrinfo*);
int connectToServer(int, struct addrinfo*);
void communicate(int, char*);
void readLine(char**, int);
void getFile(int);

int main(void) {

	struct addrinfo *res;
	int sock = 0;
	char* recvBuffer; /* recieve msg from the server */
	char* command; /*send commands to the server */


	/*Need to allocate it this way to make sure we can realloc*/
	command = (char*)malloc(sizeof(char)*BLOCKSIZE);
	if(command == NULL) {
		fprintf(stderr,"Could not allocate heap memory.\n"); exit(EXIT_FAILURE);
	}

	recvBuffer = (char*)malloc(sizeof(char)*BLOCKSIZE);
	if(recvBuffer == NULL) {
		fprintf(stderr, "Could not allocate heap memory.\n"); exit(EXIT_FAILURE);
	}

	/* Make sure the buffers have no junk data */
	memset(recvBuffer, 0, BLOCKSIZE);
	memset(command, 0, BLOCKSIZE);

	/*Set up the address info */
	setAddressInfo(&res);	
	
	/* Connect to the server */
	sock = connectToServer(sock, res);



	/*The server should send a greeting on connection, lets catch it and print it out */
	recv(sock, (void*)recvBuffer, BLOCKSIZE, 0);
	printf("%s", recvBuffer);

	
	while(true) {

		memset(command, 0, BLOCKSIZE);
		memset(recvBuffer, 0, BLOCKSIZE);
		
		fgets(command, BLOCKSIZE, stdin);			       

		if(strncasecmp(command, EXIT, strlen(EXIT)) == 0) {
			break;
		}

		/*Send the message to the server */
		communicate(sock, command); 

		if(strncasecmp(command, "get", 3) == 0) {
			getFile(sock);			
		} else {
			
			/*Get the response from the server */
			recvBuffer = (char*)realloc(recvBuffer, BLOCKSIZE); //This thing can get huge so we reset it back to BLOCKSIZE 
			if(recvBuffer == NULL) {
				fprintf(stderr, "Holy hell some memory is now floating in the ether.\n"); exit(EXIT_FAILURE);
			}
		
	
			readLine(&recvBuffer, sock);
			printf("%s", recvBuffer);
		}
	}


	free(command); //free the command buffer
	free(recvBuffer); //free the recieved buffer 
	close(sock); //close our connection 
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

	freeaddrinfo(info); //free the results linked list when we're done 
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

/*Read from the file specified by fd until there is no data left in it to be read */
void readLine(char** buffer, int fd) {

	int i = 1, currentBytes = 0; 
	char* intermediate;
	unsigned int msgBytes = 0;

	intermediate = (char*)malloc(sizeof(char)*BLOCKSIZE);
	if(intermediate == NULL) {
		fprintf(stderr, "Could not allocate heap space!\n");
		exit(EXIT_FAILURE);
	}

	memset(intermediate, 0, sizeof(char)*BLOCKSIZE);

	/* Get the size, in bytes, of the message payload */
	read(fd, (void*)(&msgBytes), sizeof(unsigned int));
	printf("Size (in bytes) of message is: %d\n", msgBytes); //debug 

	/*Read the data from the socket*/
	while(currentBytes < msgBytes) {      		       

		currentBytes += read(fd, (void*)intermediate, BLOCKSIZE);

		strncat(*buffer, intermediate, sizeof(char)*currentBytes);
		memset(intermediate, 0, sizeof(char)*BLOCKSIZE);

		*buffer = realloc((void*)*buffer, BLOCKSIZE*(2*i));		
		if(buffer == NULL) {
			fprintf(stderr, "Something went wrong allocating more space!!\n"); exit(EXIT_FAILURE);
		}
	      
		i++;
	}

	free(intermediate);
	return;
}

void getFile(int fd) {

	unsigned int fileSize = 0;
	int bytesWritten = 0; 
	int bytesRead = 0;
	int newFile = 0; 
	char fileName[256]; 
	unsigned char buffer[256];

	memset(fileName, 0, 256);
	memset(buffer, 0, 256);

	/*Get the file size */
	read(fd, (void*)(&fileSize), sizeof(unsigned int));

	/*Get the File name*/
	read(fd, (void*)fileName, 256);

	printf("Beginning file drop....\nTransfering file: %s\n204 Size of file (in Bytes): %d\n\n", fileName, fileSize);

	newFile = open(fileName, O_WRONLY | O_CREAT, S_IRWXU);

	printf("Transfering...");
	while(bytesWritten < fileSize) {
		
		bytesRead += read(fd, (void*)buffer, 256);
		bytesWritten += write(newFile, (void*)buffer, 256);
		memset(buffer, 0, 256);

		if(bytesWritten == fileSize)
			break;

		printf(".");
	}

	printf("Transfer Complete! %d bytes written\n\n", bytesWritten);
	close(newFile);
	return;
}
