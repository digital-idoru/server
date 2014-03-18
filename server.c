/************************************
Daniel Hono II
CSI416 Project 2
A Simple Server
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


#ifndef DEFINES
#define DEFINES

#define true 1
#define false 0

#define PORT_PARAM 1
#define BACKLOG 10
#define BUFSIZE 250 
#define PORT 40000
#define PORT_STRING_LEN 6
#define PATH_SIZE 256
#define FILENAME_SIZE 1024 

#define ENDLINE "\n\0"

#endif

/*Define a boolean type */
typedef uint8_t bool;  

/*Note uint8_t is the same as a byte */

/*Function prototypes */
void setAddressInfoStandard(const char*, struct addrinfo, struct addrinfo**); //set address info with some standard params.
int createSocket(struct  addrinfo *); //Function to return the fd of a socket
void msgSend(int, char*, int); //Function to send a text based message to the client. 
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

/*Response messages to client commands. */
char* welcome_msg = "\n\n~La Vie Est Drole~\n\n";
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

	if(memchr((void*)msg, 0xd, (size_t)msgLength) != NULL) {
		return true; 
	} else if (memchr((void*)msg, 0, (size_t)msgLength) != NULL) {
		return true;
	} else {
		return false;
	}
}

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

//Function to send a message to a connected client. 
//Parameters include the client socket, the message to be sent, and the flag. 
void msgSend(int cS, char* message, int flag_param) {

	int len = strlen(message);

	if((send(cS, message, len, flag_param)) < 0) {
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
	if((listen(socketFd, BACKLOG)) < 0) {
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

	int len; 			        //Length msg to send. 
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

		}while(checkFullMsg(buffer, recSize) != true);

		//Need to remove the special characters appended to the string by the telnet. 
		sBuffer = sanatize(buffer); 

		if(ack == true) {
			
			/* Process commands only after the hello */
			commands(sBuffer, &clientFd, &ack);

		} else if(checkHelo(sBuffer) == true) {
			len = strlen(helo);		
			msgSend(clientFd, helo, 0);
			ack = true;

		} else if(ack != true){
			len = strlen(nHelo);
			send(clientFd, nHelo, len, 0);
		}

	} //End While

	return;
}

void commands(char* sBuffer, int* clientFd, bool *ack) {

  
	char cwd[PATH_SIZE]; 

	if(strcasecmp(sBuffer, "quit") == 0) { 

		/*Close the connection */
		msgSend(*clientFd, goodbye, 0); 

		close(*clientFd); 
		*clientFd = 0; 
		*ack  = false; 


		return;

	} else if(strncasecmp(sBuffer, "helo", 4) == 0) {
		msgSend(*clientFd, rHELO, 0);
	} else if(strncasecmp(sBuffer, "cd", 2) == 0) {

		if(chdir(getDirectoryPath(sBuffer)) == 0) {
			msgSend(*clientFd, dirCh, 0);
		} else {
			msgSend(*clientFd, dirChF, 0);
		}	       
	} else if(strncasecmp(sBuffer, "pwd", 3) == 0) {
		
		if(getcwd(cwd, sizeof(cwd)) != NULL) {
			msgSend(*clientFd, "203 ", 0);
			strncat(cwd, ENDLINE, sizeof(ENDLINE));
			msgSend(*clientFd, cwd, 0);
			memset(cwd, 0, sizeof(cwd));
		} else {
			msgSend(*clientFd, nD, 0);
		} 
			      
	} else if(strncasecmp(sBuffer, "ls", 2) == 0) {
		lsCommand(*clientFd);	 
	} else if(strncasecmp(sBuffer, "get", 3) == 0) {
		/*code for get command */
	} else if(strncasecmp(sBuffer, "put", 3) == 0) {
		/*code for put command */
	} else {
		/*Unrecognized input */
		msgSend(*clientFd, unrec, 0);
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
	DIR *directory; //directory(?)
	char wd[PATH_SIZE]; //working directory path 
	char fn[FILENAME_SIZE]; //filename buffer 

	/*Open the directory */
	if(getcwd(wd, sizeof(wd)) != NULL){
		directory = opendir(wd);
	} else {
		/*If it fails then just send an error message to the client */
		msgSend(clientFd, nD, 0);
		return;
	}
	
	msgSend(clientFd, list, 0);

	/*Loop through the files in the opened directory */
	while((dir = readdir(directory)) != NULL) {

		if(strncmp(dir->d_name, ".", 1) == 0)
			strncat(fn, ".", 1);

		strncat(fn, dir->d_name, sizeof(dir->d_name)); 
		strncat(fn, ENDLINE, sizeof(ENDLINE));
		msgSend(clientFd, fn, 0);

		memset(fn, 0, sizeof(fn)); /*Reset filename buffer */
	}

	/*Send the final line and return */
	msgSend(clientFd, ".\n", 0);
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
       
