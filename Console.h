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
enum
    {C80,
     C50};

// Sizes
enum
    {C80_SCREEN_WIDTH  = 82,
     C80_SCREEN_HEIGHT = 24,
     C80_BUFFER_WIDTH  = 83,
     C80_BUFFER_HEIGHT = 25,
     C50_SCREEN_WIDTH  = 82,
     C50_SCREEN_HEIGHT = 50,
     C50_BUFFER_WIDTH  = 83,
     C50_BUFFER_HEIGHT = 51};

enum
    {TIMER = 1000};

// Cursor
enum
    {_NOCURSOR,
     _SOLIDCURSOR,
     _NORMALCURSOR};

// Functions
void textmode(int);
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
