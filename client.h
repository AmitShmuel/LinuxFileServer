/***
   client.h
   author: Amit Shmuel - 305213621
   
   Clients can connect to the server and perform LS, LOAD & STORE
   operations regarding the "files" folder that the server maintains
   
   LS - view list of file in the folder
   <operation> <filename> - load or store a file
   <operation> /encode <filename> - load or store a file while encoded
   <operation> /compress <filename> - load or store a file while compressed
*/

#ifndef CLIENT_H
#define CLIENT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include "message.h"

#define MAX_BUFF 1024
#define PORT "9034"
#define LOCAL_IP "127.0.0.1"


// handle recieve error
void recv_error(int sockfd, fd_set* master, size_t bytes);

// write data to a file (file name in msg)
void write_file(Message* msg);

// build messages from a given buffer

// LS
void build_ls_msg(Message* m, char* buf);

// LOAD
result build_ld_msg(Message* m, char* buf);

// STORE
result build_st_msg(Message* m, char* buf);




#endif // CLIENT_H
