#include "client.h"


int main(int argc, char* argv[])
{
	char buf[MAX_BUFF], size_of_msg_buf[MAX_BUFF], *buffed_msg; 
	int load_flag = 0;
	int rv, numbytes, sockfd;
	unsigned long int size;
	Message msg; // message to server 
	init_message(&msg); // null to pointers
	
    	struct addrinfo hints, *servinfo, *p;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC; // use AF_INET6 to force IPv6
	hints.ai_socktype = SOCK_STREAM;
	
	if(argc == 1) { 
		printf("connecting to local adress\n");
		if ((rv = getaddrinfo(LOCAL_IP, PORT, &hints, &servinfo)) != 0) {
			fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
			exit(1);
		}
	}
	else if(argc == 2) {
		printf("connecting to: %s\n", argv[1]);
		if ((rv = getaddrinfo(argv[1], PORT, &hints, &servinfo)) != 0) {
			fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
			exit(1);
		}
	}
	else {
		puts("too many arguments");
		exit(1);
	}
	printf("connected\n");
	// loop through all the results and connect to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			perror("socket");
			continue;
		}
		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			perror("connect");
			close(sockfd);
			continue;
		}
		break; // if we get here, we must have connected successfully
	}
	freeaddrinfo(servinfo);	
	
	fd_set master, fds;
	FD_ZERO(&master);
	FD_SET(0, &master);
	FD_SET(sockfd, &master);	
	
    	for (;;) {
    	
		fds = master;
		if (select(sockfd + 1, &fds, NULL, NULL, NULL) == -1) {
			perror("select");
			exit(4);
		}
		if (FD_ISSET(0, &fds)) {
			fgets(buf, MAX_BUFF, stdin); buf[strlen(buf) -1] = '\0';
			if( !strcmp(buf, "LS") || (load_flag = !strncmp(buf, "LOAD ", 5)) || !strncmp(buf, "STORE ", 6) ) { 
				
				if(!strncmp(buf, "STORE ", 6)) {
					if(!build_st_msg(&msg, buf))
						continue;
				}
				else if(load_flag) {
					if(!build_ld_msg(&msg, buf)) {
						load_flag = 0; 
						continue;
					}
				}				
				else 
					build_ls_msg(&msg, buf);				
				
				if((buffed_msg = prepare_to_send(&msg, buffed_msg)) != NULL) {
					
					if(send(sockfd, buffed_msg, size_of_msg(&msg)+sz_t, 0) == -1)
						perror("send");
					free(buffed_msg);	
				}	
				clear_message(&msg);							
			}
			else
				fprintf(stderr, "unknown operation, use LS, LOAD or STORE\n");
		}
		if (FD_ISSET(sockfd, &fds)) {
			
			if(load_flag) {	
				
				// getting message's size
                    	if ((numbytes = recv(sockfd, size_of_msg_buf, sz_t, 0)) <= 0) {                 			                 
                    		recv_error(sockfd, &master, numbytes); 
                    		continue;
                    	}                                 
                    	if(strncmp(size_of_msg_buf, "error", 5)) { // NO ERROR (strcmp != 0)
                    		     		                		
                    		memcpy(&numbytes, size_of_msg_buf, sz_t);                        			                			
                       		buffed_msg = (char*)malloc(numbytes);
                    		// getting message
                    		if ((numbytes = recv(sockfd, buffed_msg, numbytes, MSG_WAITALL)) <= 0) {                      		
                        		recv_error(sockfd, &master, numbytes); 
                        		continue;
                       		 }                      
                     	                 
                        	if(!to_message(&msg, buffed_msg)) 
                            		perror("error loading file");
                        	else    
                        		write_file(&msg); 
                        	 
                        	clear_message(&msg);
					free(buffed_msg);	
				}
				else  // ERROR (strcmp == 0)
					puts("error, either an error occured or the requested file doesn't exist");					
                    	load_flag = 0;
			}
			
			else {
				if((numbytes = recv(sockfd, buf, MAX_BUFF, 0)) <= 0) 
					recv_error(sockfd, &master, numbytes);				
				buf[numbytes] = '\0';
				puts(buf);
			}
		}
	}
    	return 0;
}



void recv_error(int sockfd, fd_set* master, size_t bytes) {                  			
  	// got error or connection to server on lost
      if (bytes == 0) {
		printf("connection to server on socket %d was lost\n", sockfd);
		exit(1);
	} 
	else {
		perror("recv");
	}
	close(sockfd);
	FD_CLR(sockfd, master);
}


