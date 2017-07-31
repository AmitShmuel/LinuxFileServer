Made by - Amit Shmuel - 305213621

-----------------------------------------------------------------
PLEASE FOLLOW THE INSTRUCTIONS BELLOW BEFORE TRYING THIS PROGRAM
-----------------------------------------------------------------

------------------------------------------------------------------------------------------------------------------------------------------
# on your first launch, the server should open a folder named "files", which will contain all stored files.

# each launch creates a temporary file used for locking. 
  as the server only dies on force close, the temporary files won't be deleted. (there's an appropriate code in the destructor though..)

# in order to store and file, please make sure the desired file is in the program's directory, i.e where "files" folder lays

# loaded files will be loaded into the program's directory

# valid commands: (invalid commands won't crush the program but an error message will be printed)

	- LS
	- STORE <filename>		- LOAD <filename>
	- STORE /encode <filename>	- LOAD /encode <filename>
	- STORE /compress <filename> 	- LOAD /compress <filename>

# resources: 
	
	writing to file using mmap - https://gist.github.com/sanmarcos/991042 
------------------------------------------------------------------------------------------------------------------------------------------

Enjoy!
