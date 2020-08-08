# Cadec Makefile
#

all:		Cadec.exe

Cadec.exe:	Cadec.c Cadec.h Console.c Console.h Makefile
		gcc Cadec.c Console.c -o Cadec.exe -mconsole
