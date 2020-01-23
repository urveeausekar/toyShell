#include "parser.h"
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

int main(){
	int pid, len, i = 0, background = 0, mode, fd;
	int j = 0, pipefd[2]; //j is the number of times we will need to run exec
	char input[128] = "";
	//char *arg2[12] = {"-l", NULL};
	retval value;
	totalCmd table = (onecmd **)malloc(sizeof(onecmd *) * 16);
	//initP(table);
	
	while(1){
		printf("\033[1;36m");
		printf("Pshell$ ");
		printf("\033[0m");
		
		/* preprocess the input string - remove \n and spaces at the end */
		fgets(input, 128, stdin);
		len = strlen(input);
		
		if(input[0] == '\n')
			continue;
		
		if(input[len-1] == '\n')
			input[len - 1] = '\0';
			
		
		for(i = len - 2; i > 0 && (input[i] == ' ' || input[i] == '\t'); i--)
			input[i] = '\0';
			
		if(strcmp(input, "exit") == 0){
			/*exit the shell when user enters exit */
			return 0;
		}
		
		
		value = ParseString(table, input);
		if(value.background == 1)
			background = 1;
		
		pid = fork();
		
		if(pid == 0){
			printf("Child\n");
			/*check for output redirection, and redirect if necessary */
			if(table[j]->outputto != NULL && table[j]->pipe == 0){
				printf("in child, redirect stdout\n");
				close(1);
				if(table[j]->append == 1)
					mode = O_WRONLY | O_CREAT | O_APPEND;
				else
					mode = O_WRONLY | O_CREAT;
					
				fd = open(table[j]->outputto, mode, S_IRUSR | S_IWUSR);
				if(fd == -1){
					perror("Couldn't open file.");
					return errno;
					
				}
				
			}
			
			if(table[j]->inputfrom != NULL){
				close(0);
				fd = open(table[j]->inputfrom, O_RDONLY);
				if(fd == -1){
					perror("Couldn't open file.");
					return errno;
					
				}
			}
				
			if(table[0]->pipe == 1){
				if(pipe(pipefd) == -1){
					perror("pipe");
					exit(EXIT_FAILURE);
				}
				if(fork() == 0){
					/*command to the left of pipe ie this process writes into pipe*/
					close(1);
					dup(pipefd[1]);
					close(pipefd[1]);
					close(pipefd[0]);
					/*stdout redirected to pipe*/
					execvp(table[0]->name, table[0]->args);
					
				}
				else{
					/* for command to the right of pipe, redirect input*/
					close(0);
					dup(pipefd[0]);
					close(pipefd[0]);
					close(pipefd[1]);
				}
			}	
			execvp(table[1]->name, table[1]->args);
				
			
			
		}
		else{
			if(background == 0){
				wait(0);
			}
			else{
				background = 0;
			}
		}
	}
	
	return 0;
}
