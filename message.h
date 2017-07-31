/***
   message.h file
   author: Amit Shmuel - 305213621
   
   the struct Message represents a message delivered between client and server
   it comprised of operation, file name, file data and their lengths
*/

#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/wait.h>

#define sz_t sizeof(size_t)

typedef enum {fail , success} result;

typedef struct {

	size_t sz_op;
	size_t sz_fn;
	size_t sz_dt;
	char* operation;
	char* filename;
	char* data;
	
} Message;


// checks if file exists
int file_exist(char* filename);

// return size of file
size_t size_of_file(char* fname);

// return sizeof serialized buffer
size_t size_of_msg(Message* m);

// clear allocated memory
void clear_message(Message* msg);

// initialize pointers with null
void init_message(Message* msg);

// serialize the message to buffer
result to_buffer(Message* m, char* buf);

// serialize the buffer to massage
result to_message(Message* m, char* buf);

// preparing the buffer before send()
char* prepare_to_send(Message* msg, char* buff);

// print the message
result print_msg(Message* msg);

// starts a new proccess and encode/compress file's data
result start_new_process(Message* m, char* filename, int op_len, int encode_op_len);



#endif // MESSAGE_H
