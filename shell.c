#include "parser.h"
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h> 


void sigint_handler(){
	int a = 5;
	//do nothing
}


int main(){
	int pid, len, i = 0, background = 0, mode, fd, execret, numpipes;
	int j = 0; 
	int pipearr[MAX][2]; //array to hold all pipes
	int pidarr[MAX]; //array to hold the return values 
	char input[128] = "";
	char *arg2[12] = {"grep", "\"apple\"" , "names", NULL};
	char *currdir, pwd[128];
	retval value;
	int r = 0, c = 0;
	totalCmd table = (onecmd **)malloc(sizeof(onecmd *) * MAX);
	
	//initP(table);
	
	while(1){
		printf("\033[1;36m");
		printf("Pshell$ ");
		printf("\033[0m");
		
		/* preprocess the input string - remove \n and spaces at the end */
		fgets(input, 128, stdin);
		len = strlen(input);
		
		if(input[0] == '\n' || input[0] == '#')
			continue;
		
		if(input[len-1] == '\n')
			input[len - 1] = '\0';
			
		
		for(i = len - 2; i > 0 && (input[i] == ' ' || input[i] == '\t'); i--)
			input[i] = '\0';
			
		if(strcmp(input, "exit") == 0){
			/*exit the shell when user enters exit */
			return 0;
		}
		
		if(input[0] == '.' && input[1] == '/'){
			input[0] = ' ';
			input[1] = ' ';
		}
		
		/*preprocessing done now parse string*/
		
		value = ParseString(table, input);
		if(value.background == 1)
			background = 1;
		
		pid = fork();
		i = 0;
		if(pid == 0){
			printf("Child\n");
			/*check for output redirection, and redirect if necessary */
			if(table[i]->outputto != NULL && table[i]->pipe == 0){
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
			
			if(table[i]->inputfrom != NULL){
				close(0);
				fd = open(table[i]->inputfrom, O_RDONLY);
				if(fd == -1){
					perror("Couldn't open file.");
					return errno;
					
				}
			}
				
			if(table[i]->pipe == 1){
				//first find number of pipes and store in numpipes. Also make the pipes.
				numpipes = 0;
				for(i = 0; i < MAX && table[i]->pipe == 1; i++){
					if(pipe(pipearr[numpipes]) == -1){
						perror("pipe");
						exit(EXIT_FAILURE);
					}
					numpipes++;
					
				}
				
				
				
				for(i = 0; i < numpipes; i++){
					pidarr[i] = fork();
					if(pidarr[i] == 0){
						
						if(i != 0){
							close(0);
							dup(pipearr[i - 1][0]);
							
							
						}
						
						close(1);
						dup(pipearr[i][1]);
						
						for(r = 0; r < numpipes; r++)
							for(c = 0; c < 2; c++)
								if(close(pipearr[r][c]) == -1)
									perror("close");
						
						execret = execvp(table[i]->name, table[i]->args);
						perror("exec");
						exit(EXIT_FAILURE);
						
						
					}
					
					
					
				}
				
				close(0);
				dup(pipearr[i - 1][0]);
				
				for(r = 0; r < numpipes; r++)
					for(c = 0; c < 2; c++)
						if(close(pipearr[r][c]) == -1)
							perror("close");		
		
				
				
			}
			
			
			
			
			
			execret = execvp(table[i]->name, table[i]->args);
			
			if(execret == -1){
				if(errno == ENOENT){
					printf("enoent set\n");
					currdir = getcwd(pwd, 128);
					if(currdir == NULL){
						perror("Some error occured");
						exit(EXIT_FAILURE);
					}
					else{
						strcat(pwd, "/");
						strcat(pwd, table[0]->name);
						execvp(pwd, table[0]->args);
						
					}
				}
					
			}	
			
			
		}
		else if(pid > 0){
			if(background == 0){
				wait(0);
			}
			else{
				background = 0;
			}
			signal(SIGINT, sigint_handler);
		}
		else{
			perror("fork :");
			exit(EXIT_FAILURE);
		}
	}
	
	return 0;
}
