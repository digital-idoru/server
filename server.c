/************************************
Daniel Hono II
CSI416 Project 2
A Simple Server
*************************************/

/*************************************
Message protocal:
payload size (in bytes) followed by that many bytes.
These are SEPERATE write calls. The client first reads the payload size,
then reads payload-size bytes from the socket.
*************************************/



#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>

#ifndef DEFINES
#define DEFINES

/*Standard booleans*/
#define true 1
#define false 0

#define PORT_PARAM 1
#define BACKLOG 10 //backlog for connections 
#define BUFSIZE 250 //a buffer. 
#define PORT 40000 //Server listens on port 40000
#define PORT_STRING_LEN 6 //size of the port string used in some function calls. 
#define PATH_SIZE 256 //maximum path size for files
#define FILENAME_SIZE 1024 //maximum file name size in bytes
#define BLOCKSIZE 512 //read and write files in 512 bytes. 

#define ENDLINE "\n"
#define NOFILE "403 No such file\n"
#define BEGIN "FILESET"

#endif

/*Define a boolean type */
typedef uint8_t bool;  

/*Note uint8_t is the same as a byte */

/*Function prototypes */
void setAddressInfoStandard(const char*, struct addrinfo, struct addrinfo**); //set address info with some standard params.
int createSocket(struct  addrinfo *); //Function to return the fd of a socket
void msgSend(int, char*, unsigned int); //Function to send a text based message to the client. 
bool checkHelo(char*); //Function to check if the HELO command was issued properly.
char* sanatize(char*); //Function to remove extra symbols added by telnet.
bool checkFullMsg(char*, int); //Function to check if entire msg has arrived. 
void server(struct addrinfo*);  //Handles the main loop of the server
void myBind(struct addrinfo*, int); //Binds a socket to the port 
void myListen(int); //Listens on a port with a bound socket 
void commands(char*, int*, bool*); //handles the commands from the client
char* getDirectoryPath(char*); //Extracts directory path from correctly formatted command
void lsCommand(int); //lists all files in the current directory. 
void resetWorkingDirectory(char*); //Reset the directory after client exits. 
void sendFile(int, char*); //Sends a file, if it exist, to the client. Get command.  
void getFile(int); //Read a file from the client for the put command. 


/*Response messages to client commands. */
char* welcome_msg = "\n\n~La Vie Est Drole~\n\n"; //HARIME. NUI. 
char* helo = "220 Connection Established.\n\n"; 
char* goodbye = "100 Don't go away mad, just go away.\n\n";
char* unrec = "401 Unrecognized Input.\n\n"; 
char* nHelo = "550 You must give the HELO command.\n\n";
char* rHELO = "450 HELO already issued\n\n"; 
char* dirCh = "201 Directory changed\n\n";
char* dirChF = "401 Directory change failed.\n\n";
char* nD = " 402 No current directory.\n\n";
char* getNF = "403 No such file.\n\n";
char* list = "203 list follows, terminated by .\n\n";



int main(void) {

	char nodeName[PORT_STRING_LEN]; //String to hold the port number. 
	struct addrinfo hints; //Hints to send to getAddressInfo
	struct  addrinfo *servinfo; //Pointer to the linked list of results. 

	//We need to make sure that the hints struct is empty to start with. 
	memset(&hints, 0, sizeof(hints)); 

	//Get the address information set up. 
	sprintf(nodeName,"%d", PORT);
	setAddressInfoStandard(nodeName, hints, &servinfo);

	/* Main loop to handle the server */ 
	server(servinfo);

	/* Free the address info linked list at the end of the program */
	freeaddrinfo(servinfo); 
	return 0;
}

//Function to check if the msg recieved is the correct HELO format, i.e. HELO followed by a domain name. 
//Paramter is a string, the message to be checked.
//Returns true if correct HELO, false if not. 
bool checkHelo(char* msg) {

	if(strcasecmp(strtok(msg, "\t "), "HELO") != 0)
		return false; 
 
	return true;  
}

//Function to remove characters added by telnet. 
//Returns a sanatized string
//Paramter is the recieved message as a string. 
char* sanatize(char* msg) {
	char* sMsg;

	if( (sMsg = strtok(msg, "\r\n")) != NULL) {
		return sMsg;
	} else {
		return msg;
	}
}

