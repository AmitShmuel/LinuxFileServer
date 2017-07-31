all: client server message
	gcc -g client.o message.o -o client #complie client object file into exec file
	gcc -g server.o message.o -o server  #complie server object file into exec file	

client: client.c message.h
	gcc -g -c client.c -o client.o #complie client into object file

server: server.c message.h
	gcc -g -c server.c -o server.o #complie server into object file

message: message.c
	gcc -g -c message.c -o message.o #compile message into object file

clean:
	rm -f client server message locker *.o #remove all compilation products
