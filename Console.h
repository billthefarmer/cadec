/******	Console.h *************************************************************
 *
 *	CADEC -- Copyright (C) 1997 Bill Farmer
 *
 *	Emulation of PDP11 based telecontrol system
 *	on standard PC hardware.
 *
 *****************************************************************************/

#include <windows.h>
#include <unistd.h>

#define BLINK     0
#define BLACK     0
#define BROWN     FOREGROUND_RED | FOREGROUND_GREEN
#define LIGHTGRAY FOREGROUND_INTENSITY
#define RED       FOREGROUND_RED | FOREGROUND_INTENSITY
#define GREEN     FOREGROUND_GREEN | FOREGROUND_INTENSITY
#define YELLOW    FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY
#define BLUE      FOREGROUND_BLUE | FOREGROUND_INTENSITY
#define MAGENTA   FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY
#define CYAN      FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY
#define WHITE     FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | \
                  FOREGROUND_INTENSITY
// Text modes
#define C80
#define C4350

enum
    {SCREEN_WIDTH  = 82,
     SCREEN_HEIGHT = 50,
     BUFFER_WIDTH  = 83,
     BUFFER_HEIGHT = 51};

enum
    {TIMER = 1000};

enum
    {_NOCURSOR,
     _SOLIDCURSOR,
     _NORMALCURSOR};

// Functions
void textmode(void);
void textcolor(int, int);
void gotoxy(int, int);
void clreol(void);
void clrscr(void);
int cputs(char *);
int cprintf(char *, ...);
int movetext(int, int, int, int, int, int);
int kbhit(void);
void _setcursortype(int);
int putch(int);

int getch(void);
int wherex(void);
int wherey(void);
int screenchar(void);
int getrandom(int);
void randomize(void);
