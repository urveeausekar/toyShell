shell:shell.o parser.o parser.h
	cc parser.o shell.o -o shell
parser.o:parser.c parser.h
	cc -c -Wall parser.c
shell.o:shell.c parser.h
	cc -c -Wall shell.c