//Function to check if entire msg has been recieved from the user. 
//msg may be recieved in fragments, need to check if \0 is present. 
//Returns 1 if entire msg is present, i.e. \0 or CR has been recieved. 
//paramters are the message recieved to be checked and the length of that recieved message in bytes. 
bool checkFullMsg(char* msg, int msgLength) {

	if(memchr((void*)msg, '\n', (size_t)msgLength) != NULL) {
		return true; 
	} else if (memchr((void*)msg, 0, (size_t)msgLength) != NULL) {
		return true;
	} else {
		return false;
	}
}

/*Function to set addressinfo with some standard parameters*/
void setAddressInfoStandard(const char* port, struct addrinfo hints, struct addrinfo** results) {

	int status; //Status for the call to getAddrInfo().

	//Now we fill out the hints structure. 
	hints.ai_family = AF_UNSPEC; 		//Don't care about the IP type, v4 or v6. 
	hints.ai_socktype = SOCK_STREAM; 	//We're using stream sockets since we need two-way communications and reliability. 
	hints.ai_flags = AI_PASSIVE;		//Figure out the ip for me. Localhost, just fill it in. 

	//Now we make the call to getAddrInfo with the correct error checking for bad status returns. 
	if((status = getaddrinfo(NULL, port, &hints, results)) != 0) {
		fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
		exit(EXIT_FAILURE);
	}
	return;
}

//Create a socket and return the fd of the socket. 
//Paramters are the struct addrinfo to be used. 
int createSocket(struct addrinfo *res) {

	int socketFileDescriptor; //file descriptor of the socket. 	

	//socket returns a negative number on error.
	//Apparently the SOCK_STREAM flavor of socket is reliabile for two-way communcations, i.e. things arrive in the same order as they are sent.
	if((socketFileDescriptor = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) < 0) {
		fprintf(stderr, "There was a problem creating the socket!\n");
		exit(EXIT_FAILURE); //Exit if we can't create the socket. 
	}
	return socketFileDescriptor;
}


/*Function to send message to client.*/
void msgSend(int cS, char* message, unsigned int payloadSize) {

	/*Send payload size */
	if(write(cS, (void*)(&payloadSize), sizeof(unsigned int)) < 0) {
		fprintf(stderr, "Could not send payload size to client!\n"); exit(EXIT_FAILURE);
	}

	/*Send the data*/
	if(write(cS, (void*)message, payloadSize) < 0) {
		fprintf(stderr, "Error sending message to client!\n");
		exit(EXIT_FAILURE);
	}
	return;
}

/*Bind the socket to the port */
void myBind(struct addrinfo* servinfo, int socketFd) {
	if(bind(socketFd, servinfo->ai_addr, servinfo->ai_addrlen) < 0) {
		fprintf(stderr, "Error binding to port!\n");
		exit(EXIT_FAILURE);
	}
	return;
}

/* Listen on the port for connections */
void myListen(int socketFd) {
	if(listen(socketFd, BACKLOG) < 0) {
		fprintf(stderr, "Error Listening on socket #: %d\n", socketFd);
		exit(EXIT_FAILURE);
	}
	return;
}

