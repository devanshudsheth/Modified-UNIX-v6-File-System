Objective

Designing a UNIX v6 file system which will allow a user access to the file system of a foreign operating system, the modified UNIX v6.

My fsaccess program will read a series of commands from the user an execute them. It supports the following commands:

1. initfs - Initializes the file system. 
	Args - name of file system, number of blocks, number of inodes
2. cpin - Create a new file called v6-file in the current directory and fill in the contents of newly created file with contents of enternal file
	Args - externalfile, v6-file
3. cpout - If v6-file exists, create externalfile and make the externalfile's contents equal to v6-file.
	Args - v6-file, externalfile
4. mkdir v6-dir - If v6-dir does not exist, create a directory.
	Args - v6-dir
5. rm v6-file - If v6-file exists, delete the file.
	Args - v6-file
6. q - Save all changes and quit

METHODS OF EXECUTION:

This program was implemented on the putty terminal on the csgrads1@utdallas.edu server.
Upon login, enter vi fsaccess.c
Once a new file has been created, copy the contents over from our source code.
Then press Esc and Shift+Z+Z to save.
Then enter cc fsaccess.c
Then enter ./a.out

Upon entry into program enter commands of the following type

1. initfs v6filesystem 5000 400
2. mkdir v6-dir
3. cpin externalfile v6-file
4. cpout v6-file externalfile
5. q
