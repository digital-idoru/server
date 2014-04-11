#Daniel S. Hono II 
#CSI416 - Client/Serve Project II Makefile 

#MACROS
CC=gcc
CFLAGS = -Wall -g
FILESS = server.o
FILESC = client.o
EXECS = server
EXECC = client
CLEAN= *.o *.c~ core makefile~ *.md~


.SUFFIXES: .c .o

.c.o:
	$(CC) $(CFLAGS) -c $< 

#Default target
all: server client

#Server
server: $(FILESS)
	gcc $(FILESS) -o $(EXECS) 

#client 
client: $(FILESC)
	gcc $(FILESC) -o $(EXECC)

#Remove unwanted files. 
clean: 
	rm -f $(CLEAN)