/* Function to handle the server */
void server(struct addrinfo *servinfo) {

	/* File descriptors for the server and client sockets resp. */
	int socketFd = 0, clientFd = 0; 
	struct sockaddr_storage client_addr; 	//Hold information about the client connection from an accept()
	socklen_t addr_size; 			//size of sockaddr struct to be used in the accept();

	unsigned int len; 			//Length msg to send in bytes. 
	int rBuff;				//size of the recieve buffer
	int recSize;				//size of the recieved msg.
	bool ack;				//acknowledge flag of HELO command


	/*Set up the recieved buffer */
	char buffer[BUFSIZE]; 
	char* sBuffer;

	char origin[PATH_SIZE];

	memset(buffer, 0, strlen(buffer)); 
	memset(origin, 0, PATH_SIZE);

	rBuff = BUFSIZE;

	/*Hello and active directory start off as false and original */
	ack = false;

	if(getcwd(origin, sizeof(origin)) == NULL) {
		fprintf(stderr, "Could not get cwd\n");
		exit(EXIT_FAILURE);
	}
	

	/*Create the server socket */
	socketFd = createSocket(servinfo);

	/*Bind to the port */
	myBind(servinfo, socketFd);

	/*Listen at that port*/
	myListen(socketFd);

	addr_size = sizeof(client_addr); 

	//Handle connections. Looping until server is "manually" forced to stop.  
	while(true) {

		//If we don't have an active client socket file descriptor. 
		if(clientFd == 0) {
			//block and wait for a connection. 
			if((clientFd = accept(socketFd, (struct sockaddr *)&client_addr, &addr_size)) < 0) {
				fprintf(stderr, "Error accepting the connection!\n");
				exit(EXIT_FAILURE);
			}

			//Send the welcome message. 
			len = strlen(welcome_msg);
			if(send(clientFd,  welcome_msg, len, 0) < 0) {
				fprintf(stderr, "Error sending msg!\n");
				exit(EXIT_FAILURE);
			}

			resetWorkingDirectory(origin); //set the directory to the start. 
		}

		/*Reset the read buffer */
		memset(buffer, 0, rBuff);

		do {
			//Recieve the message from the connected client.
			if((recSize = recv(clientFd, buffer, rBuff, 0)) <= 0) {
				fprintf(stderr, "Error recieving data or client has closed connection!\n");
				exit(EXIT_FAILURE);
				
			}
			
			printf("Message recieved: %s\n", buffer); //debug

		}while(checkFullMsg(buffer, recSize) != true);

		printf("Processing msg\n"); //debug 

		//Need to remove the special characters appended to the string by the telnet. 
		sBuffer = sanatize(buffer); 

		if(ack == true) {
			
			/* Process commands only after the hello */
			commands(sBuffer, &clientFd, &ack);

		} else if(checkHelo(sBuffer) == true) {
			len = strlen(helo)+1;		
			msgSend(clientFd, helo, len);
			ack = true;

		} else if(ack != true){
			len = strlen(nHelo)+1;
			printf("sending message\n"); //debug 
			msgSend(clientFd, nHelo, len);
		}

	} //End While

	return;
}

/*Process the commands from the client*/
void commands(char* sBuffer, int* clientFd, bool *ack) {

  
	char cwd[PATH_SIZE], fullPath[PATH_SIZE+512]; 
	unsigned int msgBytes = 0;
	char* filename; 

	memset(cwd, 0, sizeof(char)*PATH_SIZE);
	memset(fullPath, 0, sizeof(char)*PATH_SIZE+512);

	if(strcasecmp(sBuffer, "quit") == 0) { 

		/*Close the connection */
		msgSend(*clientFd, goodbye, (strlen(goodbye)+1)); 
		close(*clientFd); 
		*clientFd = 0; 
		*ack  = false; 
		return;

	} else if(strncasecmp(sBuffer, "helo", 4) == 0) {		
		
		msgBytes = strlen(rHELO)+1;
		msgSend(*clientFd, rHELO, 0);

	} else if(strncasecmp(sBuffer, "cd", 2) == 0) {

		if(chdir(getDirectoryPath(sBuffer)) == 0) {

			msgBytes = strlen(dirCh)+1;
			msgSend(*clientFd, dirCh, msgBytes);

		} else {
			
			msgBytes = strlen(dirChF)+1;
			msgSend(*clientFd, dirChF, msgBytes);

		}	       

	} else if(strncasecmp(sBuffer, "pwd", 3) == 0) {

		strncat(fullPath, "200 ", 4); 
		strncat(fullPath, "file://", 7);

		if(getcwd(cwd, sizeof(cwd)) != NULL) {

			strncat(fullPath, cwd, strlen(cwd));
			strncat(fullPath, ENDLINE, sizeof(ENDLINE));

			msgBytes = strlen(fullPath)+1;
			msgSend(*clientFd, fullPath, msgBytes);

			memset(cwd, 0, sizeof(cwd));

		} else {
			
			msgBytes = strlen(nD)+1; 
			msgSend(*clientFd, nD, msgBytes);
		} 
			      
	} else if(strncasecmp(sBuffer, "ls", 2) == 0) {

		lsCommand(*clientFd);	 

	} else if(strncasecmp(sBuffer, "get", 3) == 0) {

		strtok(sBuffer, " \t");
		if( (filename = strtok(NULL, " \t")) != NULL) {
			sendFile(*clientFd, filename); 
		} else {
			msgSend(*clientFd, NOFILE, sizeof NOFILE);
		}

	} else if(strncasecmp(sBuffer, "put", 3) == 0) {
		getFile(*clientFd);

	} else {

		/*Unrecognized input */
		msgBytes = strlen(unrec)+1;
		msgSend(*clientFd, unrec, msgBytes);
	}

	return ;
}

