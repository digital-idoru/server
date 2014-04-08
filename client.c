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
#define WELCOMEMAX 1000

#define NOFILE "403 No such file\n"

#endif

/*function prototypes */
void setAddressInfo(struct addrinfo**);
int setSocket(struct addrinfo*);
int connectToServer(int, struct addrinfo*);
void communicate(int, char*);
void readLine(int);
void getFile(int);
void sendFile(int, char*);

int main(void) {

	struct addrinfo *res;
	int sock = 0;
	char* command; /*send commands to the server */
	char welcomeMsg[WELCOMEMAX];
	

	/*Allocate space for the command */
	command = (char*)malloc(sizeof(char)*BLOCKSIZE);
	if(command == NULL) {
		fprintf(stderr,"Could not allocate heap memory.\n"); exit(EXIT_FAILURE);
	}

	/* Make sure the buffers have no junk data */
	memset(command, 0, BLOCKSIZE);

	/*Set up the address info */
	setAddressInfo(&res);	
	
	/* Connect to the server */
	sock = connectToServer(sock, res);

	/*The server should send a greeting on connection, lets catch it and print it out */
	read(sock, (void*)welcomeMsg, sizeof(char)*WELCOMEMAX);
	printf("%s", welcomeMsg);
	
	/*Interactive with the server */
	while(true) {
			     
		memset(command, 0, BLOCKSIZE); //clear the command buffer

		fgets(command, BLOCKSIZE, stdin); //read command from the server			       

		if(strncasecmp(command, EXIT, strlen(EXIT)) == 0) {
			printf("Closing connection...\n");
			close(sock); 
			printf("Connection closed. \n");
			break;			
		}

		/*Send the message to the server */
		communicate(sock, command); 

		if(strncasecmp(command, "get", 3) == 0) {			
			/*Begin file transfer */
			getFile(sock);			

		} else if(strncasecmp(command, "put", 3) == 0) { 		       
			strtok(command, " \t");
			sendFile(sock, strtok(NULL, "\n"));			
		       
		} else {			
			readLine(sock);
		}
	}
	free(command); //free the command buffer
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
void readLine(int fd) {

	int currentBytes = 0, totalBytes = 0; 
	char* intermediate = NULL;
	char* response = NULL; 
	unsigned int msgBytes = 0;
	int currentSize = BLOCKSIZE; 

	intermediate = (char*)malloc(sizeof(char)*BLOCKSIZE);
	if(intermediate == NULL) {
		fprintf(stderr, "Could not allocate heap space!\n");
		exit(EXIT_FAILURE);
	}

	response = (char*)malloc(sizeof(char)*BLOCKSIZE);
	if(response == NULL) {
		fprintf(stderr, "Could not allocate heap space!\n");
		exit(EXIT_FAILURE);
	}

	memset(intermediate, 0, sizeof(char)*BLOCKSIZE);
	memset(response, 0, sizeof(char)*BLOCKSIZE);

	/* Get the size, in bytes, of the message payload */
	read(fd, (void*)(&msgBytes), sizeof(unsigned int));
	printf("Size (in bytes) of message is: %d\n", msgBytes); //debug 

	/*Read the data from the socket*/
	while(totalBytes < msgBytes) {      		       

		currentBytes = read(fd, (void*)intermediate, BLOCKSIZE);

		/*Copy the bytes in intermediate to response+totalBytes, i.e to the end of the array */
		memcpy(response+totalBytes, intermediate, currentBytes); 

		/*Update current bytes count, and save ourselves some memory if we're done */
		totalBytes += currentBytes;
		if(totalBytes == msgBytes) {
			break;
		}

		/*Clear the intermediate buffer */
		memset(intermediate, 0, sizeof(char)*BLOCKSIZE);

		/*Expand the response array by 512 bytes */
		currentSize += BLOCKSIZE; // currentSize = currentSize + 512  
		response = (char*)realloc((void*)response, currentSize);		
		if(response == NULL) {
			fprintf(stderr, "Something went wrong allocating more space!! Moreover, a memory leak has occured.\n"); exit(EXIT_FAILURE);
		}	      		
	}

	printf("%s\n", response);
	
	/*Free the buffers */
	free(intermediate);
	free(response);
	return;
}

void getFile(int fd) {

	unsigned int fileSize = 0;
	int bytesWritten = 0; 
	int newFile = 0; 
	int currentBytes = 0; 
	char fileName[BLOCKSIZE]; 
	unsigned char buffer[BLOCKSIZE];

	memset(fileName, 0, BLOCKSIZE);
	memset(buffer, 0, BLOCKSIZE);

	/*Get the file size */
	read(fd, (void*)(&fileSize), sizeof(unsigned int));

	/*Get the File name*/
	read(fd, (void*)fileName, BLOCKSIZE);
	if(strncmp(fileName, "403", 3) == 0) {
		printf("File not found~!\n");
		return;
	}

	printf("Beginning file drop....\nTransfering file: %s\n204 Size of file (in Bytes): %d\n\n", fileName, fileSize);

	newFile = open(fileName, O_WRONLY | O_CREAT, S_IRWXU);

	printf("Transfering...");
	while(bytesWritten < fileSize) {
		
		currentBytes = read(fd, (void*)buffer, BLOCKSIZE);
		bytesWritten += write(newFile, (void*)buffer, currentBytes);
		memset(buffer, 0, BLOCKSIZE);

		if(bytesWritten == fileSize)
			break;		
	}

	printf("Transfer Complete! %d bytes written\n\n", bytesWritten);
	close(newFile);
	return;
}

void sendFile(int fd, char* filename) {

	int file; //File descriptor for the file to open.
	unsigned int fileSize = 0; //Size of the file in bytes. 
	unsigned char* buffer[BLOCKSIZE]; //Buffer for writting. 
	char* fileN; 
	int bytesWritten = 0; //Bytes written to the socket
	int bytesRead = 0; 
	
	memset(buffer, 0, sizeof(char)*BLOCKSIZE);

	file = open(filename, O_RDONLY); 
	if(file == -1) {
		printf("File not found, aborting the file transfer.\n\n");
		write(fd, (void*)&fileSize, sizeof(unsigned int));
		return;
	}

	/*Get the size, in bytes, of the file */
	fileSize = lseek(file, 0, SEEK_END);
	lseek(file, 0, SEEK_SET);
	
	/*Write the file size to the socket */
	write(fd, (void*)(&fileSize), sizeof(unsigned int));

	/*Write the filename to the socket */
	if((fileN = strrchr(filename, '/')) != NULL) { //if it's a path to the file. 
		fileN++;
		write(fd, (void*)fileN, strlen(fileN)+1);
	} else {
		write(fd, (void*)filename, (strlen(filename)+1)); //if it's a file in the current directory. 
	}
	
	while(bytesWritten < fileSize) {

		bytesRead = read(file, (void*)(buffer), 512);
		bytesWritten += write(fd, (void*)(buffer), bytesRead);

		if(bytesWritten == fileSize) {
			break;
		}

		memset(buffer, 0, 512);
	}
	
	close(file);
	return;
}
