This is an implementation of a shell, written using Linux system calls (fork-exec, signals, etc)
It implements the following functionalities:

	1) Read commands in a loop
	2) show prompt
	3) run commands
	4) run commands in background
	6) 5) > (output redirection)
	7) < (input redirection)
	8) >>
	9) | (pipe)
	10) Handle signals Ctrl-C and Ctrl-z
	11) fg , bg (as a stack, ie the commands are resumed in FIFO order)

Files:
	shell.c : the main code for the shell
	parser.h : the header file containing the definitions of structures and typedef used in the parsing code
	parser.c : the code to parse user input
	
-------------------------------------------------------------------------------------	
How to build, run and test the project:
-------------------------------------------------------------------------------------

Requirements: gcc, make

In the folder which contains all the source files, run the following commands:

	1) make
	2) ./shell
	
This will start the shell, and you can type commands.

------------------------------------------------------------------------------------
