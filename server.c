#include "server.h"


__attribute__((constructor))
static void initialize_locking() {
	
	lock_fd = mkstemp(temp_file);
}

__attribute__((destructor))
static void destroy_locking() {

	close(lock_fd);
	remove(temp_file);
}


int main(void) {
	
	// create "files" folder if doesn't exist
	if(!file_exist("files")) 
		mkdir("files", 0700);
	
	char buf[sz_t], *buffed_msg = NULL, *return_msg = NULL;
	size_t size, sz_msg, nbytes, file_size;
	char remoteIP[INET6_ADDRSTRLEN];    
    	int fdmax,  newfd, listener, i, rv, yes = 1, load_flag = 0;      
    	Message msg; // message to client 
	init_message(&msg); // nulls to pointers
	
    	struct addrinfo hints, *ai, *p;
    	struct sockaddr_storage remoteaddr; // client address
    	socklen_t addrlen;
	// get us a socket and bind it
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	if ((rv = getaddrinfo(NULL, PORT, &hints, &ai)) != 0) {
		fprintf(stderr, "selectserver: %s\n", gai_strerror(rv));
		exit(1);
	}
	
	for(p = ai; p != NULL; p = p->ai_next) {
    		listener = socket(p->ai_family, p->ai_socktype, p->ai_protocol);	
		if (listener < 0) 
			continue;
		// lose the pesky "address already in use" error message
		if(setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
			perror("setsockopt");
			exit(1);
		}
		if (bind(listener, p->ai_addr, p->ai_addrlen) < 0) {
			close(listener);
			continue;
		}
		break;
	}
	
	if (p == NULL) { // we didn't get bound
		fprintf(stderr, "selectserver: failed to bind\n");
		exit(2);
	}
	freeaddrinfo(ai); // all done with this

    	// listen
    	if (listen(listener, 10) == -1) {
        	perror("listen");
        	exit(3);
    	}

	fd_set master, read_fds;
    	FD_ZERO(&master);    
    	FD_ZERO(&read_fds);
    	FD_SET(listener, &master);
    	fdmax = listener; 
	printf("waiting for connections..\n");
	
    	for(;;) {
        	read_fds = master; // copy it
        	if (select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1) {
            	perror("select");
            	exit(4);
        	}
        	// run through the existing connections looking for data to read
        	for(i = 0; i <= fdmax; i++) {
            	if (FD_ISSET(i, &read_fds)) { // we got one!!
                		if (i == listener) {
                  		// handle new connections
                  		addrlen = sizeof remoteaddr;
					newfd = accept(listener, (struct sockaddr *)&remoteaddr, &addrlen);
					if (newfd == -1) {
                        		perror("accept");
                        	}
                    		else {
                        		FD_SET(newfd, &master); // add to master set
                        		if (newfd > fdmax) {     // keep track of the max
                            			fdmax = newfd;
                            		}
                        		printf("new connection from %s on socket %d\n",inet_ntop(remoteaddr.ss_family,
                        			 								   get_in_addr((struct sockaddr*)&remoteaddr),
                        			 							         remoteIP,INET6_ADDRSTRLEN)
                        			 						    ,newfd);
                    		}
                		} 
                		else { // handle data from a client 
                			
                    		// getting message's size
                    		if ((nbytes = recv(i, buf, sz_t, 0)) <= 0) {            			                 
                    			recv_error(i, &master, nbytes); // got error or connection closed by client
                    		      continue;
                    		}  		
                    		memcpy(&nbytes, buf, sz_t);                        			                			
                       		buffed_msg = (char*)malloc(nbytes);   
                       		                    			       
                    		// getting message
                    		if ((nbytes = recv(i, buffed_msg, nbytes, MSG_WAITALL)) <= 0) {                       		
                        		recv_error(i, &master, nbytes); // got error or connection closed by client
                    			continue;
                    		}
                            				                            
                            	if(!to_message(&msg, buffed_msg)) 
                            		continue;                  			                         	
                           		
                              if(msg.sz_op == 2) 				  	// LS
                              	return_msg = listing();	  
                              	
                              else if(!strncmp(msg.operation, "STORE", 5)) 	// STORE                      
                                    return_msg = store(&msg);
                                    				
                              else if(!strncmp(msg.operation, "LOAD", 4)) { 	// LOAD
                                   
                                    free(buffed_msg); buffed_msg = NULL;
                                    if(!load(&msg))
                                    	return_msg = "error";
                                    else{
	                                    buffed_msg = prepare_to_send(&msg, buffed_msg);
                                    	load_flag = 1;                                     
                                    }
                            	}            		
                              else
                                    return_msg = "error occured, please try again";	 
                                    
                              if(load_flag) {
                                    if(send(i, buffed_msg, size_of_msg(&msg)+sz_t, 0) == -1)
                                    	perror("send");
                                    load_flag = 0;
                                    munmap(msg.data, msg.sz_dt); msg.data = NULL;
                              }                                                     		
                              else                                          		
                                    if(send(i, return_msg, strlen(return_msg), 0) == -1)
                                    	perror("send");
                              
                              if(buffed_msg)                                          	
                              	free(buffed_msg);                              
                              clear_message(&msg);     
                                                                                         
                        } // END handle data from client
         	      } // END got new incoming connection
         	} // END looping through file descriptors
     	} // END for(;;)
    	return 0;
}


