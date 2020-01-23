#include "parser.h"
#include <string.h>
#include <errno.h>

int initP(totalCmd array){
	//array =(onecmd **)malloc(16 * sizeof(onecmd *));
	int i, j;
	if (array == NULL){
		fprintf(stdout, "Some error: Memory not allocated for array itself\n");
		return ENOMEM;
	}
	for(i = 0; i < 16; i++){
		array[i] = (onecmd *)malloc(sizeof(onecmd));
		if(array[i] == NULL){
			fprintf(stdout, "Some error: Memory not allocated for structure, i = %d\n", i);
			return ENOMEM;
		}
		array[i]->name = (char *)malloc(sizeof(char) * 32);
		array[i]->inputfrom = (char *)malloc(sizeof(char) * 32);
		array[i]->outputto = (char *)malloc(sizeof(char) * 32);
		if(array[i]->name == NULL || array[i]->inputfrom == NULL || array[i]->outputto == NULL){
			fprintf(stdout, "Some error: Memory not allocated for name or io file name\n");
			return ENOMEM;
		}
		
		// a nul at index 0 signifies that that array is still unused
		array[i]->name[0] = '\0';
		array[i]->outputto[0] = '\0';
		array[i]->inputfrom[0] = '\0';
		
		array[i]->args = (char **)malloc(sizeof(char *) * 8);
		if(array[i]->args == NULL){
			fprintf(stdout, "Some error: Memory not allocated for some args array\n");
			return ENOMEM;
		}
		for(j = 0; j < 8; j++){
			array[i]->args[j] = (char *)malloc(sizeof(char) * 32);
			
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
		//array[i]->background = 0;
		
	}
	return 0;
}



void deallocate(totalCmd array, int arrin){
	int i;
	printf("in deallocate\n");
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
		/*printf("just before for\n");
		//FIXME:decide what to do about args
		for(j = 0; j < 8; j++){
			printf("before if %d\n", j);
			if(array[i]->args[j][0] == '\0'){
				//printf("%d , %d \n",i, j);
				free(array[i]->args[j]);
				array[i]->args[j] = NULL;
				break;
			}
		}*/
		
		free(array[i]->args[array[i]->numofargs]);
		array[i]->args[array[i]->numofargs] = NULL;
	}
	printf("out of dealloc\n");
}



enum state {START, PROGNAME, ARGS, INPUTFROM, OUTPUTTO, DONE, ERROR};
char *states[10] = {"START", "PROGNAME", "ARGS", "INPUTFROM", "OUTPUTTO", "DONE", "ERROR"};

retval ParseString(totalCmd array, char *input){
	//int len = strlen(input);
	initP(array);
	retval value = {0, 0};
	int i = 0, j = 0, a = 0;
	//index i for input, j for name , a for argument
	char pname[32] = "", argn[32] = "", ipfr[32] = "", opto[32] = "";
	
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
						pname[j++] = input[i++];
						nextstate = PROGNAME;
						printf("in start default\n");
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
						pname[j] = '\0';
						if(array[arrin]->name == NULL)
							printf("array[arrin]->name null\n");
						
						strcpy(array[arrin]->name, pname);
						if(array[arrin]->args[0] == NULL)
							printf("array[arrin]->name null\n");
						strcpy(array[arrin]->args[0], pname);
						argno++;
						array[arrin]->numofargs = argno; 
						arrin++;
						printf("in progname, null\n");
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
						nextstate = OUTPUTTO;  //FIXME: do I work properly
						array[arrin]->numofargs = argno;
						//printf("ls > file: numofargs = %d\n", argno); 
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
						
						i++;
						nextstate = PROGNAME;
						
						/*
						free(array[arrin]->inputfrom);
						array[arrin]->inputfrom = NULL;
						free(array[arrin]->outputto);
						array[arrin]->outputto = NULL;*/
						break;
						
					default :
						pname[j++] = input[i++];
						printf("in progname, default\n");
						nextstate = PROGNAME;
						break;
				}
				break;
			case ARGS :
				switch(currchar){
					case '-':
						argn[a++] = input[i++];
						nextstate = ARGS;
						break;
					case ' ':
						argn[a] = '\0';
						strcpy(array[arrin]->args[argno], argn);
						argno++;
						i++;
						a = 0;
						nextstate = ARGS;
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
						strcpy(array[arrin]->args[argno], argn);
						argno++;
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
							printf("in args ampersand argno %d\n", argno);
							
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
						//printf("args pipe set argno to 0\n");
						arrin++;
						i++;
						nextstate = PROGNAME;
						j = 0;
						/*
						free(array[arrin]->inputfrom);
						array[arrin]->inputfrom = NULL;
						free(array[arrin]->outputto);
						array[arrin]->outputto = NULL;*/
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
						argn[a++] = input[i++];
						nextstate = ARGS;
						break;
											
				}
				break;
			case INPUTFROM :
				switch(currchar){
					case ' ':
						i++;
						nextstate = INPUTFROM;
						break;
					case '-':
					case '>':
					case '<':
					case '|':
						nextstate = ERROR;
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
						ipfr[ip++] = input[i++];
						nextstate = INPUTFROM;
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
						opto[op++] = input[i++];
						nextstate = OUTPUTTO;
						break;					
				}
				break;
			case DONE :
				value.num = arrin;
				//printf("in done\n, arrin is %d, argno is %d \n", arrin, argno);
				array[arrin - 1]->args[argno] = NULL;
				printf("done : the input was : %s\n", input);
				print(array, arrin);
				printf("debugging ends here ----------------------------\n");
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

