#include "parser.h"
#include <string.h>
#include <errno.h>

int initP(totalCmd array){
	int i, j;
	if (array == NULL){
		fprintf(stdout, "Some error: Memory not allocated for array itself\n");
		return ENOMEM;
	}
	for(i = 0; i < MAX; i++){
		array[i] = (onecmd *)malloc(sizeof(onecmd));
		if(array[i] == NULL){
			fprintf(stdout, "Some error: Memory not allocated for structure, i = %d\n", i);
			return ENOMEM;
		}
		array[i]->name = (char *)malloc(sizeof(char) * MAXSTR);
		array[i]->inputfrom = (char *)malloc(sizeof(char) * MAXSTR);
		array[i]->outputto = (char *)malloc(sizeof(char) * MAXSTR);
		if(array[i]->name == NULL || array[i]->inputfrom == NULL || array[i]->outputto == NULL){
			fprintf(stdout, "Some error: Memory not allocated for name or io file name\n");
			return ENOMEM;
		}
		
		// a nul at index 0 signifies that that array is still unused
		array[i]->name[0] = '\0';
		array[i]->outputto[0] = '\0';
		array[i]->inputfrom[0] = '\0';
		
		array[i]->args = (char **)malloc(sizeof(char *) * MAXARGS);
		if(array[i]->args == NULL){
			fprintf(stdout, "Some error: Memory not allocated for some args array\n");
			return ENOMEM;
		}
		for(j = 0; j < MAXARGS; j++){
			array[i]->args[j] = (char *)malloc(sizeof(char) * MAXSTR);
			
			// a nul at index 0 signifies that that array is still unused
			if(array[i]->args[j] == NULL){
				fprintf(stdout, "Some error: Memory not allocated for a string in args\n");
				perror("malloc problems");
				return ENOMEM;
			}
			array[i]->args[j][0] = '\0';
		}
		array[i]->pipe = 0;
		array[i]->stder = 0;
		array[i]->numofargs = 0;
		array[i]->append = 0;
		
	}
	return 0;
}



void deallocate(totalCmd array, int arrin){
	int i,j;
	
	for(i = 0; i < arrin; i++){
		if(array[i]->name[0] == '\0'){
			free(array[i]->name);
			array[i]->name = NULL;
		}
		if(array[i]->inputfrom[0] == '\0'){
			free(array[i]->inputfrom);
			array[i]->inputfrom = NULL;
		}
		if(array[i]->outputto[0] == '\0'){
			free(array[i]->outputto);
			array[i]->outputto = NULL;
		}
		
		
		free(array[i]->args[array[i]->numofargs]);
		array[i]->args[array[i]->numofargs] = NULL;
		for(j = array[i]->numofargs + 1; j < MAXARGS; j++)
			free(array[i]->args[j]);
			
	}
	
}



enum state {START, PROGNAME, ARGS, INPUTFROM, OUTPUTTO, DONE, ERROR};
char *states[10] = {"START", "PROGNAME", "ARGS", "INPUTFROM", "OUTPUTTO", "DONE", "ERROR"};

