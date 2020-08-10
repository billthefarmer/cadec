# Cadec Makefile
#

GCC	= gcc
WINDRES = windres

all:		Cadec.exe

Cadec.exe:	Cadec.c Cadec.h Console.c Console.h Cadec.o Makefile
		gcc Cadec.c Console.c Cadec.o -o Cadec.exe -mconsole

%.o:	%.rc
	$(WINDRES) -o $@ $<
