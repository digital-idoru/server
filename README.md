server
======

Server/Client Project for CSI416

A simple Server and Client applications for a Computer Networking class I'm currently taking. 
The Server will accept connections from the client and then will be able to execute some simple directory commands. 

There are some issues though:

1) How should I handle getting the directory paths from the client? I'm thinking about using regex.h instead of just strtok. I'm not even sure how I would use strtok since I sanatize the user input by just terminating the with the \0 character. 

2) I need to modularize this thing a bit more. It doesn't seem to be split up enough into functions that do their things. 