retval ParseString(totalCmd array, char *input){
	
	initP(array);
	retval value = {0, 0};
	int i = 0, j = 0, a = 0;
	//index i for input, j for name , a for argument
	char pname[MAXSTR] = "", argn[MAXSTR] = "", ipfr[MAXSTR] = "", opto[MAXSTR] = "";
	
	char currchar;
	enum state currstate = START, nextstate;
	int arrin = 0 ;//the arrayindex in totalCmd array
	int argno = 0;// the nth argument of a command
	int ip = 0;//index in inputfrom array
	int op = 0; //index in outputto array
	while(1){
		currchar = input[i];
		switch(currstate){
			case START :
				switch(currchar){
					
					case ' ':
						i++;
						nextstate = START;
						break;
					case '<':	
					case '>':
					case '-':
					case '&':
					case '\0':
					case '|':
						nextstate = ERROR;
						break;
					default:
						if(j < MAXSTR - 1)
							pname[j++] = input[i++];
						else
							nextstate = ERROR;
						nextstate = PROGNAME;
						break;
				}
				break;
			case PROGNAME :
				switch(currchar){
					case '-':
						nextstate = ERROR;
						break;
					case '\0':
						nextstate = DONE;
						if(j < MAXSTR)
							pname[j] = '\0';
						else
							nextstate = ERROR;
						
						strcpy(array[arrin]->name, pname);
						strcpy(array[arrin]->args[0], pname);
						argno++;
						if(argno == MAXARGS)
							arrin++;
						array[arrin]->numofargs = argno; 
						arrin++;
						if(arrin == MAX)
							nextstate = DONE;
						
						break;
					case ' ':
						if(input[i - 1] != '|'){
							pname[j] = '\0';
							i++;
							strcpy(array[arrin]->name, pname);
							strcpy(array[arrin]->args[0], pname);
							argno++;
							a = 0;
							nextstate = ARGS;
						}
						else{
							i++;
							nextstate = PROGNAME;
						}
						
						break;
					case '>':
						i++;
						pname[j] = '\0';
						strcpy(array[arrin]->name, pname);
						strcpy(array[arrin]->args[0], pname);
						argno++;
						nextstate = OUTPUTTO;  
						array[arrin]->numofargs = argno;
						
						break;
					case '<':
						i++;
						pname[j] = '\0';
						strcpy(array[arrin]->name, pname);
						strcpy(array[arrin]->args[0], pname);
						argno++;
						nextstate = INPUTFROM;
						array[arrin]->numofargs = argno;
						break;
					case '&':
						if(input[i + 1] == '\0'){
							nextstate = DONE;
							value.background = 1;
							pname[j] = '\0';
							strcpy(array[arrin]->name, pname);
							strcpy(array[arrin]->args[0], pname);
							argno++;
							array[arrin]->numofargs = argno;
						}
						else
							nextstate = ERROR;
						break;
					case '|':
						array[arrin]->pipe = 1;
						if(input[i - 1] != ' '){
							pname[j] = '\0';
							strcpy(array[arrin]->name, pname);
							j = 0;
						}
						array[arrin]->numofargs = 0;
						arrin++;
						if(arrin == MAX)
							nextstate = DONE;
						
						i++;
						nextstate = PROGNAME;
						
						
						break;
						
					default :
						
						if(j < MAXSTR)
							pname[j++] = input[i++];
						else
							nextstate = ERROR;
						
						nextstate = PROGNAME;
						break;
				}
				break;
			case ARGS :
				switch(currchar){
					case '-':
						if(a < MAXSTR){
							argn[a++] = input[i++];
							nextstate = ARGS;
						}
						else	
							nextstate = ERROR;
						break;
					case ' ':
						
						argn[a] = '\0';
						strcpy(array[arrin]->args[argno], argn);
						argno++;
						if(argno == MAXARGS)
							nextstate = ERROR;
						else{
							i++;
							a = 0;
							nextstate = ARGS;
						}
						break;
					case '>':
						argn[a] = '\0';
						if(input[i - 1] != ' '){
							strcpy(array[arrin]->args[argno], argn);
							argno++;
						}
						a = 0;
						i++;
						array[arrin]->numofargs = argno;
						nextstate = OUTPUTTO;
						break;
					case '<':
						argn[a] = '\0';
						if(input[i - 1] != ' '){
							strcpy(array[arrin]->args[argno], argn);
							argno++;
						}
						array[arrin]->numofargs = argno;
						i++;
						a = 0;
						nextstate = INPUTFROM;
						break;
					case '&':
						if(input[i + 1] == '\0' ){
							nextstate = DONE;
							value.background = 1;
							if(input[i - 1] != ' '){
								argn[a] = '\0';
								a = 0;
								strcpy(array[arrin]->args[argno], argn);
								argno++;
								
							}
							array[arrin]->numofargs = argno;
							arrin++;
							
							
						}
						else
							nextstate = ERROR;
						break;
					case '|':
						array[arrin]->pipe = 1;
						if(input[i - 1] != ' '){	
							argn[a] = '\0';
							a = 0;
							strcpy(array[arrin]->args[argno], argn);
							argno++;
						}
						
						array[arrin]->numofargs = argno;
						argno = 0;
						arrin++;
						if(arrin == MAX)
							nextstate = DONE;
						else{
							i++;
							nextstate = PROGNAME;
							j = 0;
						}
						
						break;
					case '\0':
						nextstate = DONE;
						argn[a] = '\0';
						a = 0;
						strcpy(array[arrin]->args[argno], argn);
						argno++;
						array[arrin]->numofargs = argno;
						arrin++;
						break;
					default : 
						if(a < MAXSTR){
							argn[a++] = input[i++];
							nextstate = ARGS;
						}
						else{
							nextstate = DONE;
						}
						break;
											
				}
				break;
			case INPUTFROM :
				switch(currchar){
					case ' ':
						i++;
						nextstate = INPUTFROM;
						break;
					
					case '>':
						ipfr[ip] = '\0';
						ip = 0;
						i++;
						strcpy(array[arrin]->inputfrom, ipfr);
						nextstate = OUTPUTTO;
						break;
					case '-':
					case '<':
						nextstate = ERROR;
						break;
					case '|':
						
						array[arrin]->pipe = 1;
							
						ipfr[ip] = '\0';
						ip = 0;
						i++;
						strcpy(array[arrin]->inputfrom, ipfr);
						
						arrin++;
						if(arrin == MAX)
							nextstate = DONE;
						else{
							i++;
							nextstate = PROGNAME;
							j = 0;
						}
						break;
						
					case '&':
						if(input[i + 1] == '\0' && input[i - 1] != '<' && input[i - 2] != '<'){
							nextstate = DONE;
							value.background = 1;
							ipfr[ip] = '\0';
							ip = 0;
							strcpy(array[arrin]->inputfrom, ipfr);
							arrin++;
						
						}
						else
							nextstate = ERROR;
						break;
					case '\0':
						nextstate = DONE;
						ipfr[ip] = '\0';
						ip = 0;
						strcpy(array[arrin]->inputfrom, ipfr);
						arrin++;
						break;
					default:
						if(ip < MAXSTR){
							ipfr[ip++] = input[i++];
							nextstate = INPUTFROM;
						}
						else
							nextstate = ERROR;
						break;
										
				}
				break;
			case OUTPUTTO :
				switch(currchar){
					
					case ' ':
						i++;
						nextstate = OUTPUTTO;
						break;
					case '-':
					case '>':
						if(input[i - 1] == '>'){
							array[arrin]->append = 1;
							nextstate = OUTPUTTO;
							i++;
						}
						else
							nextstate = ERROR;
						break;
					case '<':
					case '|':
						nextstate = ERROR;
						break;
					case '&':
						if(input[i + 1] == '\0' && input[i - 1] != '>' && input[i - 2] != '>'){
							nextstate = DONE;
							value.background = 1;
							opto[op] = '\0';
							op = 0;
							strcpy(array[arrin]->outputto, opto);
							arrin++;
						}
						else
							nextstate = ERROR;
						break;
					case '\0':
						nextstate = DONE;
						opto[op] = '\0';
						op = 0;
						strcpy(array[arrin]->outputto, opto);
						arrin++;
						break;
					default:
						if(op < MAXSTR){
							opto[op++] = input[i++];
							nextstate = OUTPUTTO;
						}
						else
							nextstate = ERROR;
						break;					
				}
				break;
			case DONE :
				value.num = arrin;
				
				array[arrin - 1]->args[argno] = NULL;
				deallocate(array, arrin);			
				return value;
				break;
			case ERROR :
				fprintf(stderr, "Error : unexpected symbol\n");
				return value;
				break;
		}
		currstate = nextstate;
		
		
			
		
	}
}

void print(totalCmd array, int arrin){
	int i, j;
	printf("\n\n");
	for(i = 0; i < arrin; i++){
		printf("%d:\n", i);
		printf("Name = %s\n", array[i]->name);
		printf("inputfrom = %s\n", array[i]->inputfrom);
		printf("outputto = %s\n", array[i]->outputto);
		printf("Now args\n");
		for(j = 0; j < 8 && array[i]->args[j] != NULL; j++){
			printf("arg %d = %s\n", j, array[i]->args[j]);
		}
	}
	

}

