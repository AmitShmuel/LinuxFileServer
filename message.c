#include "message.h"


int file_exist(char* filename) {

	struct stat st;
	return !stat(filename, &st);
}


size_t size_of_file(char* fname) {
	
	struct stat st;
	stat(fname, &st);
	return st.st_size;
}


unsigned long int size_of_msg(Message* m) {

	return sz_t*3 + m->sz_op + m->sz_fn + m->sz_dt;
}


void init_message(Message* msg) {
	
	msg->operation = NULL;
	msg->filename = NULL;
	msg->data = NULL;
}


void clear_message(Message* msg) {

	if(msg->operation != NULL) {
		free(msg->operation);
		msg->operation = NULL;
	}
	if(msg->filename != NULL) {	
		free(msg->filename);
		msg->filename = NULL;
	}
	if(msg->data != NULL) {
		free(msg->data);
		msg->data = NULL;
	}
}


result to_buffer(Message* m, char* buf) {

	if(!m || !buf) {
		perror("to_buffer");
		return fail;
	}
	
	memcpy(buf, &m->sz_op, sz_t);
	buf += sz_t;
	
	memcpy(buf, &m->sz_fn, sz_t);
	buf += sz_t;
	
	memcpy(buf, &m->sz_dt, sz_t);
	buf += sz_t;
	
	memcpy(buf, m->operation, m->sz_op);
	buf += m->sz_op;
	
	memcpy(buf, m->filename, m->sz_fn);
	buf += m->sz_fn;
	
	memcpy(buf, m->data, m->sz_dt);	
	
	return success; 
}


result to_message(Message* m, char* buf) {

	if(!m || !buf) {
		perror("to_message");
		return fail;
	}
	
	memcpy(&m->sz_op, buf, sz_t);
	buf += sz_t;
	
	memcpy(&m->sz_fn, buf, sz_t);
	buf += sz_t;
	
	memcpy(&m->sz_dt, buf, sz_t);
	buf += sz_t;
	
	m->operation = (char*)malloc(m->sz_op);
	memcpy(m->operation, buf, m->sz_op);
	buf += m->sz_op;
	
	m->filename = (char*)malloc(m->sz_fn);
	memcpy(m->filename, buf, m->sz_fn);
	buf += m->sz_fn;
	
	m->data = (char*)malloc(m->sz_dt);
	memcpy(m->data, buf, m->sz_dt);	
	
	return success;
}


char* prepare_to_send(Message* msg, char* buffed_msg) {
				
	size_t size = size_of_msg(msg);		// size gets size of message
	buffed_msg = (char*)malloc(size+sz_t);	// allocate 'size'+'sz_t' bytes to buffed_msg
	memcpy(buffed_msg, &size, sz_t);		// copying to buffed_msg the variable 'size'
	if(!to_buffer(msg, buffed_msg+sz_t))	// serializing the message at location +sz_t cuz he has the size in the start
		return NULL;			 
	return buffed_msg;					
}


result print_msg(Message* msg) {

	printf("printing file:\n"
		 "operation size: %lu\n"
		 "file name size: %lu\n"
		 "file data size: %lu\n"
		 "operation: %s\n"
		 "file name: %s\n"
		 "file data: %s\n"
		 "size of msg: %lu\n", msg->sz_op, msg->sz_fn, msg->sz_dt, msg->operation, msg->filename, msg->data, size_of_msg(msg));
}


result start_new_process(Message* m, char* filename, int op_len, int encode_op_len) {
	
	int pfds[2];
	if(pipe(pfds) == -1) {
		perror("pipe"); return fail;
	}
		
	if(!fork()) { // child
			
		close(pfds[0]);
		dup2(pfds[1], 1);
			
		if(op_len == encode_op_len) {
			if(execlp("uuencode", "uuencode", filename, "to_encode", NULL) == -1) {
				perror("execlp"); return fail;
			}
		}
		else
			if(execlp("gzip", "gzip", "-c", "--stdout", filename, NULL) == -1) {
				perror("execlp"); return fail;
			}
	}
	else { // parent
			
		char *temp = NULL, *realloced = NULL;
		int readcount = 0;
		close(pfds[1]);
		m->sz_dt = 0;
		m->data = (char*)malloc(1024);
		temp = m->data;
		while((readcount = read(pfds[0], temp, 1024)) > 0) {

			m->sz_dt += readcount;
				
			if(realloced = (char*)realloc(m->data, m->sz_dt*2)) // DONT ASK ME WHY *2 o_O, thats the only thing that works..
				m->data = realloced;
		//	else
		// 		realloc failed, we didn't lose our data
				
			temp = m->data+m->sz_dt;		
		}
		wait(NULL);	
		return success;
	}
}
