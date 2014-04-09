/******************************
Daniel S. Hono II
Client
CSI416 Project 2
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

/* Basic booleans*/
#define true 1
#define false 1 

#define PORT "40000" //Server listens on port 40000
#define NOFILE "403 No such file\n" //File open error to send to the server. 
#define FILEURL "file://" //File url. 
#define EXIT "EXIT" //Exit command from the user, used for pattern matching. 
#define QUIT "QUIT" //Quit message to send to the server on client exit. 
#define ENDLINE "\n" //End of line. 


//#define CODELENGTH 3 
#define BLOCKSIZE 512 //for reading and writting files in blocks of size 512 bytes
#define WELCOMEMAX 1000 //Maximum size allowed for the welcome message from the server upon connection. 
#define FILECMDLEN 3 //For pattern matching on get and put commands. 
#define DIRCMDLEN 2 //For pattern matching on cd command. 
#define ERRCODELEN 3 //For pattern matching error codes from the server on file transfers 
#define FILEFAILURE -1 //open() returns -1 on failure. 


#endif

/*Function prototypes */
void setAddressInfo(struct addrinfo**);
int setSocket(struct addrinfo*);
int connectToServer(int, struct addrinfo*);
void communicate(int, char*);
void readLine(int);
void getFile(int);
void sendFile(int, char*);
char* setFileURL(char*); 

int main(void) {

	struct addrinfo *res; //for the connection description and hints. 
	int sock = 0; //for the socket file descriptor
	char* command; //Send commands to the server
	char welcomeMsg[WELCOMEMAX]; //Buffer to hold the welcome message. 


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
	
	/*Interact with the server */
	while(true) {
			     
		/*Clear the command buffer of junk*/
		memset(command, 0, BLOCKSIZE);

		/*Read command from the user*/
		fgets(command, BLOCKSIZE, stdin); 

		/*Check if the user has entered the exit command*/
		if(strncasecmp(command, EXIT, strlen(EXIT)) == 0) {
			printf("Closing connection...\n");
			write(sock, (void*)QUIT, sizeof QUIT);  
			readLine(sock);
			printf("Connection closed.\n");	
			free(command);
			return 0;		
		}

		/*Send the message to the server */	
		if(strncasecmp(command, "cd", DIRCMDLEN) == 0) {
			//communicate(sock, cd); 
			//tmp = setFileURL(command);
			communicate(sock, command);
			//free(tmp);
			readLine(sock);
		}else if(strncasecmp(command, "get", FILECMDLEN) == 0) {
			communicate(sock, command);
			getFile(sock); //read file from the socket		
		} else if(strncasecmp(command, "put", FILECMDLEN) == 0) { 
			communicate(sock, command);
			strtok(command, " \t");
			sendFile(sock, strtok(NULL, "\n")); //send file to server. 				       
		} else {
			communicate(sock, command);
			readLine(sock);
		}
	}
	free(command); //free the command buffer
	return 0;
}

/* Sets a linked list of address results to the resultlist */
void setAddressInfo(struct addrinfo **resultList) {

	struct addrinfo hints; //hints structure 
	int status; //Used for error checking on getaddrinfo return type. 

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
/*Parameter info: addrinfo structure to be used in setting up the socket details */
int setSocket(struct addrinfo* info) {

	return socket(info->ai_family, info->ai_socktype, info->ai_protocol);
}

/*Connect to the server */ 
/*Parameter socketFileDescriptor: file descriptor to be used for the socket*/
/*Parameter info: addrinfo structure linked listused in setting up the connection*/
/*Return: socket file descriptor to a legitmate connection with the server*/
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

	freeaddrinfo(info); //free the results linked list when we're done.
	return socketFileDescriptor; //return the connected socket file descriptor. 
}


/*Function that takes a user-friendly standard file path and creates a file URL to send to the server */
/*Parameters filepath: User friendly file path that needs to be converted to the file url scheme */
char* setFileURL(char* filepath) {

	char* filepathURI = NULL; //Pointer to the completed file url string. 
	char* pathToken = NULL; //Pointer to the path token in the user's command. 

	/*Get the path from the command */
	strtok(filepath, " "); 
	pathToken = strtok(NULL, "\n"); 
	if(pathToken == NULL) {
		printf("Invalid path");
		return NULL;
	}
	printf("The path you entered hopefully is:%s\n", pathToken); //debug 
	
	/*Allocate space for a new complete file URL string */
	filepathURI = (char*)malloc(sizeof(char)*(strlen(pathToken) + sizeof FILEURL + 2));
	if(filepathURI == NULL) {
		fprintf(stderr, "Could not allocate heap space, shutting down.\n"); exit(EXIT_FAILURE);
	}

	memset(filepathURI, 0, sizeof(char)*(strlen(pathToken) + sizeof FILEURL + 2));

	/*Build the file url string up */
	strncat(filepathURI, FILEURL, sizeof FILEURL);
	strncat(filepathURI, pathToken, strlen(pathToken));
	//strncat(filepathURI, ENDLINE, sizeof ENDLINE);  
	printf("The complete file url is:%s", filepathURI); //debug

	/*Return it*/ 
	return filepathURI;
}


/*communicate with the server while the connection is up */
/*Parameter socket: open file descriptor of a socket connected to the server*/
/*Parameter command: Command to send to the server*/
void communicate(int socket, char* command) {

	int msgLength = strlen(command)+1; //+1 for null terminator 
       	if(send(socket, (void*)command, msgLength, 0) != msgLength) {
		printf("Error sending message.\n");
	}
       
	return;
}

