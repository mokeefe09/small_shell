#ifndef __COMMAND_INFO_H__
#define __COMMAND_INFO_H__

struct command_info{
	char* args[512]; // Array of char pointers. Will hold pointers to args.

	char* stdin_file; // If no change to stdin, this is NULL. If change it 
	                  // holds pointer to path of file.

	char* stdout_file; // If no change to stdout, this is NULL. If change it
	                   // holds pointer to path of file.
	
	int background; // If to be run in fg, this is 0. If bg, this is 1. 
};

#endif
