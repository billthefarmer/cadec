# Cadec Makefile
#

RM	= rm
GCC	= gcc
WINDRES = windres

all:		Cadec.exe

Cadec.exe:	Cadec.c Cadec.h Console.c Console.h Cadec.o Makefile
		$(GCC) Cadec.c Console.c Cadec.o -o Cadec.exe -mconsole

clean:
		$(RM) *.o *.exe

%.o:	%.rc
	$(WINDRES) -o $@ $<
