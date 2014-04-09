#Very general purpose makefile that I created for projects. 
#Compiles and links using the flags defined in CFLAGS 
#For different projects on only has to edit the FILES and EXEC variables 
#You'll have to add the other dependencies yourself too, if you so need to. 

#MACROS
CC=gcc
CFLAGS= -Wall -g
FILESS = server.o
FILESC = client.o
EXECS = server
EXECC = client
CLEAN= *.o *.c~ core makefile~

#Tells make what filetypes to expect 
.SUFFIXES: .c .o

#No dependencies. Tells make how to get a .o file from a .c file. 
# $< means that if the target is, for example, main.o, then look for main.c 
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
	rm -i $(CLEAN)
