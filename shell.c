#include "parser.h"
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h> 

int shell_terminal = STDIN_FILENO;
int shell_pgid;

/*void sigint_handler(){
	//printf("\n");
	int a = 5;
}*/

void sigtstp_handler(){
	tcsetpgrp(shell_terminal, shell_pgid);
}

int main(){
	int len, i = 0, background = 0, mode, fd, execret, numpipes, wfstatus, wgstatus, k = 0;
	pid_t pid, shell_pgid, gid;
	int fg = 0, bg = 0;
	pid_t paused[MAX];
	int stopped = 0;
	int j = 0; 
	int pipearr[MAX][2]; //array to hold all pipes
	int pidarr[MAX]; //array to hold the return values 
	char input[128] = "";
	char *currdir, pwd[128];
	retval value;
	int r = 0, c = 0;
	totalCmd table[MAX];
	
	
	for(j = 0; j < MAX; j++){
		table[j] = (onecmd **)malloc(sizeof(onecmd *) * MAX);
		jobtable[j].onejob = NULL;
		jobtable[j].gid = 0;
		if(table[j] == NULL){
			perror("Malloc failed");
			exit(EXIT_FAILURE);
		}
	}
	j = 0; //this index will refer to a process group ie to the job . It is current job
	//find some way to avoid buffer overflow
	
	/* Ignore interactive and job-control signals.  */
	signal (SIGINT, SIG_IGN);
	signal (SIGQUIT, SIG_IGN);
	signal (SIGTSTP, SIG_IGN);
	signal (SIGTTIN, SIG_IGN);
	signal (SIGTTOU, SIG_IGN);
	signal (SIGCHLD, SIG_IGN);
	
	shell_pgid = getpid ();
	if (setpgid (shell_pgid, shell_pgid) < 0){
		perror ("Couldn't put the shell in its own process group");
		exit (1);
        }

         /* Grab control of the terminal.  */
	tcsetpgrp (shell_terminal, shell_pgid);
	
	while(1){
		printf("\033[1;36m");
		printf("MyShell$ ");
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
			
		if(strcmp(input, "fg") == 0){
			fg = 1;
		}else if(strcmp(input, "bg") == 0){
			bg = 1;
			continue;
		}
			
		if(strcmp(input, "exit") == 0){
			/*exit the shell when user enters exit */
			return 0;
		}
		
		if(input[0] == '.' && input[1] == '/'){
			input[0] = ' ';
			input[1] = ' ';
		}
		
		/*preprocessing done now parse string*/
		
		value = ParseString(table[j], input);
		
		
		if(value.background == 1)
			background = 1;
		i = 0;
		//if(fg == 0)
		pid = fork();
	
		table[j][i]->pid = pid;
		
		if(pid == 0){
			/* In the child*/
			pid = getpid();
			gid = pid;
			setpgid (pid, gid);
			if (background == 0)
        			tcsetpgrp(shell_terminal, gid);
        			
        		/* Here we have made a new process group and set it to be in foreground.
        		 * Below we reset the signal handles to default, becuse they are inherited.
        		 */
        		 
			signal (SIGINT, SIG_DFL);
			signal (SIGQUIT, SIG_DFL);
			signal (SIGTSTP, SIG_DFL);
			signal (SIGTTIN, SIG_DFL);
			signal (SIGTTOU, SIG_DFL);
			signal (SIGCHLD, SIG_DFL);
				
			if(table[j][i]->pipe == 1){
				/*first find number of pipes and store in numpipes. Also make the pipes.*/
				numpipes = 0;
				for(i = 0; i < MAX && table[j][i]->pipe == 1; i++){
					if(pipe(pipearr[numpipes]) == -1){
						perror("pipe");
						exit(EXIT_FAILURE);
					}
					numpipes++;	
				}
				
				for(i = 0; i < numpipes; i++){
					pidarr[i] = fork();
					if(pidarr[i] == 0){
						pid = getpid();
						setpgid(pid, gid);
						//added this child to parent's proces
						if(i != 0){
							close(0);
							dup(pipearr[i - 1][0]);	
						}
						else if(i == 0 && table[j][i]->inputfrom != NULL){
							close(0);
							fd = open(table[j][i]->inputfrom, O_RDONLY);
							if(fd == -1){
								perror("Couldn't open file.");
								return errno;
								
							}
						}
						
						close(1);
						dup(pipearr[i][1]);
						/*close unnecessary descriptors*/
						for(r = 0; r < numpipes; r++)
							for(c = 0; c < 2; c++)
								if(close(pipearr[r][c]) == -1)
									perror("close");
						
						execret = execvp(table[j][i]->name, table[j][i]->args);
						perror("exec");
						exit(EXIT_FAILURE);	
					}
					
					setpgid(pidarr[i], gid);	
				}
				
				close(0);
				dup(pipearr[i - 1][0]);
				
				for(r = 0; r < numpipes; r++)
					for(c = 0; c < 2; c++)
						if(close(pipearr[r][c]) == -1)
							perror("close");		
		
				
				
			}
			
			/*check for output redirection, and redirect if necessary */
			if(table[j][i]->outputto != NULL && table[j][i]->pipe == 0){
				
				close(1);
				if(table[j][i]->append == 1)
					mode = O_WRONLY | O_CREAT | O_APPEND;
				else
					mode = O_WRONLY | O_CREAT;
					
				fd = open(table[j][i]->outputto, mode, S_IRUSR | S_IWUSR);
				if(fd == -1){
					perror("Couldn't open file.");
					return errno;
					
				}
				
			}
			
			if(table[j][i]->inputfrom != NULL){
				close(0);
				fd = open(table[j][i]->inputfrom, O_RDONLY);
				if(fd == -1){
					perror("Couldn't open file.");
					return errno;
					
				}
			}
			
			execret = execvp(table[j][i]->name, table[j][i]->args);
			
			if(execret == -1){
				if(errno == ENOENT){
					
					currdir = getcwd(pwd, 128);
					if(currdir == NULL){
						perror("Some error occured");
						exit(EXIT_FAILURE);
					}
					else{
						strcat(pwd, "/");
						strcat(pwd, table[j][i]->name);
						execvp(pwd, table[j][i]->args);
						
					}
				}
					
			}	
			
			
		}
		else if(pid > 0){
			
			jobtable[j].onejob = table[j];
			jobtable[j].status = 'r';
			jobtable[j].background = background;
			jobtable[j].foreground = background == 0 ? 1 : 0 ;
			jobtable[j].num = value.num;
			if(jobtable[j].gid == 0)
				jobtable[j].gid = pid;
			setpgid(pid, jobtable[j].gid);
			
			
			if(background == 0){
				/*Then set process to foreground*/
				tcsetpgrp(shell_terminal, jobtable[j].gid);
				/*if(fg == 1){
					tcsetpgrp (shell_terminal, paused[stopped - 1]);
					fg = 0;
					if (kill (- paused[stopped - 1], SIGCONT) < 0)
        					perror ("kill (SIGCONT)");
				}*/
				pid = waitpid(-1, &wfstatus, WUNTRACED);
				
				tcsetpgrp(shell_terminal, shell_pgid);
				if(WIFSTOPPED(wfstatus)){
					paused[stopped++] = pid;
					for(k = 0; k < MAX && jobtable[k].onejob != NULL ; k++)
						if(jobtable[k].gid == pid){
							jobtable[k].status = 'p';
						}
				}
					
			}
			else{
				background = 0;
				if(bg == 1){
					bg = 0;
					if (kill (- paused[stopped - 1], SIGCONT) < 0)
        					perror ("kill (SIGCONT)");
				}
				waitpid(-1, &wgstatus, WNOHANG);
				j++;
			}
		
		}
		else{
			perror("fork :");
			exit(EXIT_FAILURE);
		}
	}
	
	return 0;
}
