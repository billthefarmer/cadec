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

// textmode
void textmode()
{
    CONSOLE_SCREEN_BUFFER_INFO info;
    COORD size;
    DWORD mode;

    // Do it once
    if (hConsoleOutput == NULL)
    {
        // Disable Ctrl-C
        SetConsoleCtrlHandler(NULL, TRUE);

        // Get handles
        hConsoleOutput = GetStdHandle(STD_OUTPUT_HANDLE);
        hConsoleInput = GetStdHandle(STD_INPUT_HANDLE);

        // Update console mode
        GetConsoleMode(hConsoleOutput, &mode);
        SetConsoleMode(hConsoleOutput, mode | ENABLE_PROCESSED_OUTPUT);
        GetConsoleMode(hConsoleInput, &mode);
        SetConsoleMode(hConsoleInput, mode & !ENABLE_ECHO_INPUT);

        // Resize window
        GetConsoleScreenBufferInfo(hConsoleOutput, &info);
        info.srWindow.Right = info.srWindow.Left + SCREEN_WIDTH;
        info.srWindow.Bottom = info.srWindow.Top + SCREEN_HEIGHT;
        SetConsoleWindowInfo(hConsoleOutput, TRUE, &info.srWindow);
        size.X = BUFFER_WIDTH;
        size.Y = BUFFER_HEIGHT;
        SetConsoleScreenBufferSize(hConsoleOutput, size);

        // Set code page
        SetConsoleOutputCP(437);
    }
}

void textcolor(int f, int b)
{
    SetConsoleTextAttribute(hConsoleOutput, f + b * BACKGROUND_BLUE);
}

void gotoxy(int x, int y)
{
    COORD posn = {x + 1, y};

    SetConsoleCursorPosition(hConsoleOutput, posn);
}

void clreol()
{
    CONSOLE_SCREEN_BUFFER_INFO info;
    DWORD n;

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
    CONSOLE_SCREEN_BUFFER_INFO info;
    COORD posn = {0, 0};
    DWORD n;

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
    INPUT_RECORD record;
    DWORD n;

    // Wait for something to happen
    WaitForSingleObject(hConsoleInput, TIMER);
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
            // And key up events
            (!record.Event.KeyEvent.bKeyDown))
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
    INPUT_RECORD record;
    static BOOL special;
    DWORD n;

    // If the char is zero, return zero, but don't read event
    PeekConsoleInput(hConsoleInput, &record, 1, &n);
    if (!special && record.Event.KeyEvent.uChar.AsciiChar == 0)
    {
        special = TRUE;
        return 0;
    }

    // Read the event
    ReadConsoleInput(hConsoleInput, &record, 1, &n);
    // Not special, return the char
    if (!special)
        return record.Event.KeyEvent.uChar.AsciiChar;

    special = FALSE;
    switch (record.Event.KeyEvent.dwControlKeyState &
            (SHIFT_PRESSED | LEFT_ALT_PRESSED))
    {
        // Special, return key code
    case 0:
        return record.Event.KeyEvent.wVirtualKeyCode;

        // Special, shift pressed, return key code plus shift
    case SHIFT_PRESSED:
        return record.Event.KeyEvent.wVirtualKeyCode + VK_SHIFT;

        // Special, alt pressed, return key code plus shift
    case LEFT_ALT_PRESSED:
        return record.Event.KeyEvent.wVirtualKeyCode + VK_SHIFT * 2;

        // Special, shift and alt pressed, return key code plus shift
    case SHIFT_PRESSED | LEFT_ALT_PRESSED:
        return record.Event.KeyEvent.wVirtualKeyCode + VK_SHIFT * 3;
    }
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
