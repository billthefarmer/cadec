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

// FOREGROUND_BLUE 	Text color contains blue.
// FOREGROUND_GREEN 	Text color contains green.
// FOREGROUND_RED 	Text color contains red.
// FOREGROUND_INTENSITY Text color is intensified.
// BACKGROUND_BLUE 	Background color contains blue.
// BACKGROUND_GREEN 	Background color contains green.
// BACKGROUND_RED 	Background color contains red.
// BACKGROUND_INTENSITY Background color is intensified.

// #define FOREGROUND_BLUE 0x1
// #define FOREGROUND_GREEN 0x2
// #define FOREGROUND_RED 0x4
// #define FOREGROUND_INTENSITY 0x8
// #define BACKGROUND_BLUE 0x10
// #define BACKGROUND_GREEN 0x20
// #define BACKGROUND_RED 0x40
// #define BACKGROUND_INTENSITY 0x80

// Text modes
#define C80
#define C4350
#define ESC "\x1b"
#define CSI "\x1b["

enum
    {WIDTH  = 82,
     HEIGHT = 50};

enum
    {TIMER = 1000};

enum
    {_NOCURSOR,
     _SOLIDCURSOR,
     _NORMALCURSOR};

// Functions
void textmode(void);
void textcolor(int, int);
void textbackground(int);
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