char* getDirectoryPath(char* buffer) {

	char* dir;

	strtok(buffer, " ");
	dir = strtok(NULL, " ");

	return dir;
}

/*Function to handle the ls command from the client */
/*List files in a directory, return is void, parameter is the client socket file descriptor */
void lsCommand(int clientFd) {

	struct dirent *dir; //dirent structure holds information about the directory. 
	DIR *directory; //directory structure to hold information about the files. 
	char wd[PATH_SIZE]; //working directory path 
	char* fn = NULL; //File name
	int fLen = 0; //length of the file..?



	/*Open the directory */
	if(getcwd(wd, sizeof(wd)) != NULL){

		directory = opendir(wd);

	} else {

		/*If it fails then just send an error message to the client */
		msgSend(clientFd, nD, (strlen(nD)+1));
		return;

	}
	       
	fn = (char*)malloc(sizeof(char)*(strlen(list)+1));
	if(fn == NULL) {
		fprintf(stderr, "Error allocating heap memory.\n");
		exit(EXIT_FAILURE);
	}

	strncat(fn, list, strlen(list));

	/*Loop through the files in the opened directory */
	while((dir = readdir(directory)) != NULL) {

		fLen = strlen(fn);
		       		
		/*Expand the size of the array */ 
		fn = (char*)realloc(fn, (fLen+(dir->d_namlen)+sizeof(ENDLINE)+1));
		if(fn == NULL) {
			fprintf(stderr, "Error allocating space on the heap! Also, a memory leak has just occured.\n");
			exit(EXIT_FAILURE);
		}

		if(strncmp(dir->d_name, ".", 1) == 0) {
			strncat(fn, ".", 1);
		}

		strncat(fn, dir->d_name, dir->d_namlen); 
		strncat(fn, ENDLINE, sizeof(ENDLINE));

	}

	/*Send the final line and return */
	fn = realloc(fn, strlen(fn)+3);
	if(fn == NULL) {
		fprintf(stderr, "Error allocating heap memory, in addition, a memory leak has just occured.\n");
		exit(EXIT_FAILURE);
	}

	strncat(fn, ".\n", 2);

	msgSend(clientFd, fn, (strlen(fn)+1));

	free(fn);

	return;

}

/*Function to reset the working directory back to original when client leaves. */
void resetWorkingDirectory(char* origin) {

	if(chdir(origin) != 0) { 
		fprintf(stderr, "Could not return to original directory!\n");
		exit(EXIT_FAILURE);
	}

	return ;
}
       
void sendFile(int fd, char* filename) {

	int file; //File descriptor for the file to open.
	unsigned int fileSize; //Size of the file in bytes. 
	unsigned char* buffer[512]; //Buffer for writting. 
	int bytesWritten = 0; //Bytes written to the socket
	int bytesRead = 0; 

	memset(buffer, 0, sizeof(char)*512);

	file = open(filename, O_RDONLY); 
	if(file == -1) {
		msgSend(fd, NOFILE, sizeof NOFILE);
		return;
	}

	/*Get the size, in bytes, of the file */
	fileSize = lseek(file, 0, SEEK_END);
	lseek(file, 0, SEEK_SET);
	
	/*Write the file size to the socket */
	write(fd, (void*)(&fileSize), sizeof(unsigned int));

	/*Write the filename to the socket */
	write(fd, (void*)filename, (strlen(filename)+1));
	
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

	 //Protocal. If filesize is 0, then the file could not be opened on the client end. 
	if(fileSize == 0) {
		printf("File not found~!\n\n");
		return;
	}

	/*Get the File name*/
	read(fd, (void*)fileName, BLOCKSIZE);

	printf("Beginning file drop....\nTransfering file: %s\n204 Size of file (in Bytes): %d\n\n", fileName, fileSize);

	newFile = open(fileName, O_WRONLY | O_CREAT, S_IRWXU);
	if(newFile < 0) {
		printf("Could not open file!\n");
		return; 
	}

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
