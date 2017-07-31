/***
   server.h file
   author: Amit Shmuel - 305213621
   
   This server maintains a folder named "files"
   multiple clients can connect to the server
   and load / store files to the folder, also ask for list of the files..
   There are also special operations which allows compression and encoding while transfer is made.
*/

#ifndef SERVER_H
#define SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <dirent.h>
#include "message.h"

#define PORT "9034"   // port we're listening on
#define MAX_BUFF 1024

char temp_file[] = "lockerXXXXXX";
struct flock fl = {F_WRLCK, SEEK_SET, 0, 0, 0};
int lock_fd;


// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa);

// dealing with recv error
void recv_error(int fd, fd_set* master, size_t bytes);

// lock a file
result lock_file(int fd);

// unlock a file
result unlock_file(int fd);

// mapping message's data to file
char* map_to_file(Message* msg, char* path);

// return list of server's files
char* listing();

// load a file
result load(Message* msg);

// store a file
char* store(Message* msg);




#endif // SERVER_H