void *get_in_addr(struct sockaddr *sa) {

	if (sa->sa_family == AF_INET) 
		return &(((struct sockaddr_in*)sa)->sin_addr);

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}


char* listing() {
	
	static char list[MAX_BUFF];
	memset(list, 0, MAX_BUFF);
	DIR *pDir;
	struct dirent *pDirent;
	
	for(pDir = opendir("files"); pDirent = readdir(pDir); ) {
		if(pDirent->d_name[0] != '.') {
			strcat(list, pDirent->d_name);
			strcat(list, "  ");
		}
	}
	closedir(pDir);
	return list;
}


result load(Message* m) {
	
	char filename[128];
	
	strncpy(filename, m->filename, m->sz_fn); filename[m->sz_fn] = '\0';
	chdir("files");
	
	if(!file_exist(filename)) {
		chdir("..");
		return fail;
	}
	
	if(m->sz_op == 14 || m->sz_op == 12) { 
		
		lock_file(lock_fd); // ensuring one process
		
		if(start_new_process(m, filename, m->sz_op, 12) == fail) {
			chdir("..");
			return fail;
		}
		
		unlock_file(lock_fd);
	}
	else { // simple LOAD
	
		int fd = open(filename, O_RDWR);
		if(fd == -1) {
			perror("open"); chdir(".."); return fail; 
		}
		
		m->sz_dt = size_of_file(filename);
		m->data = (char*)malloc(m->sz_dt);
		
		lock_file(fd);
	
		if((m->data = mmap(NULL, m->sz_dt, PROT_READ, MAP_PRIVATE, fd, 0)) == NULL) {
			perror("mmap"); chdir(".."); return fail;
		}
	
		unlock_file(fd);
		close(fd);
	}
	
	if(m->sz_op == 14) {
		free(m->filename);
		m->sz_fn += 3;
		strcat(filename, ".gz");
		m->filename = (char*)malloc(m->sz_fn);
		memcpy(m->filename, filename, m->sz_fn);
	}
	
	chdir("..");
	return success;
}


char* store(Message* msg) {
				 
	char filename[32], *return_msg = NULL;
	
	strncpy(filename, msg->filename, msg->sz_fn);
	filename[msg->sz_fn] = '\0';
	
	chdir("files");
	if(file_exist(filename)) {
		chdir(".."); return "file already exists";
	}
	
     	if(msg->sz_op == 13 || msg->sz_op == 15) { // STORE /encode or STORE /compress
     		
     		lock_file(lock_fd);
     		
     		if(!fork()) { // child
     			
			if((return_msg = map_to_file(msg, filename)) != NULL) { // there's a failure massage 
				perror(""); exit(1);
			}
     			
     			if(msg->sz_op == 13) {
     				if(execlp("uudecode", "uudecode", filename, NULL) == -1) {
					perror(""); exit(1);
				}
			} 
			else {
				if(execlp("gunzip", "gunzip", filename, NULL) == -1) {
					perror(""); exit(1);
				}
			}
     		}
     		else { // parent
     		
     			wait(NULL);
     			unlock_file(lock_fd);
    			if(msg->sz_op == 13) {
    				remove(filename);
    				rename("to_encode", filename);	
    			}
     			chdir("..");
     			return "file was saved successfully";
     		}
     	} 
     	else {
     		if((return_msg = map_to_file(msg, filename)) != NULL) { // there's a failure massage 
			chdir(".."); return return_msg;
		}
		chdir("..");
     		return "file was saved successfully";
     	}
}


char* map_to_file(Message* msg, char* path) { 
	
      int fd = open(path, O_RDWR | O_CREAT | O_TRUNC, (mode_t)0600);
      if(fd == -1) {
      	perror("open"); return "error storing file"; }
      
      if(lseek(fd, msg->sz_dt-1, SEEK_SET) == -1) {
      	close(fd); perror("lseek"); return "error storing file"; }
      
      if(write(fd, "", 1) == -1) {
      	close(fd); perror("write"); return "error storing file"; }

	char* map = mmap(NULL, msg->sz_dt, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if(map == MAP_FAILED) {
		close(fd); perror("mmap"); return "error storing file"; }
	
	memcpy(map, msg->data, msg->sz_dt);
	
	if(msync(map, msg->sz_dt, MS_SYNC) == -1) {
		perror("msync"); return "error storing file"; }
	
	if(munmap(map, msg->sz_dt) == -1) {
		close(fd); perror("munmap"); return "error storing file"; }
		
	close(fd);
	return NULL;
}


void recv_error(int fd, fd_set* master, size_t bytes) {                    			
  	// got error or connection closed by client
      if (bytes == 0) 
       	printf("socket %d hung up\n", fd);
      else 
     	      perror("recv"); 
      close(fd); // bye!
      FD_CLR(fd, master); // remove from master set
}


result lock_file(int fd) {
	
	fl.l_type = F_WRLCK;
	if(fcntl(fd, F_SETLKW, &fl) == -1) {
		perror("fcntl");
		return fail;
	}
	return success;
}

result unlock_file(int fd) {
	
	fl.l_type = F_UNLCK;
	if(fcntl(fd, F_SETLK, &fl) == -1) {
		perror("fcntl");
		return fail;
	}
	return success;
}