/*Read from the file specified by fd until there is no data left in it to be read */
/*Expands a buffer to read text messages from the server, which sometimes can be large due to the ls command */
void readLine(int fd) {

	int currentBytes = 0, totalBytes = 0; //current bytes read from the server, totalbytes read from the server.  
	char* intermediate = NULL; //Buffer to read from the socket. 
	char* response = NULL; //Expanding buffer to hold text response from the server. 
	unsigned int msgBytes = 0; //Size of the message in bytes. 
	int currentSize = BLOCKSIZE; //start off response buffer as blocksize bytes in length. 

	/*Allocate space for the buffers */
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

	/*Clear the buffers of any junk data.*/
	memset(intermediate, 0, sizeof(char)*BLOCKSIZE);
	memset(response, 0, sizeof(char)*BLOCKSIZE);

	/* Get the size, in bytes, of the message payload */
	read(fd, (void*)(&msgBytes), sizeof(unsigned int));
	printf("Size (in bytes) of message is: %d\n", msgBytes); //debug 

	/*Read the data from the socket*/
	while(totalBytes < msgBytes) {      		       

		/*read BLOCKSIZE bytes from the socket into the intermediate buffer*/
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

	/*Print the response from the server in its entirety.*/
	printf("%s\n", response);
	
	/*Free the buffers */
	free(intermediate);
	free(response);
	return;
}

/* Function to recieve a file from the server after a get command has been issued */
/* Parameter fd: File descriptor of open socket connection to the server */
void getFile(int fd) {

	unsigned int fileSize = 0; //Size of the file being read. 
	int bytesWritten = 0;  //Number of bytes written to the newly created local file. 
	int newFile = 0; //file descriptor for the newly created local file. 
	int currentBytes = 0; //Current # of bytes read from the socket. 
	char fileName[BLOCKSIZE]; //Name of the file sent from the server. 
	unsigned char buffer[BLOCKSIZE]; //buffer to hold blocks of data written to the socket from the server. 

	/*Clear the buffers */
	memset(fileName, 0, BLOCKSIZE);
	memset(buffer, 0, BLOCKSIZE);

	/*Get the file size */
	read(fd, (void*)(&fileSize), sizeof(unsigned int));

	/*Get the File name*/
	read(fd, (void*)fileName, BLOCKSIZE);
	if(strncmp(fileName, "403", ERRCODELEN) == 0) { //If the filename written by the server is "403: File not Found", then we return to main. 
		printf("File not found~!\n");
		return;
	}

	printf("Beginning file drop....\nTransfering file: %s\n204 Size of file (in Bytes): %d\n\n", fileName, fileSize); //friendly message. 

	newFile = open(fileName, O_WRONLY | O_CREAT, S_IRWXU); //Open a new file to write to. 
	printf("Transfering...");

	/*Read the bytes from the socket, and write them to the newly created file*/
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

/*Function to send a file to the server after the issueing of a put command*/
/*Parameters: fd: File descriptor of the open socket with the server */
/*Parameters: filename, either a full user-friendly path to a file, or a file in the current directory*/
void sendFile(int fd, char* filename) {

	int file; //File descriptor for the file to open.
	unsigned int fileSize = 0; //Size of the file in bytes. 
	unsigned char* buffer[BLOCKSIZE]; //Buffer for writting. 
	char* fileN; //Pointer to a tokenized filename if needed 
	int bytesWritten = 0; //Bytes written to the socket
	int bytesRead = 0; //Bytes read from the file being sent. 
	
	/*Clear the buffer */
	memset(buffer, 0, sizeof(char)*BLOCKSIZE);

	/*Open file for reading. If it doesn't exist, send the failure protocal to the server*/
	file = open(filename, O_RDONLY); 
	if(file == FILEFAILURE) {
		printf("File not found, aborting the file transfer.\n\n"); //Display error message to the client. 
		write(fd, (void*)&fileSize, sizeof(unsigned int)); //Write a filesize of 0 to the server
		return;
	}

	/*Get the size, in bytes, of the file */
	fileSize = lseek(file, 0, SEEK_END);
	lseek(file, 0, SEEK_SET);
	
	/*Write the file size to the socket */
	write(fd, (void*)(&fileSize), sizeof(unsigned int));

	/*Write the filename to the socket */
	if((fileN = strrchr(filename, '/')) != NULL) { //if it's a full user-friendly path to the file. 
		fileN++;
		write(fd, (void*)fileN, strlen(fileN)+1);
	} else {
		write(fd, (void*)filename, (strlen(filename)+1)); //if it's a file in the current directory. 
	}
	
	/*Write until the total number of bytes in the file being sent is written to the socket */
	while(bytesWritten < fileSize) {

		/*Read the data from the file and write it to the socket for the server to read.*/
		bytesRead = read(file, (void*)(buffer), BLOCKSIZE);
		bytesWritten += write(fd, (void*)(buffer), bytesRead);

		/*If we've written everything to the socket, break*/
		if(bytesWritten == fileSize) {
			break;
		}

		/*Clear the buffer of data after we don't need it anymore*/
		memset(buffer, 0, BLOCKSIZE);
	}
	
	close(file);
	return;
}