void write_file(Message* msg) {
	
	char filename[32];
	
	strncpy(filename, msg->filename, msg->sz_fn);
	filename[msg->sz_fn] = '\0'; 
	
	if(file_exist(filename)) {
		printf("the file %s already exists\n", filename); return; }
     	
     	int fd = open(filename, O_RDWR | O_CREAT | O_TRUNC, (mode_t)0600);
      if( fd == -1) {
      	perror("open"); return; }
     	
     	if(msg->sz_op == 12 || msg->sz_op == 14) { // LOAD /encode or LOAD /compress
     	
     		if(!fork()) {
     			
     			if(write(fd, msg->data, msg->sz_dt) == -1) { // fwrite creates bugs when trying to LOAD /encode or LOAD /compress..
      			perror("open"); return; 
      		}
      		
     			if(msg->sz_op == 12) {
     				if(execlp("uudecode", "uudecode", filename, NULL) == -1) {
					perror(""); return;
				}
			}
     			else
     				if(execlp("gunzip", "gunzip", filename, NULL) == -1) {
					perror(""); return;
				}
     		}
     		else {
     			
     			wait(NULL);
    			if(msg->sz_op == 12) {
    				remove(filename);
    				rename("to_encode", filename);	
    			}
    			puts("file was successfully loaded");
     		}
     	}
     	else {
     	
     		if(write(fd, msg->data, msg->sz_dt) == -1) { // fwrite creates bugs when trying to LOAD /encode or LOAD /compress..
      		perror("open"); return; 
      	}
     		puts("file was successfully loaded");
     	}
}


void build_ls_msg(Message* m, char* buf) {

	
	int op_len = 2; // "LS" = 2 chars
	
	m->sz_op = op_len;
	
	m->operation = (char*)malloc(op_len);
	memcpy(m->operation, buf, op_len);
	
	m->sz_fn = m->sz_dt = 0;
	
	m->filename = m->data = NULL;
}


result build_ld_msg(Message* m, char* buf) {
	
	int op_len;
	if(!strncmp(buf, "LOAD /", 6)) {
		if(!strncmp(buf, "LOAD /encode ", 13)) 
			op_len = 12;
	
		else if(!strncmp(buf, "LOAD /compress ", 15))
			op_len = 14;
		else {
			fprintf(stderr, "unknown operation, use LS, LOAD or STORE\n");
			return fail;
		}
	}
	else 
		op_len = 4; 
	
	m->sz_op = op_len;
	
	if(file_exist(buf+op_len+1)) {
		printf("the file %s already exists\n", buf+op_len+1); 
		return fail;
	}
	
	m->operation = (char*)malloc(op_len);
	memcpy(m->operation, buf, op_len);
	m->sz_fn = strlen(buf)-op_len-1;
	m->filename = (char*)malloc(m->sz_fn);
	memcpy(m->filename, buf+op_len+1, m->sz_fn);
	m->sz_dt = 0;
	m->data = NULL;
	
	return success;
}


result build_st_msg(Message* m, char* buf) { 
	
	char filename[128];
	int op_len;
	
	if(!strncmp(buf, "STORE /", 7)) {
		if(!strncmp(buf, "STORE /encode ", 14)) 
			op_len = 13;
	
		else if(!strncmp(buf, "STORE /compress ", 16))
			op_len = 15;
		else {
			fprintf(stderr, "unknown operation, use LS, LOAD or STORE\n");
			return fail;
		}
	}
	else 
		op_len = 5; 
	
	strcpy(filename, buf+op_len+1);
	
	if(!file_exist(filename)) {
		printf("the file %s doesn't exist\n", filename); return fail; }

	if(op_len == 13 || op_len ==15) {
		
		if(start_new_process(m, filename, op_len, 13) == fail)
			return fail;
	}
	else { // simple STORE
		
		FILE* fp = fopen(filename, "r");
		if(!fp) {
			perror("fopen"); return fail;
		}
		m->sz_dt = size_of_file(filename);
		m->data = (char*)malloc(m->sz_dt);
		fread(m->data, 1, m->sz_dt, fp);
		fclose(fp);
	}
		
	m->sz_op = op_len;
	m->operation = (char*)malloc(op_len);
	memcpy(m->operation, buf, op_len);
	
	if(op_len == 15) 
		strcat(filename, ".gz");
		
	m->sz_fn = strlen(filename);
	m->filename = (char*)malloc(m->sz_fn);
	memcpy(m->filename, filename, m->sz_fn);
	
	return success;
}
