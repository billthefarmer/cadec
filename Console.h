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

// Colours
// #define BLINK     0
// #define BLACK     30
// #define BROWN     33
// #define LIGHTGRAY 90
// #define RED       91
// #define GREEN     92
// #define YELLOW    93
// #define BLUE      94
// #define MAGENTA   95
// #define CYAN      96
// #define WHITE     97

// 30 Foreground        Black 	Applies non-bold/bright black to foreground
// 33 Foreground        Yellow 	Applies non-bold/bright yellow to foreground
// 40 Background        Black 	Applies non-bold/bright black to background
// 90 Bright Foreground Black   Applies bold/bright black to foreground
// 91 Bright Foreground Red 	Applies bold/bright red to foreground
// 92 Bright Foreground Green   Applies bold/bright green to foreground
// 93 Bright Foreground Yellow  Applies bold/bright yellow to foreground
// 94 Bright Foreground Blue 	Applies bold/bright blue to foreground
// 95 Bright Foreground Magenta Applies bold/bright magenta to foreground
// 96 Bright Foreground Cyan 	Applies bold/bright cyan to foreground
// 97 Bright Foreground White   Applies bold/bright white to foreground
// 100 Bright Background Black 	Applies bold/bright black to background
// 101 Bright Background Red 	Applies bold/bright red to background
// 102 Bright Background Green 	Applies bold/bright green to background
// 103 Bright Background Yellow Applies bold/bright yellow to background
// 104 Bright Background Blue 	Applies bold/bright blue to background
// 105 Bright Background MagentaApplies bold/bright magenta to background
// 106 Bright Background Cyan 	Applies bold/bright cyan to background
// 107 Bright Background White 	Applies bold/bright white to background

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
