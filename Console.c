/******	Console.c *************************************************************
 *
 *	CADEC -- Copyright (C) 1997 Bill Farmer
 *
 *	Emulation of PDP11 based telecontrol system
 *	on standard PC hardware.
 *
 *****************************************************************************/

#include <time.h>
#include <stdio.h>
#include <windows.h>

#include "Console.h"

// Borland console functions, API is in conio.h in Embarcadero C++ 10.2

HANDLE hConsoleOutput, hConsoleInput;

// handles
void handles()
{
    DWORD mode;

    // Disable Ctrl-C
    SetConsoleCtrlHandler(NULL, TRUE);

    // Get handles
    if (hConsoleOutput == NULL)
        hConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);

    if (hConsoleInput == NULL)
        hConsoleInput = GetStdHandle(STD_INPUT_HANDLE);

    // Update console mode
    GetConsoleMode(hConsoleOutput, &mode);
    SetConsoleMode(hConsoleOutput, mode | ENABLE_PROCESSED_OUTPUT);

    GetConsoleMode(hConsoleInput, &mode);
    SetConsoleMode(hConsoleInput, mode & !ENABLE_ECHO_INPUT);
}

// textmode
void textmode()
{
    CONSOLE_SCREEN_BUFFER_INFO info;
    HANDLE handle;

    // Get handles
    if (hConsoleOutput == NULL)
        handles();

    // Resize window
    GetConsoleScreenBufferInfo(hConsoleOutput, &info);
    info.srWindow.Right = info.srWindow.Left + 82;
    info.srWindow.Bottom = info.srWindow.Top + 48;
    SetConsoleWindowInfo(hConsoleOutput, TRUE, &info.srWindow);

    // Save handle
    handle = hConsoleOutput;

    // Create new screen buffer to get rid of scroll bars
    hConsoleOutput = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0,
                                               NULL, CONSOLE_TEXTMODE_BUFFER,
                                               NULL);
    SetConsoleActiveScreenBuffer(hConsoleOutput);

    // Set code page
    SetConsoleOutputCP(437);

    // Close old handle
    CloseHandle(handle);
}

void textcolor(int f, int b)
{
    SetConsoleTextAttribute(hConsoleOutput, f + b * BACKGROUND_BLUE);
}

void textbackground(int c)
{
}

void gotoxy(int x, int y)
{
    COORD posn = {x + 1, y};

    SetConsoleCursorPosition(hConsoleOutput, posn);
}

void clreol()
{
    DWORD n;
    CONSOLE_SCREEN_BUFFER_INFO info;

    GetConsoleScreenBufferInfo(hConsoleOutput, &info);
    FillConsoleOutputAttribute(hConsoleOutput, info.wAttributes,
                               info.dwSize.X - info.dwCursorPosition.X - 1,
                               info.dwCursorPosition, &n);
    FillConsoleOutputCharacter(hConsoleOutput, ' ',
                               info.dwSize.X - info.dwCursorPosition.X - 1,
                               info.dwCursorPosition, &n);
}

void clrscr()
{
    DWORD n;
    COORD posn = {0, 0};
    CONSOLE_SCREEN_BUFFER_INFO info;

    GetConsoleScreenBufferInfo(hConsoleOutput, &info);
    FillConsoleOutputAttribute(hConsoleOutput, info.wAttributes,
                               info.dwSize.X * info.dwSize.Y,
                               posn, &n);
    FillConsoleOutputCharacter(hConsoleOutput, ' ',
                               info.dwSize.X * info.dwSize.Y,
                               posn, &n);
}

int cputs(char *s)
{
    DWORD n;

    if (hConsoleOutput == NULL)
        handles();

    return WriteConsole(hConsoleOutput, s, strlen(s), &n, NULL);
}

int putch(int c)
{
    char s[2];

    s[0] = c;
    s[1] = '\0';
    return cputs(s);
}

int cprintf(char *f, ...)
{
    char s[96];
    va_list args;

    va_start(args, f);
    vsprintf(s, f, args);
    return cputs(s);
}

int kbhit()
{
    DWORD n;
    INPUT_RECORD record;

    GetNumberOfConsoleInputEvents(hConsoleInput, &n);

    if (n > 0)
    {
        // Ignore unwanted events
        PeekConsoleInput(hConsoleInput, &record, 1, &n);
        if ((record.EventType !=  KEY_EVENT) ||
            (record.Event.KeyEvent.wVirtualKeyCode == VK_SHIFT) ||
            (record.Event.KeyEvent.wVirtualKeyCode == VK_CONTROL) ||
            (record.Event.KeyEvent.wVirtualKeyCode == VK_MENU) ||
            (record.Event.KeyEvent.wVirtualKeyCode == VK_CAPITAL) ||
            (!record.Event.KeyEvent.bKeyDown &&
             record.Event.KeyEvent.uChar.AsciiChar > 0))
            ReadConsoleInput(hConsoleInput, &record, 1, &n);

        // Update the event count
        GetNumberOfConsoleInputEvents(hConsoleInput, &n);
    }

    return n;
}

void _setcursortype(int type)
{
    CONSOLE_CURSOR_INFO info;

    switch (type)
    {
    case _NOCURSOR:
        info.bVisible = FALSE;
        break;

    case _SOLIDCURSOR:
        info.dwSize = 100;
        info.bVisible = TRUE;
        break;

    case _NORMALCURSOR:
        info.dwSize = 20;
        info.bVisible = TRUE;
        break;
    }

    SetConsoleCursorInfo(hConsoleOutput, &info);
}

int getch()
{
    DWORD n;
    INPUT_RECORD record;

    ReadConsoleInput(hConsoleInput, &record, 1, &n);
    if (record.Event.KeyEvent.bKeyDown)
        return record.Event.KeyEvent.uChar.AsciiChar;

    else if (record.Event.KeyEvent.dwControlKeyState & LEFT_ALT_PRESSED)
        return record.Event.KeyEvent.wVirtualKeyCode + VK_SHIFT;

    else
        return record.Event.KeyEvent.wVirtualKeyCode;
}

int wherex()
{
    CONSOLE_SCREEN_BUFFER_INFO info;

    GetConsoleScreenBufferInfo(hConsoleOutput, &info);
    return info.dwCursorPosition.X - 1;
}

int wherey()
{
    CONSOLE_SCREEN_BUFFER_INFO info;

    GetConsoleScreenBufferInfo(hConsoleOutput, &info);
    return info.dwCursorPosition.Y;
}

int screenchar()
{
    u_char c;
    DWORD n;
    CONSOLE_SCREEN_BUFFER_INFO info;

    GetConsoleScreenBufferInfo(hConsoleOutput, &info);
    ReadConsoleOutputCharacter(hConsoleOutput, &c, 1,
                               info.dwCursorPosition, &n);
    return c;
}

int movetext(int left, int top, int right, int bottom,
             int destleft, int desttop)
{
    DWORD n;
    WORD attr;
    COORD source = {left + 1, top};
    COORD dest = {destleft + 1, desttop };
    SMALL_RECT scroll = {left + 1, top, right + 1, bottom};
    CHAR_INFO info = {' ', attr};
    ReadConsoleOutputAttribute(hConsoleOutput, &attr, 1, source, &n);
    return ScrollConsoleScreenBuffer(hConsoleOutput, &scroll, NULL,
                                     dest, &info);
}

void randomize()
{
    srand(time(NULL));
}

int getrandom(int n)
{
    return (n == 0)? 0: rand() % n;
}
