/************************************
Daniel Hono II
CSI416 Project 2
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


#ifndef DEFINES
#define DEFINES

#define true 1
#define false 0

#define PORT_PARAM 1
#define BACKLOG 10
#define BUFSIZE 250 
#define PORT 40000
#define PORT_STRING_LEN 6

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


/*Response messages to client commands. */
const char* welcome_msg = "\n\n~WELCOME TO THE DESERT OF THE REAL~\n\n";
const char* helo = "220 Connection Established.\n\n";
const char* goodbye = "100 Don't go away mad, just go away.\n";
const char* unrec = "401 Unrecognized Input.\n"; 
const char* nHelo = "550 You must give the HELO command.\n";
const char* rHELO = "450 HELO already issued\n"; 

int main(void) {

  char nodeName[PORT_STRING_LEN];       //String to hold the port number. 
  struct addrinfo hints; 		//Hints to send to getAddressInfo
  struct  addrinfo *servinfo; 		//Pointer to the linked list of results. 

  //We need to make sure that the hints struct is empty to start with. 
  memset(&hints, 0, sizeof(hints)); 

  //Get the address information set up. 
  sprintf(nodeName,"%d", PORT);
  setAddressInfoStandard(nodeName, hints, &servinfo);

  //Handle the server 
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

  /*Set up the msg buffer */
  char buffer[BUFSIZE]; 
  char* sBuffer;
  memset(buffer, 0, strlen(buffer)); 

  rBuff = BUFSIZE;

  /*Hello Flag starts off as false */
  ack = false;

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

    if(ack == true) { //Only process commands after HELO has been recieved. 

      if(strcasecmp(sBuffer, "QUIT") == 0) { 
	/*Close the connection */
	msgSend(clientFd, goodbye, 0); 

	close(clientFd); //Close the client socket
	clientFd = 0; //Reset the client file descriptor to wait for the next connection. 
	ack  = false; //Reset HELO flag. 

      } else if(strcasecmp(sBuffer, "HELO") == 0) {
	msgSend(clientFd, rHELO, 0);

      } else {
	/*Unrecognized input */
	msgSend(clientFd, unrec, 0);
      }

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
