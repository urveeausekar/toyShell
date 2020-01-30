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
int sigint_came = 0;
char input[128] = "";

void sigint_handler(){
	sigint_came = 1;
	
}

void sigtstp_handler(){
	tcsetpgrp(shell_terminal, shell_pgid);
}

int main(){
	int len, i = 0, background = 0, mode, fd, execret, numpipes, wfstatus, wgstatus, k = 0, m, l;
	pid_t pid, shell_pgid, gid, prev_pid, retpid;
	int fg = 0, bg = 0, jobnum = 0;
	pid_t paused[MAX];
	int stopped = 0, begin = -1;
	int j = 0; 
	int pipearr[MAX][2]; //array to hold all pipes
	int pidarr[MAX]; //array to hold the return values 
	
	char *currdir, pwd[128];
	retval value;
	int r = 0, c = 0;
	totalCmd table[MAX];
	
	
	
	for(j = 0; j < MAX; j++){
		table[j] = (onecmd **)malloc(sizeof(onecmd *) * MAX);
		jobtable[j].onejob = NULL;
		jobtable[j].gid = 0;
		jobtable[j].resumed = 0;
		if(table[j] == NULL){
			perror("Malloc failed");
			exit(EXIT_FAILURE);
		}
	}
	j = 0; //this index will refer to a process group ie to the job . It is current job
	//find some way to avoid buffer overflow
	
	/* Ignore interactive and job-control signals.  */
	signal (SIGINT, sigint_handler);
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

         /* set shell as foreground process  */
	tcsetpgrp(shell_terminal, shell_pgid);
	
	while(1){
		printf("\033[1;36m");
		printf("MyShell$ ");
		printf("\033[0m");
		
		
		
		/* preprocess the input string - remove \n and spaces at the end */
		if(sigint_came){
			sigint_came = 0;
			//printf("in sigint came\n");
			//strcpy(input, "\n");
		}
		
		fgets(input, 128, stdin);
		
		len = strlen(input);
		
		
		if(input[0] == '\n' || input[0] == '#')
			continue;
		
		if(input[len-1] == '\n')
			input[len - 1] = '\0';
			
		
		for(i = len - 2; i > 0 && (input[i] == ' ' || input[i] == '\t'); i--, len--)
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
		
		value = ParseString(table[j], input);
		
		if(strcmp(table[j][0]->name, "fg") == 0)
			fg = 1;
		else if(strcmp(table[j][0]->name, "bg") == 0){
			bg = 1;
			background = 1;
		}
		if(fg == 1 || bg == 1){//if you really want jobnum it will be at args 1
			if(table[j][0]->args[0] && table[j][0]->args[0][0] == '%'){
					jobnum = (int)(table[j][0]->args[0][1] - '0');
				}
			//jobnum = index of paused array at which pid of that process - 1
		}
		
		if(value.background == 1)
			background = 1;
		i = 0;
		if(fg == 0 && bg == 0){
			pid = fork();
			//printf("forked\n");
			table[j][i]->pid = pid;
		}
		else{
			prev_pid = pid;
			pid = 3;
			//printf("in pid changer\n");
		}
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
						perror("execve");
						printf("Command not found\n");
						exit(EXIT_FAILURE);
					}
				}
					
			}	
			
			
		}
		else if(pid > 0){
			if(fg == 0 && bg == 0){
				jobtable[j].onejob = table[j];
				jobtable[j].status = 'r';
				jobtable[j].background = background;
				jobtable[j].foreground = background == 0 ? 1 : 0 ;
				jobtable[j].num = value.num;
				if(jobtable[j].gid == 0)
					jobtable[j].gid = pid;
				setpgid(pid, jobtable[j].gid);
				//printf("updated job table\n");
			}
			
			if(background == 0){
				/*Then set process to foreground*/
				if(fg != 1){
					tcsetpgrp(shell_terminal, jobtable[j].gid);
					//printf("set as foreground the job, not fg\n");
				}
				if(fg == 1){
					//printf("fg read in parent background is 0\n");
					tcsetpgrp (shell_terminal, paused[stopped - 1]);
					fg = 0;
					pid = prev_pid;
					for(k = 0; k < MAX && jobtable[k].onejob != NULL ; k++)
						if(jobtable[k].gid == paused[stopped - 1]){
							jobtable[k].status = 'r';
						}
					stopped--;
					if (kill (- paused[stopped], SIGCONT) < 0)
        					perror ("kill (SIGCONT)");
        				retpid = waitpid(paused[stopped], &wfstatus, WUNTRACED | WCONTINUED);
				}
				else{
					retpid = waitpid(jobtable[j].gid, &wfstatus, WUNTRACED | WCONTINUED);
					//printf("done waiting, regular child, \n");
				}
				
				tcsetpgrp(shell_terminal, shell_pgid);
				
				/*if sigcont was received*/
				if(WIFCONTINUED(wfstatus)){
					//printf("sigcont received\n");
					for(k = 0; k < MAX && jobtable[k].onejob != NULL ; k++)
						if(jobtable[k].gid == retpid){
							jobtable[k].status = 'r';
							jobtable[k].resumed = 1;
						}
					
					retpid = waitpid(retpid, &wfstatus, WUNTRACED);
				}
				
				
				
				
				/*if sigtstp, then put job in paused array and update job table*/
				if(WIFSTOPPED(wfstatus) && WSTOPSIG(wfstatus) == SIGTSTP){
					if(stopped == MAX){
						printf("This facility is temporarily unavailable.\n");
						
					}
					else{
						paused[stopped++] = retpid;
						for(k = 0; k < MAX && jobtable[k].onejob != NULL ; k++)
							if(jobtable[k].gid == retpid){
								jobtable[k].status = 'p';
							}
						printf("\n");
						printf("[%d]   Stopped            %s\n", stopped, input);
					}
				}
				
				/*if sigint, then say job is complete*/
				if(WIFSIGNALED(wfstatus) && WTERMSIG(wfstatus) == SIGINT){
					printf("terminated by sigint\n");
					for(k = 0; k < MAX && jobtable[k].onejob != NULL ; k++){
						if(jobtable[k].gid == pid)
							jobtable[k].status = 'c';
					}
					
				}
				
				
				/*if child exited normally*/
				if(WIFEXITED(wfstatus)){
					//printf("child exited normally foreground\n");
					for(k = 0; k < MAX && jobtable[k].onejob != NULL ; k++){
						if(jobtable[k].gid == pid)
							jobtable[k].status = 'c';
					}
				}
				wfstatus = 0;
					
			}
			else{
				background = 0;
				if(bg == 1){
					bg = 0;
					pid = prev_pid;
					stopped--;
					if (kill (- paused[stopped], SIGCONT) < 0)
        					perror ("kill (SIGCONT)");
				}
				pid = waitpid(-1, &wgstatus, WNOHANG | WUNTRACED);
				
				if(WIFEXITED(wgstatus)){
					//printf("child exited normally, background\n");
					for(k = 0; k < MAX && jobtable[k].onejob != NULL ; k++){
						if(jobtable[k].gid == pid){
							jobtable[k].status = 'c';
							
						}
					}
				}
				
				j++;
				if(j == MAX){
					fprintf(stderr, "resource unavailable, buffer full");
					exit(EXIT_FAILURE);
				}
			}
			
			jobtable[j].gid = 0;//changed this now
		}
		else{
			perror("fork :");
			exit(EXIT_FAILURE);
		}
	}
	
	return 0;
}
