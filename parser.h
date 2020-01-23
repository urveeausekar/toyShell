#include <stdio.h>
#include <stdlib.h>

typedef struct oneCommand{
	char *name; //the name of the command or file. Find using whereis or check in current directory
	char **args; //an array of strings holding the arguments to the command;
	char *inputfrom; //if exists, then take input from here (input redirection) . 
	char *outputto; // if exists, then give output to this file or command (either output redirection or pipe)
	int pipe; //if yes, then the result is to be output to outputto via a pipe
	int stder; //if 1, then redirect stderr
	int numofargs; //the number of args to this particular command
	int append; //if 1, then append, don't overwrite
}onecmd;

// above structure represents one command like cat, ls etc.
//in parser, init will limit the arguments of the one command to 8
//the entire command input into shell by user is is limited to 16 individual structures
//any char pointer except for a char array that holds args is 32 char long. The arg char array is 16 chars long


typedef onecmd** totalCmd; //array of pointers to onecmd structures. This array contains the entire information of the entire input

typedef struct retval{
	int background; //if 1, then run in background
	int num; //number of oneCommand structures actually used;
}retval;

int initP(totalCmd array);
retval ParseString(totalCmd array, char *input);
void print(totalCmd array, int arrin);
