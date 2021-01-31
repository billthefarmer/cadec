# Cadec Makefile
#

GCC	= gcc
WINDRES = windres

all:		Cadec.exe

Cadec.exe:	Cadec.c Cadec.h Console.c Console.h Cadec.o Makefile
		$(GCC) Cadec.c Console.c Cadec.o -o Cadec.exe -mconsole

clean:
		rm *.o *.exe
%.o:	%.rc
	$(WINDRES) -o $@ $<
