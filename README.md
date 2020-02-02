This is a shell written in C, using the fork-exec model.
It implements the following functionalities:

	Read commands in a loop,
	show prompt,
	run commands,
	run commands in background,
	> (output redirection),
	< (input redirection) and
	>>
	| (pipe)
	Handle signals Ctrl-C and Ctrl-z
	fg , bg (as a stack, ie the commands are resumed in FIFO order)

