/****** cadec.c ***************************************************************
 *
 *  CADEC -- Copyright (C) 1997 Bill Farmer
 *
 *  Emulation of PDP11 based telecontrol system
 *  on standard PC hardware.
 *
 *****************************************************************************/

char copy[] = "CADEC -- Copyright (C) 1997 Bill Farmer";

#include <process.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <time.h>

#include "Cadec.h"
#include "Console.h"

/*****************************************************************************/

char Command[96] = {0},
    *CommandList[] =
    {"GIN", "SSD", "SST", "EVT", "ALM", "ALA",
     "OPE", "CLS", "PON", "POF", "AON", "AOF",
     "RFR", "DCN", "PGB", "PGO", "FND", "SYS",
     "SPR", "TRP", "EFI", "ANA", "OPS", "PSR",
     "PPC", "PTR", "DRW", "SAR", "ZAP", "SJF",
     "SSP", "SFR", "EXT", "EDB", "EDR", "DCS",
     "CAD", "RES", "STP", "STA", "SHT"},
    *ItemList[] =
    {"SUB", "EQU", "SWG", "TAP", "VAL"},
    *CommandWords[] =
    {NULL, NULL, NULL, NULL, NULL, NULL};

/*****************************************************************************/

int CommandIndex = 0,
    CommandNumber= 0,
    nSubstation  = 0,
    nEquipment   = 0,
    nAlarms      = 0,
    nConcentrator= 0,
    nOutstation  = 0,
    nCoords      = 0,
    TimeOut      = 0,
    Stopit       = 0,
    Dingdong     = 0,
    AlarmNumber  = 1,
    Frequency    = 4998,
    ProbTrip     = 5,
    ProbReclose  = 75,
    ProbCursor   = 5,
    JitterFactor = 5,
    SerialPort   = 1;

/*****************************************************************************/

Substation     *pSubstation     = NULL;
Equipment      *pEquipment      = NULL;
Coords         *pCoords         = NULL;
Transformers   *pTransformers   = NULL;
CurrentDisplay *pCurrentDisplay = NULL;
AlarmList      *pAlarmList      = NULL;
Concentrator   *pConcentrator   = NULL;
Outstation     *pOutstation     = NULL;

/*****************************************************************************/

void main(int argc, char *argv[])
{
    randomize();

    InitDisplay();
    InitExternal();

    LoadData();
    LoadAlarms();
    LoadNetwork();

    DisplayBanner();
    DisplayCadec();

    DoCadec();
}

/*****************************************************************************/

void DoCadec()
{
    time_t s, t;

    s = time(NULL);
    DisplayTime(s);

    for (;;)
    {
        if (kbhit())
            ProcessKey();

        t = time(NULL);
        if (s != t)
        {
            s = t;
            ProcessTime(t);
        }
    }
}

/*****************************************************************************/

void DisplayCancel()
{
    pCurrentDisplay->Display = 0;

    SetColours(WHITE, BLACK);
    gotoxy(19, 7);
    clreol();

    ClearDisplay();
    ClearCommand();
}

/*****************************************************************************/

void ClearCommand()
{
    SetColours(WHITE, BLACK);
    gotoxy(1, 1);
    clreol();

    gotoxy(1, 2);
    clreol();
}

/*****************************************************************************/

void ClearDisplay()
{
    int i;

    SetColours(WHITE, BLACK);
    for (i = 9; i < 46; i++)
    {
        gotoxy(1, i);
        clreol();
    }
}

/*****************************************************************************/

void DisplayBackground(char *FileName)
{
    int i;
    FILE *File;
    char TextLine[96];

    File = fopen(FileName, "r");

    if (File == NULL)
    {
        DisplayError("CADEC DISPLAY FILE NOT FOUND");
        return;
    }

    ClearDisplay();
    SetColours(LIGHTGRAY, BLACK);

    for (i = 9; i < 46; i++)
    {
        if (fgets(TextLine, 95, File) == NULL)
            break;

        gotoxy(1, i);
        cputs(TextLine);
    }

    ClearCommand();
    fclose(File);
}

/*****************************************************************************/

void DisplayIndex()
{
    pCurrentDisplay->Display = Index;
    pCurrentDisplay->Function = Init;

    IndexDisplay();
}

/*****************************************************************************/

void IndexDisplay()
{
    int i, n;
    FILE *File;
    char TextLine[96];

    switch(pCurrentDisplay->Function)
    {
    case Init:
        DisplayHeader("GENERAL INDEX");
        pCurrentDisplay->Page = 1;
        break;

    case Pageon:
        pCurrentDisplay->Page++;
        break;

    case Pageback:
        if (pCurrentDisplay->Page == 1)
        {
            DisplayError("INVALID PAGING");
            return;
        }

        pCurrentDisplay->Page--;
        break;
    }

    File = fopen("CADEC.HLP", "r");

    if (File == NULL)
    {
        DisplayError("CADEC INDEX FILE NOT FOUND");
        return;
    }

    while (!feof(File))
    {
        if (fgets(TextLine, 95, File) == NULL)
            break;

        if ((sscanf(TextLine, "# %d", &n) == 1) &&
            (pCurrentDisplay->Page == n))
            break;
    }

    if (feof(File))
    {
        DisplayError("PAGE NOT FOUND");

        pCurrentDisplay->Page--;
        if (pCurrentDisplay->Page < 1)
            pCurrentDisplay->Page = 1;

        fclose(File);
        return;
    }

    ClearDisplay();
    SetColours(YELLOW, BLACK);

    n = 0;
    for (i = 9; i < 46; i++)
    {
        if ((fgets(TextLine, 95, File) == NULL) ||
            (sscanf(TextLine, "# %d", &n) == 1))
            break;

        gotoxy(1, i);
        cputs(TextLine);
    }

    gotoxy(71, 7);
    SetColours(GREEN, BLACK);
    cprintf("PAGE %d", pCurrentDisplay->Page);
    clreol();

    if ((n != -1) &&
        (fgets(TextLine, 95, File) != NULL) &&
        (sscanf(TextLine, "# %d", &n) == 1) &&
        (n != -1))
        cputs(" >>");

    ClearCommand();
    fclose(File);
}

/*****************************************************************************/

void DisplayDirectory()
{
    int i;
    Substation *pSub;
 
    pCurrentDisplay->Display = Directory;

    pSub = malloc(sizeof(Substation) * nSubstation);
    if (pSub == NULL)
    {
        DisplayError("NOT ENOUGH MEMORY");
        return;
    }

    memcpy(pSub, pSubstation, sizeof(Substation) * nSubstation);

    if (CommandNumber == 0)
    {
        qsort(pSub, nSubstation, sizeof(Substation),
              (int (*)())Namecmp);
    }
    else
    {
        qsort(pSub, nSubstation, sizeof(Substation),
              (int (*)())Spenscmp);
    }

    ClearDisplay();
    DisplayHeader("GLOBAL SUBSTATION DIRECTORY");

    gotoxy(71, 7);
    SetColours(GREEN, BLACK);
    cputs("PAGE 1");

    SetColours(YELLOW, BLACK);

    for (i = 0; i < nSubstation; i++)
    {
        gotoxy(1 + ((i / 18) * 25), 10 + ((i % 18) * 2));
        if (pSub[i].Equipment != NULL)
            SetColours(YELLOW, BLACK);
        else
            SetColours(BROWN, BLACK);
        cprintf("%s %s", pSub[i].Spens, pSub[i].Name);
    }

    ClearCommand();
    free(pSub);
}

/*****************************************************************************/

int Namecmp(Substation *s, Substation *t)
{
    return strcmp(s->Name, t->Name);
}

/*****************************************************************************/

int Spenscmp(Substation *s, Substation *t)
{
    return strcmp(s->Spens, t->Spens);
}

/*****************************************************************************/

void DisplaySubstation()
{
    int n;

    if ((CommandNumber != 0) ||
        ((pCurrentDisplay->Display != Drawing) &&
         (pCurrentDisplay->Display != Data) &&
         (pCurrentDisplay->Display != Substat)))
    {
        for (n = 0; n < nSubstation; n++)
        {
            if (strcmp(CommandWords[1], pSubstation[n].Spens) == 0)
                break;
        }

        if (n == nSubstation)
        {
            DisplayError("SUBSTATION NOT FOUND");
            return;
        }

        if (pSubstation[n].Equipment == NULL)
        {
            DisplayError("SUBSTATION HAS NO EQUIPMENT");
            return;
        }

        pCurrentDisplay->Substation = &pSubstation[n];
    }

    pCurrentDisplay->Equipment = pCurrentDisplay->Substation->Equipment;
    pCurrentDisplay->Function = Init;
    pCurrentDisplay->Display = Substat;

    SubstationDisplay();
}

/*****************************************************************************/

void SubstationDisplay()
{
    switch(pCurrentDisplay->Function)
    {
    case Init:
        SubstationOne();
        break;

    case Pageon:
        switch (pCurrentDisplay->Page)
        {
        case 1:
            if (pCurrentDisplay->Next != NULL)
                SubstationTwo();
            else
                DisplayError("INVALID PAGING");
            break;

        case 2:
            if (pCurrentDisplay->Next != NULL)
                SubstationThree();
            else
                DisplayError("INVALID PAGING");
            break;

        case 3:
            DisplayError("INVALID PAGING");
            break;
        }
        break;

    case Pageback:
        switch (pCurrentDisplay->Page)
        {
        case 1:
            DisplayError("INVALID PAGING");
            break;

        case 2:
            SubstationOne();
            break;

        case 3:
            SubstationTwo();
            break;
        }
        break;

    case Refresh:
        switch (pCurrentDisplay->Page)
        {
        case 1:
            SubstationOne();
            break;

        case 2:
            SubstationTwo();
            break;

        case 3:
            SubstationThree();
            break;
        }
        break;
    }
}

/*****************************************************************************/

void SubstationOne()
{
    pCurrentDisplay->Page = 1;
    pCurrentDisplay->Next = pCurrentDisplay->Equipment->Next;

    if (pCurrentDisplay->Function != Refresh)
    {
        DisplayBackground("DISPLAY.001");
        DisplayHeader(pCurrentDisplay->Substation->Name);

        gotoxy(26, 7);
        SetColours(BLACK, WHITE);
        cputs(pCurrentDisplay->Substation->Spens);

        gotoxy(71, 7);
        SetColours(GREEN, BLACK);
        cprintf("PAGE %d", pCurrentDisplay->Page);

        if (pCurrentDisplay->Next != NULL)
            cputs(" >>");
    }

    gotoxy(2, 9);
    SetColours(YELLOW, BLACK);
    cputs(pCurrentDisplay->Equipment->Spens);

    gotoxy(5, 9);
    cputs(pCurrentDisplay->Equipment->Name);

    gotoxy(60, 9);
    cprintf("%3d", pCurrentDisplay->Equipment->Operns);

    gotoxy(18, 11);
    SetColours(GREEN, BLACK);
    cprintf("%2d.%02d", pCurrentDisplay->Equipment->Analog[0] / 100,
            pCurrentDisplay->Equipment->Analog[0] % 100);

    gotoxy(17, 13);
    cprintf("%2d.%02d", Frequency / 100, Frequency % 100);

    gotoxy(2, 18);
    clreol();

    SetColours(RED, BLACK);
    if (pCurrentDisplay->Equipment->Status & StatusProtAlarm)
        cputs("FEEDER PROTECTION OP");

    gotoxy(41, 18);
    SetColours(YELLOW, BLACK);
    cputs("VOLTAGE REDUCTION NORMAL");
}

/*****************************************************************************/

void SubstationTwo()
{
    int n;
    Equipment *pTrans;

    pCurrentDisplay->Page = 2;

    if (pCurrentDisplay->Function != Refresh)
    {
        DisplayBackground("DISPLAY.002");
        DisplayHeader(pCurrentDisplay->Substation->Name);

        gotoxy(26, 7);
        SetColours(BLACK, WHITE);
        cputs(pCurrentDisplay->Substation->Spens);

        gotoxy(71, 7);
        SetColours(GREEN, BLACK);
        cprintf("PAGE %d", pCurrentDisplay->Page);
    }

    gotoxy(20, 9);
    SetColours(YELLOW, BLACK);
    cputs(pCurrentDisplay->Substation->Switchgear);

    gotoxy(20, 10);
    cputs(pCurrentDisplay->Substation->TapChanger);

    pCurrentDisplay->Next = pCurrentDisplay->Equipment->Next;

    n = 0;

    while (pCurrentDisplay->Next != NULL)
    {
        if (pCurrentDisplay->Next->Spens[0] != 'T')
            break;

        pTrans = pCurrentDisplay->Next->Trans;

        gotoxy(2, 16 + n);
        SetColours(YELLOW, BLACK);
        cputs(pCurrentDisplay->Next->Spens);

        clreol();

        if ((pTrans->Status & (StatusOpen | StatusClosed)) != StatusOpen)
        {
            gotoxy(11, 16 + n);
            cprintf("%2.1f", ((pCurrentDisplay->Next->Analog[0] / 100.0)
                              * pTrans->Analog[1] * 1.732) / 1000.0);

            gotoxy(16, 16 + n);
            cprintf("%2.1f", (((pCurrentDisplay->Next->Analog[0] / 100.0)
                               * pTrans->Analog[1] * 1.732) / 1000.0)
                    * (pCurrentDisplay->Equipment->Analog[0] / 100.0));


            gotoxy(21, 16 + n);
            SetColours(GREEN, BLACK);
            cprintf("%2.2f", pCurrentDisplay->Next->Analog[0] / 100.0);

            gotoxy(28, 16 + n);
            cprintf("%2d.%02d", pCurrentDisplay->Equipment->Analog[0] / 100,
                    pCurrentDisplay->Equipment->Analog[0] % 100);
        }

        else
        {
            gotoxy(11, 16 + n);
            cprintf("%2.1f", 0.0);

            gotoxy(16, 16 + n);
            cprintf("%2.1f", 0.0);

            gotoxy(21, 16 + n);
            SetColours(GREEN, BLACK);
            cprintf("%2.2f", 0.0);

            gotoxy(28, 16 + n);
            cprintf("%2.2f", 0.0);
        }

        gotoxy(36, 16 + n);
        SetColours(YELLOW, BLACK);
        cputs("PARAL AUT");

        gotoxy(46, 16 + n);
        SetColours(GREEN, BLACK);
        cprintf("%02d", pCurrentDisplay->Next->Operns);

        pCurrentDisplay->Next = pCurrentDisplay->Next->Next;
        n++;
    }

    n = 0;

    while (pCurrentDisplay->Next != NULL)
    {
        if (pCurrentDisplay->Next->Spens[0] == '0')
            break;

        gotoxy(2, 31 + n);
        SetColours(YELLOW, BLACK);
        cprintf("%s    %s", pCurrentDisplay->Next->Spens,
                pCurrentDisplay->Next->Name);

        clreol();

        gotoxy(31, 31 + n);
        switch(pCurrentDisplay->Next->Status & (StatusOpen | StatusClosed))
        {
        case StatusOpen:
            cputs("OPEN");
            break;
        case StatusClosed:
            cputs("CLSD");
            break;
        default:
            cputs("****");
            break;
        }

        gotoxy(22, 31 + n);
        SetColours(CYAN, BLACK);

        if (pCurrentDisplay->Next->Operns >= 0)
            cprintf("%2d", pCurrentDisplay->Next->Operns);
        else
            cputs("**");

        if (pCurrentDisplay->Next->Analog[0] >= 0)
        {
            gotoxy(25, 31 + n);
            SetColours(GREEN, BLACK);

            if ((pCurrentDisplay->Next->Status & (StatusOpen | StatusClosed)) != StatusOpen)
                cprintf("%4d", pCurrentDisplay->Next->Analog[1]);

            else
                cprintf("%4d", 0);
        }

        if (pCurrentDisplay->Next->Status & StatusAutomation)
        {
            gotoxy(49, 31 + n);
            SetColours(BLACK, WHITE);
            switch(pCurrentDisplay->Next->Status & (StatusAutoOff | StatusAutoOn))
            {
            case StatusAutoOff:
                cputs("OFF");
                break;
            case StatusAutoOn:
                cputs("ON ");
                break;
            default:
                cputs("***");
                break;
            }
        }

        if (pCurrentDisplay->Next->Status & StatusProtection)
        {
            gotoxy(54, 31 + n);
            SetColours(YELLOW, BLACK);
            switch(pCurrentDisplay->Next->Status & (StatusProtOff | StatusProtOn))
            {
            case StatusProtOff:
                cputs("OFF");
                break;
            case StatusProtOn:
                cputs("ON ");
                break;
            default:
                cputs("***");
                break;
            }
        }

        if (pCurrentDisplay->Next->Status & StatusTripped)
        {
            gotoxy(59, 31 + n);
            SetColours(BLACK, RED);
            cputs("TRP");
        }

        if (pCurrentDisplay->Next->Status & StatusLockout)
        {
            gotoxy(63, 31 + n);
            SetColours(BLACK, RED);
            cputs("LCK");
        }

        if (pCurrentDisplay->Next->Status & StatusEfiAlarm)
        {
            gotoxy(67, 31 + n);
            SetColours(BLACK, RED);
            cputs("EFI");
        }

        if (pCurrentDisplay->Next->Status & StatusExternal)
        {
            gotoxy(71, 31 + n);
            SetColours(YELLOW, BLACK);
            cputs("EXT");
        }

        pCurrentDisplay->Next = pCurrentDisplay->Next->Next;
        n++;
    }

    if (pCurrentDisplay->Next != NULL)
    {
        gotoxy(78, 7);
        SetColours(GREEN, BLACK);
        cputs(">>");
    }
}

/*****************************************************************************/

void SubstationThree()
{
    int n;

    pCurrentDisplay->Page = 3;

    if (pCurrentDisplay->Function != Refresh)
    {
        DisplayBackground("DISPLAY.003");
        DisplayHeader(pCurrentDisplay->Substation->Name);

        gotoxy(26, 7);
        SetColours(BLACK, WHITE);
        cputs(pCurrentDisplay->Substation->Spens);

        gotoxy(71, 7);
        SetColours(GREEN, BLACK);
        cprintf("PAGE %d", pCurrentDisplay->Page);
    }

    gotoxy(20, 9);
    SetColours(YELLOW, BLACK);
    cputs(pCurrentDisplay->Substation->Switchgear);

    pCurrentDisplay->Next = pCurrentDisplay->Equipment->Next;
    while (pCurrentDisplay->Next != NULL)
    {
        if (pCurrentDisplay->Next->Spens[0] == '0')
            break;

        pCurrentDisplay->Next = pCurrentDisplay->Next->Next;
    }

    n = 0;

    while (pCurrentDisplay->Next != NULL)
    {
        gotoxy(2, 14 + (n * 2));
        SetColours(YELLOW, BLACK);
        cprintf("%s    %s", pCurrentDisplay->Next->Spens,
                pCurrentDisplay->Next->Name);

        clreol();

        gotoxy(31, 14 + (n * 2));
        switch(pCurrentDisplay->Next->Status & (StatusOpen | StatusClosed))
        {
        case StatusOpen:
            cputs("OPEN");
            break;
        case StatusClosed:
            cputs("CLSD");
            break;
        default:
            cputs("****");
            break;
        }

        if (pCurrentDisplay->Next->Status & StatusAutomation)
        {
            gotoxy(49, 14 + (n * 2));
            SetColours(BLACK, WHITE);
            switch(pCurrentDisplay->Next->Status & (StatusAutoOff | StatusAutoOn))
            {
            case StatusAutoOff:
                cputs("OFF");
                break;
            case StatusAutoOn:
                cputs("ON ");
                break;
            default:
                cputs("***");
                break;
            }
        }

        if (pCurrentDisplay->Next->Status & StatusProtection)
        {
            gotoxy(54, 14 + (n * 2));
            SetColours(YELLOW, BLACK);
            switch(pCurrentDisplay->Next->Status & (StatusProtOff | StatusProtOn))
            {
            case StatusProtOff:
                cputs("OFF");
                break;
            case StatusProtOn:
                cputs("ON ");
                break;
            default:
                cputs("***");
                break;
            }
        }

        gotoxy(22, 14 + (n * 2));
        SetColours(CYAN, BLACK);

        if (pCurrentDisplay->Next->Operns >= 0)
            cprintf("%2d", pCurrentDisplay->Next->Operns);
        else
            cputs("**");

        if (pCurrentDisplay->Next->Analog[0] >= 0)
        {
            gotoxy(25, 14 + (n * 2));
            SetColours(GREEN, BLACK);

            if ((pCurrentDisplay->Next->Status &
                 (StatusOpen | StatusClosed)) != StatusOpen)
                cprintf("%4d", pCurrentDisplay->Next->Analog[1]);

            else
                cprintf("%4d", 0);
        }

        if (pCurrentDisplay->Next->Status & StatusTripped)
        {
            gotoxy(59, 14 + (n * 2));
            SetColours(BLACK, RED);
            cputs("TRP");
        }

        if (pCurrentDisplay->Next->Status & StatusLockout)
        {
            gotoxy(63, 14 + (n * 2));
            SetColours(BLACK, RED);
            cputs("LCK");
        }

        if (pCurrentDisplay->Next->Status & StatusEfiAlarm)
        {
            gotoxy(67, 14 + (n * 2));
            SetColours(BLACK, RED);
            cputs("EFI");
        }

        if (pCurrentDisplay->Next->Status & StatusExternal)
        {
            gotoxy(71, 14 + (n * 2));
            SetColours(YELLOW, BLACK);
            cputs("EXT");
        }

        pCurrentDisplay->Next = pCurrentDisplay->Next->Next;

        n++;
        if (n == 16)
            break;
    }

}

/*****************************************************************************/

void DisplayDrawing()
{
    int n;

    if ((CommandNumber != 0) ||
        ((pCurrentDisplay->Display != Substat) &&
         (pCurrentDisplay->Display != Drawing)))
    {
        for (n = 0; n < nSubstation; n++)
        {
            if (strcmp(CommandWords[1], pSubstation[n].Spens) == 0)
                break;
        }

        if (n == nSubstation)
        {
            DisplayError("SUBSTATION NOT FOUND");
            return;
        }

        if (pSubstation[n].Equipment == NULL)
        {
            DisplayError("SUBSTATION HAS NO EQUIPMENT");
            return;
        }

        pCurrentDisplay->Substation = &pSubstation[n];
    }

    pCurrentDisplay->Equipment = pCurrentDisplay->Substation->Equipment;
    pCurrentDisplay->Function = Init;
    pCurrentDisplay->Display = Drawing;

    DrawingDisplay();
}

/*****************************************************************************/

void DrawingDisplay()
{
    int n, xEqu, yEqu, xAna, yAna;
    Equipment *pTrans, *pEqu;
    FILE *File;
    char Spens[8],
        FileName[16],
        TextLine[96];

    if (pCurrentDisplay->Function == Init)
    {
        strcpy(FileName, pCurrentDisplay->Substation->Spens);
        strcat(FileName, ".DRW");

        File = fopen(FileName, "r");
        if (File == NULL)
        {
            File = fopen("CADEC.DRW", "r");
            if (File == NULL)
            {
                DisplayError("BACKGROUND FILE NOT FOUND");
                return;
            }
        }

        DisplayHeader(pCurrentDisplay->Substation->Name);

        gotoxy(26, 7);
        SetColours(BLACK, WHITE);
        cputs(pCurrentDisplay->Substation->Spens);

        gotoxy(71, 7);
        SetColours(GREEN, BLACK);
        cputs("PAGE 1");

        ClearDisplay();
        SetColours(LIGHTGRAY, BLACK);
        for (n = 9; n < 46; n++)
        {
            if (fgets(TextLine, 95, File) == NULL)
                break;

            gotoxy(1, n);
            cputs(TextLine);
        }

        fclose(File);

        n = 0;
        pEqu = pCurrentDisplay->Equipment;
        while (pEqu != NULL)
        {
            pEqu = pEqu->Next;
            n++;
        }

        nCoords = n;

        if (pCoords != NULL)
            free(pCoords);

        pCoords = malloc(sizeof(Coords) * nCoords);
        if (pCoords == NULL)
        {
            DisplayError("NOT ENOUGH MEMORY");
            return;
        }

        n = 0;
        pEqu = pCurrentDisplay->Equipment;
        while (pEqu != NULL)
        {
            pCoords[n].Equipment = pEqu;

            pCoords[n].xEqu = 0;
            pCoords[n].yEqu = 0;
            pCoords[n].xAna = 0;
            pCoords[n].yAna = 0;

            pEqu = pEqu->Next;
            n++;
        }

        strcpy(FileName, pCurrentDisplay->Substation->Spens);
        strcat(FileName, ".POS");

        File = fopen(FileName, "r");
        if (File == NULL)
        {
            File = fopen("CADEC.POS", "r");
            if (File == NULL)
            {
                DisplayError("FOREGROUND FILE NOT FOUND");
                return;
            }
        }

        while ((fgets(TextLine, 95, File)) != NULL)
        {
            if (TextLine[0] == '#')
                continue;

            if (sscanf(TextLine, "EQU %2s %d %d %d %d", &Spens,
                       &xEqu, &yEqu, &xAna, &yAna) != 5)
                continue;

            for (n = 0; n != nCoords; n++)
                if (strcmp(Spens, pCoords[n].Equipment->Spens) == 0)
                    break;

            if (n == nCoords)
                continue;

            pCoords[n].xEqu = xEqu;
            pCoords[n].yEqu = yEqu;
            pCoords[n].xAna = xAna;
            pCoords[n].yAna = yAna;
        }

        fclose(File);

        ClearCommand();
    }

    for (n = 0; n != nCoords; n++)
    {
        if ((pCoords[n].xEqu == 0) && (pCoords[n].yEqu == 0))
            continue;

        if (strcmp(pCoords[n].Equipment->Spens, "00") == 0)
        {
            gotoxy(pCoords[n].xEqu, pCoords[n].yEqu + 8);
            SetColours(YELLOW, BLACK);
            cputs(pCurrentDisplay->Substation->Spens);
            continue;
        }

        gotoxy(pCoords[n].xEqu, pCoords[n].yEqu + 8);
        SetColours(RED, BLACK);

        if ((pCoords[n].Equipment->Status & (StatusOpen | StatusClosed))
            == StatusOpen)
            cputs(" ");

        if ((pCoords[n].Equipment->Status & (StatusOpen | StatusClosed))
            == StatusClosed)
            cputs("л");

        SetColours(YELLOW, BLACK);
        cputs(pCoords[n].Equipment->Spens);

        if ((pCoords[n].Equipment->Analog[0] < 0) ||
            ((pCoords[n].xAna == 0) && (pCoords[n].yAna == 0)))
            continue;

        gotoxy(pCoords[n].xAna, pCoords[n].yAna + 8);
        if (pCoords[n].Equipment->Spens[0] == 'T')
        {
            pTrans = pCoords[n].Equipment->Trans;

            if ((pTrans->Status & (StatusOpen | StatusClosed))
                != StatusOpen)
            {
                SetColours(BLACK, GREEN);
                cprintf("%2.2f", pCoords[n].Equipment->Analog[0]
                        / 100.0);
            }

            else
            {
                SetColours(BLACK, WHITE);
                cprintf("%2.2f", 0.0);
            }
        }

        else
        {
            if ((pCoords[n].Equipment->Status
                 & (StatusOpen | StatusClosed)) != StatusOpen)
            {
                SetColours(BLACK, GREEN);
                cprintf("%5d", pCoords[n].Equipment->Analog[1]);
            }

            else
            {
                SetColours(BLACK, WHITE);
                cprintf("%5d", 0);
            }
        }
    }
}

/*****************************************************************************/

void EditDrawing()
{
    int n;

    if ((CommandNumber != 0) ||
        ((pCurrentDisplay->Display != Substat) &&
         (pCurrentDisplay->Display != Drawing)))
    {
        for (n = 0; n < nSubstation; n++)
        {
            if (strcmp(CommandWords[1], pSubstation[n].Spens) == 0)
                break;
        }

        if (n == nSubstation)
        {
            DisplayError("SUBSTATION NOT FOUND");
            return;
        }

        if (pSubstation[n].Equipment == NULL)
        {
            DisplayError("SUBSTATION HAS NO EQUIPMENT");
            return;
        }

        pCurrentDisplay->Substation = &pSubstation[n];
    }

    pCurrentDisplay->Equipment = pCurrentDisplay->Substation->Equipment;
    pCurrentDisplay->Function = Init;
    pCurrentDisplay->Display = Drawing;

    DrawingDisplay();
    DrawingEdit(1, 9);
}

/*****************************************************************************/

void DrawingEdit(int x, int y)
{
    char *Equ;
    time_t s, t;
    int c;

    s = time(NULL);
    DisplayTime(s);

    gotoxy(x, y);
    _setcursortype(_SOLIDCURSOR);
    SetColours(WHITE, BLACK);

    for (;;)
    {
        if (kbhit())
        {
            TimeOut = 0;
            Dingdong = 0;

            if ((c = getch()) == '\0')
            {
                switch (c = getch())
                {
                case VK_HOME:
                    x = 1;
                    y = 9;
                    ShowPosition(x, y);
                    break;
                case VK_UP:
                    if (--y == 8)
                        y = 45;
                    ShowPosition(x, y);
                    break;
                case VK_LEFT:
                    if (--x == 0)
                        x = 80;
                    ShowPosition(x, y);
                    break;
                case VK_RIGHT:
                    if (++x == 81)
                        x = 1;
                    ShowPosition(x, y);
                    break;
                case VK_DOWN:
                    if (++y == 46)
                        y = 9;
                    ShowPosition(x, y);
                    break;
                case VK_F10 + VK_SHIFT:
                    Equ = GetScreenStr(x, y);

                    ClearCommand();
                    gotoxy(1, 1);
                    _setcursortype(_NOCURSOR);
                    SetColours(WHITE, BLACK);
                    cputs(Equ);
                    cputs("<EQUIP>");
                    sleep(1);

                    UpdateEquipment(Equ, x, y);
                    ShowPosition(x, y);
                    break;
                case VK_F11 + VK_SHIFT:
                    Equ = GetScreenStr(x, y);

                    ClearCommand();
                    gotoxy(1, 1);
                    _setcursortype(_NOCURSOR);
                    SetColours(WHITE, BLACK);
                    cputs(Equ);
                    cputs("<ANALOG>");
                    sleep(1);

                    UpdateAnalog(Equ, x, y);
                    ShowPosition(x, y);
                    break;
                default:
#ifdef TESTING
                    gotoxy(1, 2);
                    SetColours(WHITE, BLACK);
                    cprintf("%d", c);
                    clreol();
#endif
                    gotoxy(x, y);
                    break;
                }
            }

            else
            {
                if (c == VK_RETURN)
                {
                    ClearCommand();
                    gotoxy(1, 1);
                    _setcursortype(_NOCURSOR);
                    SetColours(WHITE, BLACK);
                    cputs("<SAVE>");
                    sleep(1);

                    SaveDrawing();

                    pCurrentDisplay->Function = Init;
                    DisplayRefresh();
                    return;
                }

                else if (c == VK_BACK)
                {
                    SetColours(WHITE, BLACK);
                    putch(c);

                    if (--x == 0)
                        x = 80;

                    ShowPosition(x, y);
                }

                else if (c == VK_ESCAPE)
                {
                    ClearCommand();
                    gotoxy(1, 1);
                    _setcursortype(_NOCURSOR);
                    SetColours(WHITE, BLACK);
                    cputs("<CANCEL>");
                    sleep(1);

                    gotoxy(1, 1);
                    SetColours(CYAN, BLACK);
                    cputs("DO YOU WANT TO DISCARD THIS DRAWING? ");

                    _setcursortype(_SOLIDCURSOR);
                    SetColours(WHITE, BLACK);

                    if (DisplayConfirm())
                    {
                        sleep(2);
                        pCurrentDisplay->Function = Init;
                        DisplayRefresh();
                        return;
                    }

                    else
                    {
                        sleep(2);
                        ClearCommand();
                        ShowPosition(x, y);
                    }

                }
  
                else if (!iscntrl(c))
                {
                    SetColours(WHITE, BLACK);
                    putch(toupper(c));

                    if (++x ==81)
                        x = 1;

                    ShowPosition(x, y);
                }
            }
        }

        t = time(NULL);
        if (s != t)
        {
            s = t;
            ProcessTime(t);
        }
    }
}

/*****************************************************************************/

void UpdateEquipment(char *Equ, int x, int y)
{
    int i;

    for (i = 0; i != nCoords; i++)
    {
        if (strcmp(Equ, pCoords[i].Equipment->Spens) == 0)
            break;
    }

    if (i == nCoords)
    {
        DisplayError("EQUIPMENT NOT FOUND");
        return;
    }

    pCoords[i].xEqu = x;
    pCoords[i].yEqu = y - 8;

    sleep(2);

    ClearCommand();
    DisplayRefresh();
}

/*****************************************************************************/

void UpdateAnalog(char *Equ, int x, int y)
{
    int i;

    for (i = 0; i != nCoords; i++)
    {
        if (strcmp(Equ, pCoords[i].Equipment->Spens) == 0)
            break;
    }

    if (i == nCoords)
    {
        DisplayError("EQUIPMENT NOT FOUND");
        return;
    }

    pCoords[i].xAna = x;
    pCoords[i].yAna = y - 8;

    sleep(2);

    ClearCommand();
    DisplayRefresh();
}

/*****************************************************************************/

void SaveDrawing()
{
    FILE *File;
    int c, i, x, y,
        lastgraph;
    char FileName[16],
        Textline[96];

    gotoxy(1, 1);
    SetColours(CYAN, BLACK);
    cputs("DO YOU WANT TO SAVE THIS DRAWING? ");

    _setcursortype(_SOLIDCURSOR);
    SetColours(WHITE, BLACK);
    if (DisplayConfirm())
    {
        strcpy(FileName, pCurrentDisplay->Substation->Spens);
        strcat(FileName, ".DRW");

        File = fopen(FileName, "w");
        if (File == NULL)
        {
            DisplayError("BACKGROUND FILE NOT FOUND");
            pCurrentDisplay->Function = Init;
            DisplayRefresh();
            return;
        }

        for (y = 9; y != 46; y++)
        {
            lastgraph = 0;

            for (x = 1; x != 81; x++)
            {
                gotoxy(x, y);
                c = GetScreenChar();

                Textline[x - 1] = c;

                if (c != ' ')
                    lastgraph = x;
            }

            Textline[lastgraph] = '\0';

            fprintf(File, "%s\n", Textline);
        }

        fclose(File);

        strcpy(FileName, pCurrentDisplay->Substation->Spens);
        strcat(FileName, ".POS");

        File = fopen(FileName, "w");
        if (File == NULL)
        {
            DisplayError("FOREGROUND FILE NOT FOUND");
            pCurrentDisplay->Function = Init;
            DisplayRefresh();
            return;
        }

        fprintf(File, "#\tDRAWING POSITIONS %s\n#\n",
                pCurrentDisplay->Substation->Spens);

        for (i = 0; i != nCoords; i++)
            if (pCoords[i].Equipment != NULL)
                fprintf(File, "EQU\t%s\t%d\t%d\t%d\t%d\n",
                        pCoords[i].Equipment->Spens,
                        pCoords[i].xEqu,
                        pCoords[i].yEqu,
                        pCoords[i].xAna,
                        pCoords[i].yAna);

        fclose(File);

        sleep(2);
    }
}

/*****************************************************************************/

void DisplayData()
{
    int  n;
    Equipment *pEqu;
    char  *pSpens;

    if ((CommandNumber == 2) ||
        ((pCurrentDisplay->Display != Substat) &&
         (pCurrentDisplay->Display != Data)))
    {
        for (n = 0; n < nSubstation; n++)
        {
            if (strcmp(CommandWords[1], pSubstation[n].Spens) == 0)
                break;
        }

        if (n == nSubstation)
        {
            DisplayError("SUBSTATION NOT FOUND");
            return;
        }

        if (pSubstation[n].Equipment == NULL)
        {
            DisplayError("SUBSTATION HAS NO EQUIPMENT");
            return;
        }

        pSpens = CommandWords[2];
        pCurrentDisplay->Substation = &pSubstation[n];
    }

    else
        pSpens = CommandWords[1];

    pEqu = pCurrentDisplay->Substation->Equipment;

    while (pEqu != NULL)
    {
        if (strcmp(pSpens, pEqu->Spens) == 0)
            break;

        pEqu = pEqu->Next;
    }

    if (pEqu == NULL)
    {
        DisplayError("EQUIPMENT NOT FOUND");
        return;
    }

    pCurrentDisplay->Function = Init;
    pCurrentDisplay->Display = Data;
    pCurrentDisplay->Equipment = pEqu;

    DataDisplay();
}

/*****************************************************************************/

void DataDisplay()
{
    switch(pCurrentDisplay->Function)
    {
    case Init:
        DataOne();
        break;

    case Pageon:
        switch (pCurrentDisplay->Page)
        {
        case 1:
            DataTwo();
            break;

        case 2:
            DisplayError("INVALID PAGING");
            break;
        }
        break;

    case Pageback:
        switch (pCurrentDisplay->Page)
        {
        case 1:
            DisplayError("INVALID PAGING");
            break;

        case 2:
            DataOne();
            break;
        }
        break;

    case Refresh:
        switch (pCurrentDisplay->Page)
        {
        case 1:
            DataOne();
            break;

        case 2:
            DataTwo();
            break;
        }
        break;
    }
}

/*****************************************************************************/

void DataOne()
{
    Concentrator *pConc;
    Outstation *pOstn;
    static int Conc, Ostn;
    int n;

    pCurrentDisplay->Page = 1;

    if (pCurrentDisplay->Function != Refresh)
    {
        DisplayBackground("DISPLAY.004");
        DisplayHeader("EQUIPMENT DATA");

        gotoxy(71, 7);
        SetColours(GREEN, BLACK);
        cprintf("PAGE %d", pCurrentDisplay->Page);

        cputs(" >>");

        Conc = 0;
        for (n = 0; n != nConcentrator; n++)
        {
            pConc = &pConcentrator[n];
            pOstn = pConc->Outstation;

            Ostn = (n + 1) * 10;
            while (pOstn != NULL)
            {
                if (strcmp(pCurrentDisplay->Equipment->Name,
                           pOstn->Name) == 0)
                {
                    Conc = n + 1;
                    break;
                }

                Ostn++;
                pOstn = pOstn->Next;
            }
            if (Conc != 0)
                break;
        }
    }

    gotoxy(18, 9);
    SetColours(YELLOW, BLACK);
    cputs(pCurrentDisplay->Substation->Spens);

    gotoxy(38, 9);
    cputs(pCurrentDisplay->Equipment->Spens);

    gotoxy(68, 9);
    cprintf("%d", Conc);

    gotoxy(29, 11);
    cputs(pCurrentDisplay->Equipment->Name);

    gotoxy(66, 11);
    cprintf("%3d", Ostn);

    gotoxy(19, 14);
    if (pCurrentDisplay->Substation->Equipment ==
        pCurrentDisplay->Equipment)
        cputs("1.02 PSEUDO EQUIPMENT, VOLTAGE REDUCTION");

    else if (pCurrentDisplay->Equipment->Spens[0] == 'T')
        cputs("2.04 TRANSFORMER, TAP CHANGE");

    else
    {
        switch(pCurrentDisplay->Equipment->Status &
               (StatusProtection | StatusAutomation))
        {
        case 0:
            cputs("3.02 CIRCUIT BREAKER, OPEN CLOSE");
            break;

        case StatusProtection:
            cputs("3.05 CIRCUIT BREAKER, OPEN CLOSE, EFI OFF ON");
            break;

        case StatusAutomation:
            cputs("3.06 CIRCUIT BREAKER, OPEN CLOSE, AUTO OFF ON");
            break;

        case StatusProtection | StatusAutomation:
            cputs("3.07 CIRCUIT BREAKER, OPEN CLOSE, EFI OFF ON, AUTO OFF ON");
            break;
        }
    }

    gotoxy(29, 16);
    cputs("ON");

    gotoxy(29, 20);
    cputs("N/A");

    gotoxy(29, 21);
    cputs("OFF");

    gotoxy(29, 22);
    cputs("ON");

    gotoxy(29, 23);
    cputs("ON");

    gotoxy(29, 24);
    cputs("N/A");

    gotoxy(29, 25);
    cputs("OFF");

    if (pCurrentDisplay->Substation->Equipment ==
        pCurrentDisplay->Equipment)
    {
        gotoxy(4, 32);
        SetColours(YELLOW, BLACK);
        cputs("1 PHASE   60.0  -60.0");

        gotoxy(48, 32);
        SetColours(GREEN, BLACK);
        cprintf("%2d.0     %2d.0",
                pCurrentDisplay->Equipment->Analog[1],
                pCurrentDisplay->Equipment->Analog[0]);

        gotoxy(4, 33);
        SetColours(YELLOW, BLACK);
        cputs("2 FREQ    55.0   45.0");

        gotoxy(30, 33);
        cputs("50.50");

        gotoxy(39, 33);
        cputs("48.80");

        gotoxy(48, 33);
        SetColours(GREEN, BLACK);
        cprintf("%2d.%02d    %2d.%02d", Frequency / 100, Frequency % 100,
                Frequency / 100, Frequency % 100);
    }

    else if (pCurrentDisplay->Equipment->Spens[0] == 'T')
    {
        gotoxy(4, 32);
        SetColours(YELLOW, BLACK);
        cputs("1 KV      14.0    0.0");

        gotoxy(30, 32);
        cputs("11.80");

        gotoxy(39, 32);
        cputs("10.80");

        gotoxy(48, 32);
        SetColours(GREEN, BLACK);
        cprintf("%2.2f    %2.2f",
                pCurrentDisplay->Equipment->Analog[1] / 100.0,
                pCurrentDisplay->Equipment->Analog[0] / 100.0);

        gotoxy(4, 33);
        SetColours(YELLOW, BLACK);
        cputs("2 TPI     18.0    0.0");

        gotoxy(48, 33);
        SetColours(GREEN, BLACK);
        cprintf("%2d.0     %2d.0", pCurrentDisplay->Equipment->Operns,
                pCurrentDisplay->Equipment->Operns);

    }

    else 
    {
        if (pCurrentDisplay->Equipment->Analog[0] >= 0)
        {
            SetColours(YELLOW, BLACK);

            if (pCurrentDisplay->Equipment->Spens[0] == '5')
            {
                gotoxy(4, 32);
                cputs("1 AMPS  1200.0    0.0");

                gotoxy(29, 32);
                cputs("1000.0");
            }

            else
            {
                gotoxy(4, 32);
                cputs("1 AMPS   400.0    0.0");

                gotoxy(30, 32);
                cputs("300.0");
            }

            gotoxy(47, 32);
            SetColours(GREEN, BLACK);
            cprintf("%4d.0  %4d.0",
                    pCurrentDisplay->Equipment->Analog[1],
                    pCurrentDisplay->Equipment->Analog[0]);
        }
    }
}

/*****************************************************************************/

void DataTwo()
{
    pCurrentDisplay->Page = 2;

    if (pCurrentDisplay->Function != Refresh)
    {
        DisplayBackground("DISPLAY.005");
        DisplayHeader("EQUIPMENT DATA");

        gotoxy(71, 7);
        SetColours(GREEN, BLACK);
        cprintf("PAGE %d", pCurrentDisplay->Page);
    }

    gotoxy(17, 9);
    SetColours(YELLOW, BLACK);
    cputs(pCurrentDisplay->Substation->Spens);

    gotoxy(38, 9);
    cputs(pCurrentDisplay->Equipment->Spens);

    if (pCurrentDisplay->Equipment ==
        pCurrentDisplay->Substation->Equipment)
    {
        if (pCurrentDisplay->Equipment->Status & StatusProtAlarm)
            SetColours(BLACK, RED);
        else
            SetColours(GREEN, BLACK);

        gotoxy(5, 11);
        cputs("FEEDER PROTECTION OP         FPO");

        gotoxy(39, 11);
        SetColours(GREEN, BLACK);
        cputs("H");

        gotoxy(5, 12);
        cputs("TRANSFORMER PROTECTION OP    TPO");
        gotoxy(39, 12);
        cputs("H");

        gotoxy(5, 13);
        cputs("BATTERY VOLTS LOW            BVL");

        gotoxy(39, 13);
        cputs("H");

        gotoxy(5, 14);
        cputs("BATTERY VOLTS HIGH           BVH");

        gotoxy(39, 14);
        cputs("L");

        gotoxy(5, 15);
        cputs("BATTERY CHARGER FAIL         BCF");

        gotoxy(39, 15);
        cputs("L");

        gotoxy(5, 16);
        cputs("AUXILIARY SUPPLY FAIL        ASF");

        gotoxy(39, 16);
        cputs("L");

        gotoxy(5, 35);
        cputs("OUTSTN ANALOGUES FAIL        OAF");

        gotoxy(39, 35);
        cputs("L");

        gotoxy(5, 36);
        cputs("OUTSTN DIGITALS FAIL         ODF");

        gotoxy(39, 36);
        cputs("L");

        gotoxy(5, 37);
        cputs("COMMUNICATION FAILURE        COM");

        gotoxy(39, 37);
        cputs("L");

        gotoxy(48, 11);
        cputs("VOLT REDUCTION STAGE 1       VR1");

        gotoxy(48, 12);
        cputs("VOLT REDUCTION STAGE 2       VR2");

        gotoxy(48, 43);
        SetColours(BLACK, RED);
        cputs("VOLTAGE REDUCTION NORMAL     VRN");

        gotoxy(48, 44);
        SetColours(GREEN, BLACK);
        cputs("VOLTAGE REDUCTION DBI        DBI");
    }

    else if (pCurrentDisplay->Equipment->Spens[0] == 'T')
    {
        gotoxy(5, 13);
        SetColours(GREEN, BLACK);
        cputs("WINDING TEMP                 WTI");

        gotoxy(39, 13);
        cputs("H");

        gotoxy(5, 15);
        cputs("AVC FAILURE                  AVC");

        gotoxy(39, 15);
        cputs("L");

        gotoxy(5, 16);
        cputs("BUCHHOLZ GAS                 BUG");

        gotoxy(39, 16);
        cputs("H");

        gotoxy(48, 11);
        SetColours(BLACK, RED);
        cputs("PARAL AUTO                      ");

        gotoxy(48, 12);
        SetColours(GREEN, BLACK);
        cputs("PARAL MANUAL");

        gotoxy(48, 13);
        cputs("INDEP AUTO");

        gotoxy(48, 14);
        cputs("INDEP MANUAL");

        gotoxy(48, 15);
        cputs("LOCAL CONTROL SELECTED");
    }

    else
    {
        if (pCurrentDisplay->Equipment->Status & StatusTripped)
            SetColours(BLACK, RED);
        else
            SetColours(GREEN, BLACK);

        gotoxy(5, 29);
        cputs("C.B. TRIPPED                 TRP");

        SetColours(GREEN, BLACK);
        gotoxy(39, 29);
        cputs("H");

        gotoxy(5, 34);
        cputs("C.B. MAINTENANCE             MNT");

        gotoxy(39, 34);
        cputs("L");

        gotoxy(5, 35);
        cputs("OPEN STATE WARNING           OSW");

        gotoxy(39, 35);
        cputs("L");

        gotoxy(5, 38);
        cputs("CLOSED STATE WARNING         CSW");

        gotoxy(39, 38);
        cputs("L");

        if (pCurrentDisplay->Equipment->Status & StatusOpen)
            SetColours(BLACK, RED);
        else
            SetColours(GREEN, BLACK);

        gotoxy(48, 11);
        cputs("OPEN                            ");

        if (pCurrentDisplay->Equipment->Status & StatusClosed)
            SetColours(BLACK, RED);
        else
            SetColours(GREEN, BLACK);

        gotoxy(48, 12);
        cputs("CLOSED                          ");

        gotoxy(48, 13);
        SetColours(GREEN, BLACK);
        cputs("LOCAL (ALL CONTROLS)");

        if (pCurrentDisplay->Equipment->Status & StatusExternal)
            SetColours(BLACK, RED);
        else
            SetColours(GREEN, BLACK);

        gotoxy(48, 35);
        cputs("EXTERNAL OUTPUT                 ");

        gotoxy(48, 39);
        SetColours(GREEN, BLACK);
        cputs("SPONTANEOUS CLOSE");

        gotoxy(48, 43);
        cputs("DISCREPANCY");

        gotoxy(48, 44);
        cputs("DBI");
    }

    if (pCurrentDisplay->Equipment->Status & StatusTrippable)
    {
        gotoxy(5, 11);
        SetColours(GREEN, BLACK);
        cputs("TRIP CIRCUIT FAULTY          TCF");

        gotoxy(39, 11);
        cputs("H");
    }

    if (pCurrentDisplay->Equipment->Status & StatusAutomation)
    {
        if (pCurrentDisplay->Equipment->Status & StatusLockout)
            SetColours(BLACK, RED);
        else
            SetColours(GREEN, BLACK);

        gotoxy(5, 13);
        cputs("A.R. LOCKED OUT              LCK");

        SetColours(GREEN, BLACK);
        gotoxy(39, 13);
        cputs("H");

        if (pCurrentDisplay->Equipment->Status & StatusAutoOff)
            SetColours(BLACK, RED);
        else
            SetColours(GREEN, BLACK);

        gotoxy(48, 14);
        cputs("AUTO RECLOSE OFF                ");
        if (pCurrentDisplay->Equipment->Status & StatusAutoOn)
            SetColours(BLACK, RED);
        else
            SetColours(GREEN, BLACK);

        gotoxy(48, 15);
        cputs("AUTO RECLOSE ON                 ");
    }

    if (pCurrentDisplay->Equipment->Status & StatusProtection)
    {
        if (pCurrentDisplay->Equipment->Status & StatusEfiAlarm)
            SetColours(BLACK, RED);
        else
            SetColours(GREEN, BLACK);

        gotoxy(5, 12);
        cputs("EARTH FAULT INDICATION       EFI");

        SetColours(GREEN, BLACK);
        gotoxy(39, 12);
        cputs("H");

        if (pCurrentDisplay->Equipment->Status & StatusProtOff)
            SetColours(BLACK, RED);
        else
            SetColours(GREEN, BLACK);

        gotoxy(48, 16);
        cputs("SEF OFF                         ");

        if (pCurrentDisplay->Equipment->Status & StatusProtOn)
            SetColours(BLACK, RED);
        else
            SetColours(GREEN, BLACK);

        gotoxy(48, 17);
        cputs("SEF ON                          ");
    }
}

/*****************************************************************************/

void DisplayPageon()
{
    switch(pCurrentDisplay->Display)
    {
    case Index:
        pCurrentDisplay->Function = Pageon;
        IndexDisplay();
        break;

    case Substat:
        pCurrentDisplay->Function = Pageon;
        SubstationDisplay();
        break;

    case Data:
        pCurrentDisplay->Function = Pageon;
        DataDisplay();
        break;

    case Events:
        pCurrentDisplay->Function = Pageon;
        EventsDisplay();
        break;

    case Alarms:
        pCurrentDisplay->Function = Pageon;
        AlarmsDisplay();
        break;

    default:
        DisplayError("INVALID PAGING");
        return;
    }

    SetColours(WHITE, BLACK);
}

/*****************************************************************************/

void DisplayPageback()
{
    switch(pCurrentDisplay->Display)
    {
    case Index:
        pCurrentDisplay->Function = Pageback;
        IndexDisplay();
        break;

    case Substat:
        pCurrentDisplay->Function = Pageback;
        SubstationDisplay();
        break;

    case Data:
        pCurrentDisplay->Function = Pageback;
        DataDisplay();
        break;

    case Events:
        pCurrentDisplay->Function = Pageback;
        EventsDisplay();
        break;

    case Alarms:
        pCurrentDisplay->Function = Pageback;
        AlarmsDisplay();
        break;

    default:
        DisplayError("INVALID PAGING");
        return;
    }

    SetColours(WHITE, BLACK);
}

/*****************************************************************************/

void DisplayRefresh()
{
    RefreshDisplay();
    ClearCommand();
}

/*****************************************************************************/

void RefreshDisplay()
{
    int x, y;

    x = wherex();
    y = wherey();

    switch(pCurrentDisplay->Display)
    {
    case Substat:
        pCurrentDisplay->Function = Refresh;
        SubstationDisplay();
        break;

    case Data:
        pCurrentDisplay->Function = Refresh;
        DataDisplay();
        break;

    case Drawing:
        pCurrentDisplay->Function = Refresh;
        DrawingDisplay();
        break;

    case Events:
        pCurrentDisplay->Function = Refresh;
        EventsDisplay();
        break;

    case Alarms:
        pCurrentDisplay->Function = Refresh;
        AlarmsDisplay();
        break;
    }

    SetColours(WHITE, BLACK);
    gotoxy(x, y);
}

/*****************************************************************************/

void DisplayOpen()
{
    int  n;
    Equipment *pEqu;
    char  *pSpens;

    if (pCurrentDisplay->Display != Substat)
    {
        DisplayError("SELECT A SUBSTATION FIRST");
        return;
    }

    pSpens = CommandWords[1];

    switch (pCurrentDisplay->Page)
    {
    case 1:
        DisplayError("OPERATION NOT PERMITTED ON EQUIPMENT");
        return;

    case 2:
        pEqu = pCurrentDisplay->Equipment->Next;

        while (pEqu != NULL)
        {
            if (pEqu->Spens[0] != 'T')
                break;

            pEqu = pEqu->Next;
        }

        n = 0;

        while (pEqu != NULL)
        {
            if (pEqu->Spens[0] == '0')
            {
                DisplayError("EQUIPMENT NOT FOUND");
                return;
            }

            if (strcmp(pSpens, pEqu->Spens) == 0)
                break;

            pEqu = pEqu->Next;
            n++;
        }

        if ((pEqu->Status & (StatusOpen | StatusClosed)) == StatusOpen)
        {
            DisplayError("EQUIPMENT IN DESIRED STATE");
            return;
        }

        gotoxy(2, 31 + n);
        SetColours(YELLOW | BLINK, BLACK);
        cprintf("%s", pEqu->Spens);
        break;

    case 3:
        pEqu = pCurrentDisplay->Equipment->Next;

        while (pEqu != NULL)
        {
            if (pEqu->Spens[0] == '0')
                break;

            pEqu = pEqu->Next;
        }

        n = 0;

        while (pEqu != NULL)
        {
            if (strcmp(pSpens, pEqu->Spens) == 0)
                break;

            pEqu = pEqu->Next;
            n++;
        }

        if (pEqu == NULL)
        {
            DisplayError("EQUIPMENT NOT FOUND");
            return;
        }

        if ((pEqu->Status & (StatusOpen | StatusClosed)) == StatusOpen)
        {
            DisplayError("EQUIPMENT IN DESIRED STATE");
            return;
        }

        gotoxy(2, 14 + (n * 2));
        SetColours(YELLOW | BLINK, BLACK);
        cprintf("%s", pEqu->Spens);
        break;
    }

    gotoxy(1, 2);
    SetColours(GREEN, BLACK);
    cputs("CONFIRM OR CANCEL WITHIN 30 SECS");

    gotoxy(1, 1);
    SetColours(CYAN, BLACK);
    cprintf("%s %s   %s OPEN ", pCurrentDisplay->Substation->Spens,
            pEqu->Spens, pEqu->Name);

    if (DisplayConfirm())
    {
        pEqu->Status |= StatusOpen;
        pEqu->Status &= ~StatusClosed;

        if (pEqu->Status & StatusExternal)
        {
            ExternalOperation(OPEN);
            sleep(PULSETIME);
            ExternalOperation(NORMAL);
        }

        else
            sleep(2);

        LogEvent("P1", pCurrentDisplay->Substation, pEqu,
                 "OPEN       SUCCESSFUL");
    }

    else
        sleep(2);

    DisplayRefresh();
}

/*****************************************************************************/

void DisplayClose()
{
    int  n;
    Equipment *pEqu;
    char  *pSpens;

    if (pCurrentDisplay->Display != Substat)
    {
        DisplayError("SELECT A SUBSTATION FIRST");
        return;
    }

    pSpens = CommandWords[1];

    switch (pCurrentDisplay->Page)
    {
    case 1:
        DisplayError("OPERATION NOT PERMITTED ON EQUIPMENT");
        return;

    case 2:
        pEqu = pCurrentDisplay->Equipment->Next;

        while (pEqu != NULL)
        {
            if (pEqu->Spens[0] != 'T')
                break;

            pEqu = pEqu->Next;
        }

        n = 0;

        while (pEqu != NULL)
        {
            if (pEqu->Spens[0] == '0')
            {
                DisplayError("EQUIPMENT NOT FOUND");
                return;
            }

            if (strcmp(pSpens, pEqu->Spens) == 0)
                break;

            pEqu = pEqu->Next;
            n++;
        }

        if ((pEqu->Status & (StatusOpen | StatusClosed)) == StatusClosed)
        {
            DisplayError("EQUIPMENT IN DESIRED STATE");
            return;
        }

        gotoxy(2, 31 + n);
        SetColours(YELLOW | BLINK, BLACK);
        cprintf("%s", pEqu->Spens);
        break;

    case 3:
        pEqu = pCurrentDisplay->Equipment->Next;

        while (pEqu != NULL)
        {
            if (pEqu->Spens[0] == '0')
                break;

            pEqu = pEqu->Next;
        }

        n = 0;

        while (pEqu != NULL)
        {
            if (strcmp(pSpens, pEqu->Spens) == 0)
                break;

            pEqu = pEqu->Next;
            n++;
        }

        if (pEqu == NULL)
        {
            DisplayError("EQUIPMENT NOT FOUND");
            return;
        }

        if ((pEqu->Status & (StatusOpen | StatusClosed)) == StatusClosed)
        {
            DisplayError("EQUIPMENT IN DESIRED STATE");
            return;
        }

        gotoxy(2, 14 + (n * 2));
        SetColours(YELLOW | BLINK, BLACK);
        cprintf("%s", pEqu->Spens);
        break;
    }

    gotoxy(1, 2);
    SetColours(GREEN, BLACK);
    cputs("CONFIRM OR CANCEL WITHIN 30 SECS");

    gotoxy(1, 1);
    SetColours(CYAN, BLACK);
    cprintf("%s %s   %s CLOSE ", pCurrentDisplay->Substation->Spens,
            pEqu->Spens, pEqu->Name);

    if (DisplayConfirm())
    {
        pEqu->Status |= StatusClosed;
        pEqu->Status &= ~StatusOpen;

        if (pEqu->Status & StatusExternal)
        {
            ExternalOperation(CLOSE);
            sleep(PULSETIME);
            ExternalOperation(NORMAL);
        }

        else
            sleep(2);

        LogEvent("P1", pCurrentDisplay->Substation, pEqu,
                 "CLOSE      SUCCESSFUL");
    }

    else
        sleep(2);

    DisplayRefresh();
}

/*****************************************************************************/

void DisplayAutOff()
{
    int  n;
    Equipment *pEqu;
    char  *pSpens;

    if (pCurrentDisplay->Display != Substat)
    {
        DisplayError("SELECT A SUBSTATION FIRST");
        return;
    }

    pSpens = CommandWords[1];

    switch (pCurrentDisplay->Page)
    {
    case 1:
        DisplayError("OPERATION NOT PERMITTED ON EQUIPMENT");
        return;

    case 2:
        pEqu = pCurrentDisplay->Equipment->Next;

        while (pEqu != NULL)
        {
            if (pEqu->Spens[0] != 'T')
                break;

            pEqu = pEqu->Next;
        }

        n = 0;

        while (pEqu != NULL)
        {
            if (pEqu->Spens[0] == '0')
            {
                DisplayError("EQUIPMENT NOT FOUND");
                return;
            }

            if (strcmp(pSpens, pEqu->Spens) == 0)
                break;

            pEqu = pEqu->Next;
            n++;
        }

        if (!(pEqu->Status & StatusAutomation))
        {
            DisplayError("OPERATION NOT PERMITTED ON EQUIPMENT");
            return;
        }

        if ((pEqu->Status & (StatusAutoOff | StatusAutoOn)) == StatusAutoOff)
        {
            DisplayError("EQUIPMENT IN DESIRED STATE");
            return;
        }

        gotoxy(2, 31 + n);
        SetColours(YELLOW | BLINK, BLACK);
        cprintf("%s", pEqu->Spens);
        break;

    case 3:
        pEqu = pCurrentDisplay->Equipment->Next;

        while (pEqu != NULL)
        {
            if (pEqu->Spens[0] == '0')
                break;

            pEqu = pEqu->Next;
        }

        n = 0;

        while (pEqu != NULL)
        {
            if (strcmp(pSpens, pEqu->Spens) == 0)
                break;

            pEqu = pEqu->Next;
            n++;
        }

        if (pEqu == NULL)
        {
            DisplayError("EQUIPMENT NOT FOUND");
            return;
        }

        if (!(pEqu->Status & StatusAutomation))
        {
            DisplayError("OPERATION NOT PERMITTED ON EQUIPMENT");
            return;
        }

        if ((pEqu->Status & (StatusAutoOff | StatusAutoOn)) == StatusAutoOff)
        {
            DisplayError("EQUIPMENT IN DESIRED STATE");
            return;
        }

        gotoxy(2, 14 + (n * 2));
        SetColours(YELLOW | BLINK, BLACK);
        cprintf("%s", pEqu->Spens);
        break;
    }


    gotoxy(1, 2);
    SetColours(GREEN, BLACK);
    cputs("CONFIRM OR CANCEL WITHIN 30 SECS");

    gotoxy(1, 1);
    SetColours(CYAN, BLACK);
    cprintf("%s %s   %s AUTO OFF ", pCurrentDisplay->Substation->Spens,
            pEqu->Spens, pEqu->Name);

    if (DisplayConfirm())
    {
        pEqu->Status |= StatusAutoOff;
        pEqu->Status &= ~StatusAutoOn;

        LogEvent("P1", pCurrentDisplay->Substation, pEqu,
                 "AUTO OFF   SUCCESSFUL");
    }

    sleep(2);
    DisplayRefresh();
}

/*****************************************************************************/

void DisplayAutOn()
{
    int  n;
    Equipment *pEqu;
    char  *pSpens;

    if (pCurrentDisplay->Display != Substat)
    {
        DisplayError("SELECT A SUBSTATION FIRST");
        return;
    }

    pSpens = CommandWords[1];

    switch (pCurrentDisplay->Page)
    {
    case 1:
        DisplayError("OPERATION NOT PERMITTED ON EQUIPMENT");
        return;

    case 2:
        pEqu = pCurrentDisplay->Equipment->Next;

        while (pEqu != NULL)
        {
            if (pEqu->Spens[0] != 'T')
                break;

            pEqu = pEqu->Next;
        }

        n = 0;

        while (pEqu != NULL)
        {
            if (pEqu->Spens[0] == '0')
            {
                DisplayError("EQUIPMENT NOT FOUND");
                return;
            }

            if (strcmp(pSpens, pEqu->Spens) == 0)
                break;

            pEqu = pEqu->Next;
            n++;
        }

        if (!(pEqu->Status & StatusAutomation))
        {
            DisplayError("OPERATION NOT PERMITTED ON EQUIPMENT");
            return;
        }

        if ((pEqu->Status & (StatusAutoOff | StatusAutoOn)) == StatusAutoOn)
        {
            DisplayError("EQUIPMENT IN DESIRED STATE");
            return;
        }

        gotoxy(2, 31 + n);
        SetColours(YELLOW | BLINK, BLACK);
        cprintf("%s", pEqu->Spens);
        break;

    case 3:
        pEqu = pCurrentDisplay->Equipment->Next;

        while (pEqu != NULL)
        {
            if (pEqu->Spens[0] == '0')
                break;

            pEqu = pEqu->Next;
        }

        n = 0;

        while (pEqu != NULL)
        {
            if (strcmp(pSpens, pEqu->Spens) == 0)
                break;

            pEqu = pEqu->Next;
            n++;
        }

        if (pEqu == NULL)
        {
            DisplayError("EQUIPMENT NOT FOUND");
            return;
        }

        if (!(pEqu->Status & StatusAutomation))
        {
            DisplayError("OPERATION NOT PERMITTED ON EQUIPMENT");
            return;
        }

        if ((pEqu->Status & (StatusAutoOff | StatusAutoOn)) == StatusAutoOn)
        {
            DisplayError("EQUIPMENT IN DESIRED STATE");
            return;
        }

        gotoxy(2, 14 + (n * 2));
        SetColours(YELLOW | BLINK, BLACK);
        cprintf("%s", pEqu->Spens);
        break;
    }

    gotoxy(1, 2);
    SetColours(GREEN, BLACK);
    cputs("CONFIRM OR CANCEL WITHIN 30 SECS");

    gotoxy(1, 1);
    SetColours(CYAN, BLACK);
    cprintf("%s %s   %s AUTO ON ", pCurrentDisplay->Substation->Spens,
            pEqu->Spens, pEqu->Name);

    if (DisplayConfirm())
    {
        pEqu->Status |= StatusAutoOn;
        pEqu->Status &= ~StatusAutoOff;

        LogEvent("P1", pCurrentDisplay->Substation, pEqu,
                 "AUTO ON    SUCCESSFUL");
    }

    sleep(2);
    DisplayRefresh();
}

/*****************************************************************************/

void DisplayProtOff()
{
    int  n;
    Equipment *pEqu;
    char  *pSpens;

    if (pCurrentDisplay->Display != Substat)
    {
        DisplayError("SELECT A SUBSTATION FIRST");
        return;
    }

    pSpens = CommandWords[1];

    switch (pCurrentDisplay->Page)
    {
    case 1:
        DisplayError("OPERATION NOT PERMITTED ON EQUIPMENT");
        return;

    case 2:
        pEqu = pCurrentDisplay->Equipment->Next;

        while (pEqu != NULL)
        {
            if (pEqu->Spens[0] != 'T')
                break;

            pEqu = pEqu->Next;
        }

        n = 0;

        while (pEqu != NULL)
        {
            if (pEqu->Spens[0] == '0')
            {
                DisplayError("EQUIPMENT NOT FOUND");
                return;
            }

            if (strcmp(pSpens, pEqu->Spens) == 0)
                break;

            pEqu = pEqu->Next;
            n++;
        }

        if (!(pEqu->Status & StatusProtection))
        {
            DisplayError("OPERATION NOT PERMITTED ON EQUIPMENT");
            return;
        }

        if ((pEqu->Status & (StatusProtOff | StatusProtOn)) == StatusProtOff)
        {
            DisplayError("EQUIPMENT IN DESIRED STATE");
            return;
        }

        gotoxy(2, 31 + n);
        SetColours(YELLOW | BLINK, BLACK);
        cprintf("%s", pEqu->Spens);
        break;

    case 3:
        pEqu = pCurrentDisplay->Equipment->Next;

        while (pEqu != NULL)
        {
            if (pEqu->Spens[0] == '0')
                break;

            pEqu = pEqu->Next;
        }

        n = 0;

        while (pEqu != NULL)
        {
            if (strcmp(pSpens, pEqu->Spens) == 0)
                break;

            pEqu = pEqu->Next;
            n++;
        }

        if (pEqu == NULL)
        {
            DisplayError("EQUIPMENT NOT FOUND");
            return;
        }

        if (!(pEqu->Status & StatusProtection))
        {
            DisplayError("OPERATION NOT PERMITTED ON EQUIPMENT");
            return;
        }

        if ((pEqu->Status & (StatusProtOff | StatusProtOn)) == StatusProtOff)
        {
            DisplayError("EQUIPMENT IN DESIRED STATE");
            return;
        }

        gotoxy(2, 14 + (n * 2));
        SetColours(YELLOW | BLINK, BLACK);
        cprintf("%s", pEqu->Spens);
        break;
    }


    gotoxy(1, 2);
    SetColours(GREEN, BLACK);
    cputs("CONFIRM OR CANCEL WITHIN 30 SECS");

    gotoxy(1, 1);
    SetColours(CYAN, BLACK);
    cprintf("%s %s   %s SEF OFF ", pCurrentDisplay->Substation->Spens,
            pEqu->Spens, pEqu->Name);

    if (DisplayConfirm())
    {
        pEqu->Status |= StatusProtOff;
        pEqu->Status &= ~StatusProtOn;

        LogEvent("P1", pCurrentDisplay->Substation, pEqu,
                 "SEF OFF    SUCCESSFUL");
    }

    sleep(2);
    DisplayRefresh();
}

/*****************************************************************************/

void DisplayProtOn()
{
    int  n;
    Equipment *pEqu;
    char  *pSpens;

    if (pCurrentDisplay->Display != Substat)
    {
        DisplayError("SELECT A SUBSTATION FIRST");
        return;
    }

    pSpens = CommandWords[1];

    switch (pCurrentDisplay->Page)
    {
    case 1:
        DisplayError("OPERATION NOT PERMITTED ON EQUIPMENT");
        return;

    case 2:
        pEqu = pCurrentDisplay->Equipment->Next;

        while (pEqu != NULL)
        {
            if (pEqu->Spens[0] != 'T')
                break;

            pEqu = pEqu->Next;
        }

        n = 0;

        while (pEqu != NULL)
        {
            if (pEqu->Spens[0] == '0')
            {
                DisplayError("EQUIPMENT NOT FOUND");
                return;
            }

            if (strcmp(pSpens, pEqu->Spens) == 0)
                break;

            pEqu = pEqu->Next;
            n++;
        }

        if (!(pEqu->Status & StatusProtection))
        {
            DisplayError("OPERATION NOT PERMITTED ON EQUIPMENT");
            return;
        }

        if ((pEqu->Status & (StatusProtOff | StatusProtOn)) == StatusProtOn)
        {
            DisplayError("EQUIPMENT IN DESIRED STATE");
            return;
        }

        gotoxy(2, 31 + n);
        SetColours(YELLOW | BLINK, BLACK);
        cprintf("%s", pEqu->Spens);
        break;

    case 3:
        pEqu = pCurrentDisplay->Equipment->Next;

        while (pEqu != NULL)
        {
            if (pEqu->Spens[0] == '0')
                break;

            pEqu = pEqu->Next;
        }

        n = 0;

        while (pEqu != NULL)
        {
            if (strcmp(pSpens, pEqu->Spens) == 0)
                break;

            pEqu = pEqu->Next;
            n++;
        }

        if (pEqu == NULL)
        {
            DisplayError("EQUIPMENT NOT FOUND");
            return;
        }

        if (!(pEqu->Status & StatusProtection))
        {
            DisplayError("OPERATION NOT PERMITTED ON EQUIPMENT");
            return;
        }

        if ((pEqu->Status & (StatusProtOff | StatusProtOn)) == StatusProtOn)
        {
            DisplayError("EQUIPMENT IN DESIRED STATE");
            return;
        }

        gotoxy(2, 14 + (n * 2));
        SetColours(YELLOW | BLINK, BLACK);
        cprintf("%s", pEqu->Spens);
        break;
    }

    gotoxy(1, 2);
    SetColours(GREEN, BLACK);
    cputs("CONFIRM OR CANCEL WITHIN 30 SECS");

    gotoxy(1, 1);
    SetColours(CYAN, BLACK);
    cprintf("%s %s   %s SEF ON ", pCurrentDisplay->Substation->Spens,
            pEqu->Spens, pEqu->Name);

    if (DisplayConfirm())
    {
        pEqu->Status |= StatusProtOn;
        pEqu->Status &= ~StatusProtOff;

        LogEvent("P1", pCurrentDisplay->Substation, pEqu,
                 "SEF ON     SUCCESSFUL");
    }

    sleep(2);
    DisplayRefresh();
}

/*****************************************************************************/

int DisplayConfirm()
{
    int n = CONFIRMTIME;
    time_t s, t;

    SetColours(WHITE, BLACK);
    _setcursortype(_SOLIDCURSOR);

    s = time(NULL);
    for (;;)
    {
        if (kbhit())
        {
            SetColours(WHITE, BLACK);

            if (getch() == VK_RETURN)
            {
                _setcursortype(_NOCURSOR);
                cputs("<CONFIRM>");
                return TRUE;
            }

            else
            {
                _setcursortype(_NOCURSOR);
                cputs("<CANCEL>");
                return FALSE;
            }
        }

        t = time(NULL);
        if (s != t)
        {
            s = t;
            ProcessTime(t);

            SetColours(WHITE, BLACK);

            if (n-- == 0)
            {
                _setcursortype(_NOCURSOR);
                cputs("<CANCEL>");
                return FALSE;
            }
        }
    }
}

/*****************************************************************************/

void DisplayTrip()
{
    Equipment *pEqu;
    char  *pSpens;

    if (pCurrentDisplay->Display != Substat)
    {
        DisplayError("SELECT A SUBSTATION FIRST");
        return;
    }

    pSpens = CommandWords[1];

    switch (pCurrentDisplay->Page)
    {
    case 1:
        DisplayError("OPERATION NOT PERMITTED ON EQUIPMENT");
        return;

    case 2:
        pEqu = pCurrentDisplay->Equipment->Next;

        while (pEqu != NULL)
        {
            if (pEqu->Spens[0] == '0')
            {
                DisplayError("EQUIPMENT NOT FOUND");
                return;
            }

            if (strcmp(pSpens, pEqu->Spens) == 0)
                break;

            pEqu = pEqu->Next;
        }

        break;

    case 3:
        pEqu = pCurrentDisplay->Equipment->Next;

        while (pEqu != NULL)
        {
            if (pEqu->Spens[0] == '0')
                break;

            pEqu = pEqu->Next;
        }

        while (pEqu != NULL)
        {
            if (strcmp(pSpens, pEqu->Spens) == 0)
                break;

            pEqu = pEqu->Next;
        }

        break;
    }

    if (pEqu == NULL)
    {
        DisplayError("EQUIPMENT NOT FOUND");
        return;
    }

    if (!(pEqu->Status & StatusTrippable))
    {
        DisplayError("OPERATION NOT PERMITTED ON EQUIPMENT");
        return;
    }

    if ((pEqu->Status & (StatusOpen | StatusClosed)) == StatusOpen)
    {
        DisplayError("EQUIPMENT IN DESIRED STATE");
        return;
    }

    pEqu->Status |= StatusOpen;
    pEqu->Status &= ~StatusClosed;

    if (pEqu->Status & StatusExternal)
        ExternalOperation(OPEN);

    sleep(2);

    if (pEqu->Status & StatusExternal)
        ExternalOperation(NORMAL);

    if ((pEqu->Status & StatusAutomation)
        && ((pEqu->Status & (StatusAutoOff | StatusAutoOn)) == StatusAutoOn))
    {
        pEqu->Count = RECLOSETIME;
        LogEvent("  ", pCurrentDisplay->Substation, pEqu,
                 "A A.R. IN PROGRESS");
    }

    else
    {
        pCurrentDisplay->Equipment->Status |= StatusProtAlarm;

        LogAlarm(pCurrentDisplay->Substation, pCurrentDisplay->Equipment,
                 "F FEEDER PROTECTION OP");
        LogEvent("  ", pCurrentDisplay->Substation, pCurrentDisplay->Equipment,
                 "F FEEDER PROTECTION OP");

        pEqu->Status |= StatusTripped;

        LogAlarm(pCurrentDisplay->Substation, pEqu,
                 "A C.B. TRIPPED");
        LogEvent("  ", pCurrentDisplay->Substation, pEqu,
                 "A C.B. TRIPPED");
    }

    DisplayRefresh();
}

/*****************************************************************************/

void DisplayEfi()
{
    Equipment *pEqu;
    char  *pSpens;

    if (pCurrentDisplay->Display != Substat)
    {
        DisplayError("SELECT A SUBSTATION FIRST");
        return;
    }

    pSpens = CommandWords[1];

    switch (pCurrentDisplay->Page)
    {
    case 1:
        DisplayError("OPERATION NOT PERMITTED ON EQUIPMENT");
        return;

    case 2:
        pEqu = pCurrentDisplay->Equipment->Next;

        while (pEqu != NULL)
        {
            if (pEqu->Spens[0] == '0')
            {
                DisplayError("EQUIPMENT NOT FOUND");
                return;
            }

            if (strcmp(pSpens, pEqu->Spens) == 0)
                break;

            pEqu = pEqu->Next;
        }

        break;

    case 3:
        pEqu = pCurrentDisplay->Equipment->Next;

        while (pEqu != NULL)
        {
            if (pEqu->Spens[0] == '0')
                break;

            pEqu = pEqu->Next;
        }

        while (pEqu != NULL)
        {
            if (strcmp(pSpens, pEqu->Spens) == 0)
                break;

            pEqu = pEqu->Next;
        }

        break;
    }

    if (pEqu == NULL)
    {
        DisplayError("EQUIPMENT NOT FOUND");
        return;
    }

    if (!(pEqu->Status & StatusProtection))
    {
        DisplayError("OPERATION NOT PERMITTED ON EQUIPMENT");
        return;
    }

    if ((pEqu->Status & (StatusOpen | StatusClosed)) == StatusOpen)
    {
        DisplayError("EQUIPMENT IN DESIRED STATE");
        return;
    }

    if ((pEqu->Status & (StatusProtOff | StatusProtOn)) == StatusProtOn)
    {
        pEqu->Status |= StatusEfiAlarm;

        LogAlarm(pCurrentDisplay->Substation, pEqu,
                 "F EARTH FAULT INDICATION");
        LogEvent("  ", pCurrentDisplay->Substation, pEqu,
                 "F EARTH FAULT INDICATION");
    }

    sleep(2);
    DisplayRefresh();
}

/*****************************************************************************/

void SetAnalogue()
{
    Equipment *pEqu;
    char  *pSpens;

    if ((pCurrentDisplay->Display != Substat) && (pCurrentDisplay->Display != Drawing))
    {
        DisplayError("SELECT A SUBSTATION FIRST");
        return;
    }

    pSpens = CommandWords[1];

    pEqu = pCurrentDisplay->Equipment;

    while (pEqu != NULL)
    {
        if (strcmp(pSpens, pEqu->Spens) == 0)
            break;

        pEqu = pEqu->Next;
    }

    if (pEqu == NULL)
    {
        DisplayError("EQUIPMENT NOT FOUND");
        return;
    }

    pEqu->Analog[0] = atoi(CommandWords[2]);
    pEqu->Analog[1] = pEqu->Analog[0];

    sleep(2);
    DisplayRefresh();
}

/*****************************************************************************/

void SetOperations()
{
    Equipment *pEqu;
    char  *pSpens;

    if (pCurrentDisplay->Display != Substat)
    {
        DisplayError("SELECT A SUBSTATION FIRST");
        return;
    }

    pSpens = CommandWords[1];

    pEqu = pCurrentDisplay->Equipment;

    while (pEqu != NULL)
    {
        if (strcmp(pSpens, pEqu->Spens) == 0)
            break;

        pEqu = pEqu->Next;
    }

    if (pEqu == NULL)
    {
        DisplayError("EQUIPMENT NOT FOUND");
        return;
    }

    pEqu->Operns = atoi(CommandWords[2]);

    sleep(2);
    DisplayRefresh();
}

/*****************************************************************************/

void SetExternal()
{
    Equipment *pEqu;
    char  *pSpens;

    if (pCurrentDisplay->Display != Substat)
    {
        DisplayError("SELECT A SUBSTATION FIRST");
        return;
    }

    pSpens = CommandWords[1];

    pEqu = pCurrentDisplay->Equipment;

    while (pEqu != NULL)
    {
        if (strcmp(pSpens, pEqu->Spens) == 0)
            break;

        pEqu = pEqu->Next;
    }

    if (pEqu == NULL)
    {
        DisplayError("EQUIPMENT NOT FOUND");
        return;
    }

    if (strcmp("ON", CommandWords[2]) == 0)
        pEqu->Status |= StatusExternal;

    if (strcmp("OFF", CommandWords[2]) == 0)
        pEqu->Status &= ~StatusExternal;

    sleep(2);
    DisplayRefresh();
}

/*****************************************************************************/

void SetStatus()
{
    Equipment *pEqu;
    char  *pSpens;

    if ((pCurrentDisplay->Display != Substat) && (pCurrentDisplay->Display != Drawing))
    {
        DisplayError("SELECT A SUBSTATION FIRST");
        return;
    }

    pSpens = CommandWords[1];

    pEqu = pCurrentDisplay->Equipment;

    while (pEqu != NULL)
    {
        if (strcmp(pSpens, pEqu->Spens) == 0)
            break;

        pEqu = pEqu->Next;
    }

    if (pEqu == NULL)
    {
        DisplayError("EQUIPMENT NOT FOUND");
        return;
    }

    pEqu->Status = (int) strtol(CommandWords[2], NULL, 16);

    sleep(2);
    DisplayRefresh();
}

/*****************************************************************************/

void LogEvent(char *Pos, Substation *pSub, Equipment *pEqu, char *Event)
{
    time_t t;
    FILE *File;
    struct tm *tp;
    static int nPrintLines = 0;
    static long EventDate = 0;

    t = time(NULL);
    tp = localtime(&t);

    File = fopen("EVENTS.LOG", "a");

    if (File == NULL)
    {
        DisplayError("CADEC EVENTS FILE NOT FOUND");
        return;
    }

    fprintf(File, "%02d/%02d/%02d %02d:%02d:%02d %s %s %-14.14s %s    %-16.16s %s\n",
            tp->tm_mday, tp->tm_mon + 1, tp->tm_year % 100,
            tp->tm_hour, tp->tm_min, tp->tm_sec, Pos,
            pSub->Spens, pSub->Name, pEqu->Spens, pEqu->Name, Event);

    fclose(File);
#ifdef DOS
    if (biosprint(2, 0, 0) != 0x90)
        return;

    if ((EventDate != (t / (60l * 60l * 24l))) || ((nPrintLines++) == 10))
    {
        EventDate = t / (60l * 60l * 24l);
        nPrintLines = 0;

        fprintf(stdprn, "%02d/%02d/%02d\n",
                tp->tm_mday, tp->tm_mon + 1, tp->tm_year % 100);
    }

    fprintf(stdprn, "%02d:%02d:%02d %s %s %-14.14s %s    %-16.16s %s\n",
            tp->tm_hour, tp->tm_min, tp->tm_sec, Pos,
            pSub->Spens, pSub->Name, pEqu->Spens, pEqu->Name, Event);
#endif
}

/*****************************************************************************/

void DisplayEvents()
{
    pCurrentDisplay->Function = Init;

    EventsDisplay();
}

/*****************************************************************************/

void EventsDisplay()
{
    int n;
    FILE *File;

    char TextLine[96],
        EventDate[16] = "",
        *pFindText;

    short EventYear,
        EventMonth,
        EventDay;

    static long EventStart = 0,
        EventEnd   = 0,
        EventFind  = 0;

    static char FindText[40] = "";

    File = fopen("EVENTS.LOG", "r");

    if (File == NULL)
    {
        DisplayError("CADEC EVENTS FILE NOT FOUND");
        return;
    }

    switch(pCurrentDisplay->Function)
    {
    case Init:
        EventStart = 0;

        if (CommandNumber > 0)
        {
            sscanf(CommandWords[1], "%d/%d/%d",
                   &EventDay, &EventMonth, &EventYear);

            if (EventDay > 31)
                EventDay = 31;

            if (EventMonth > 12)
                EventMonth = 12;

            if (EventYear > 99)
                EventYear = EventYear % 100;

            sprintf(EventDate, "%02d/%02d/%02d",
                    EventDay, EventMonth, EventYear);

            while (fgets(TextLine, 95, File) != NULL)
            {
                if (strncmp(TextLine, EventDate, 8) == 0)
                    break;

                EventStart = ftell(File);
            }

            if (feof(File))
            {
                DisplayError("DATE NOT FOUND");
                fclose(File);
                return;
            }

            fseek(File, EventStart, SEEK_SET);
            EventDate[0] = '\0';
        }

        else
        {
            fseek(File, 0L, SEEK_END);
            EventEnd = ftell(File);

            if (EventEnd < 1536)
            {
                fseek(File, 0L, SEEK_SET);
            }

            else
            {
                fseek(File, EventEnd - 1536, SEEK_SET);
                fgets(TextLine, 95, File);
            }
        }

        pCurrentDisplay->Display = Events;
        FindText[0] = '\0';

        ClearCommand();
        ClearDisplay();
        DisplayHeader("HISTORICAL EVENTS");

        break;

    case Pageon:
        fseek(File, EventEnd, SEEK_SET);
        fgets(TextLine, 95, File);

        if (feof(File))
        {
            DisplayError("END OF EVENT FILE");
            fclose(File);
            return;
        }

        fseek(File, EventEnd, SEEK_SET);
        ClearCommand();
        ClearDisplay();
        break;

    case Pageback:
        if (EventStart == 0)
        {
            DisplayError("START OF EVENT FILE");
            fclose(File);
            return;
        }

        if (EventStart < 2048)
        {
            fseek(File, 0L, SEEK_SET);
        }

        else
        {
            fseek(File, EventStart - 2048, SEEK_SET);
            fgets(TextLine, 95, File);
        }

        ClearCommand();
        ClearDisplay();
        break;

    case Refresh:
        fseek(File, EventStart, SEEK_SET);
        break;

    case Find:
        if (CommandNumber > 0)
        {
            strcpy(FindText, CommandWords[1]);

            EventFind = EventStart;
        }
        else
        {
            if (FindText[0] == '\0')
            {
                DisplayError("NOTHING TO FIND");
                fclose(File);
                return;
            }
        }

        fseek(File, EventFind, SEEK_SET);
        while (fgets(TextLine, 95, File) != NULL)
        {
            if (strstr(TextLine, FindText) != NULL)
                break;

            EventFind = ftell(File);
        }

        if (feof(File))
        {
            DisplayError("NOT FOUND");
            fclose(File);
            return;
        }

        fseek(File, EventFind, SEEK_SET);

        ClearCommand();
        ClearDisplay();
        break;
    }

    EventStart = ftell(File);

    gotoxy(62, 7);
    SetColours(GREEN, BLACK);
    cprintf("\"%s\"", FindText);
    clreol();

    for (n = 8; n < 46; n++)
    {
        if (fgets(TextLine, 95, File) == NULL)
            break;

        gotoxy(1, n);
        if (strncmp(EventDate, TextLine, 8) != 0)
        {
            if (n >= 44)
                break;

            strncpy(EventDate, TextLine, 8);

            gotoxy(1, ++n);
            SetColours(BROWN, BLACK);
            cputs(EventDate);

            SetColours(GREEN, BLACK);
            gotoxy(1, ++n);
        }

        cputs(&TextLine[9]);

        if ((FindText[0] != '\0')
            && ((pFindText = strstr(&TextLine[9], FindText)) != NULL))
        {
            EventFind = ftell(File);

            gotoxy((pFindText - TextLine) - 8, n);
            SetColours(BLACK, RED);
            cputs(FindText);
            SetColours(GREEN, BLACK);
        }
    }

    EventEnd = ftell(File);

    if (!feof(File))
    {
        gotoxy(78, 7);
        SetColours(GREEN, BLACK);
        cputs(">>");
    }

    fclose(File);
}

/*****************************************************************************/

void DisplayFind()
{
    if (pCurrentDisplay->Display != Events)
    {
        DisplayError("DISPLAY EVENTS FIRST");
        return;
    }

    pCurrentDisplay->Function = Find;

    EventsDisplay();
}

/*****************************************************************************/

void LogAlarm(Substation *pSub, Equipment *pEqu, char *Alarm)
{
    memmove(&pAlarmList[1], &pAlarmList[0], sizeof(AlarmList) * nAlarms);

    pAlarmList[0].DateTime = time(NULL);
    pAlarmList[0].Number = AlarmNumber;
    pAlarmList[0].Substation = pSub;
    pAlarmList[0].Equipment = pEqu;
    strcpy(pAlarmList[0].Alarm, Alarm);

    AlarmNumber++;
    if (AlarmNumber > 63)
        AlarmNumber = 1;

    nAlarms++;
    if (nAlarms > 63)
        nAlarms = 63;

    DisplayBanner();

    Dingdong = 1;
}

/*****************************************************************************/

void DisplayAlarms()
{
    pCurrentDisplay->Function = Init;

    AlarmsDisplay();
}

/*****************************************************************************/

void AlarmsDisplay()
{
    int n;
    long AlarmDate = 0;
    struct tm *tp;

    static int AlarmStart = 0,
        AlarmEnd = 0;

    switch(pCurrentDisplay->Function)
    {
    case Init:
        pCurrentDisplay->Display = Alarms;

        ClearCommand();
        DisplayHeader("OUTSTANDING ALARM LIST");

        AlarmStart = 0;
        break;

    case Pageon:
        if (nAlarms == AlarmEnd)
        {
            DisplayError("END OF ALARMS");
            return;
        }

        ClearCommand();
        AlarmStart = AlarmEnd;
        break;

    case Pageback:
        if (AlarmStart == 0)
        {
            DisplayError("START OF ALARMS");
            return;
        }

        ClearCommand();
        AlarmStart -= 18;
        if (AlarmStart < 0)
            AlarmStart = 0;
        break;

    case Refresh:
        break;
    }

    gotoxy(71, 7);
    SetColours(GREEN, BLACK);
    cprintf("PAGE %d", (AlarmStart / 18) + 1);
    clreol();

    ClearDisplay();

    if (nAlarms == 0)
    {
        gotoxy(28, 13);
        SetColours(YELLOW, BLACK);
        cputs("NO OUTSTANDING ALARMS");
        return;
    }

    for (n = AlarmStart; n < (AlarmStart + 18); n++)
    {
        if (n == nAlarms)
            break;

        tp = localtime(&pAlarmList[n].DateTime);

        if (AlarmDate != (pAlarmList[n].DateTime / (60l * 60l * 24l)))
        {
            AlarmDate = pAlarmList[n].DateTime / (60l * 60l * 24l);

            gotoxy(1, 9 + ((n - AlarmStart) * 2));
            SetColours(GREEN, BLACK);

            cprintf("%02d/%02d/%02d",
                    tp->tm_mday, tp->tm_mon + 1, tp->tm_year % 100);
        }

        gotoxy(1, 10 + ((n - AlarmStart) * 2));
        SetColours(YELLOW, BLACK);

        cprintf("%2d %02d:%02d:%02d %4s %-14.14s %-5s %-16.16s %-24.24s",
                pAlarmList[n].Number,
                tp->tm_hour, tp->tm_min, tp->tm_sec,
                pAlarmList[n].Substation->Spens, pAlarmList[n].Substation->Name,
                pAlarmList[n].Equipment->Spens, pAlarmList[n].Equipment->Name,
                pAlarmList[n].Alarm);
    }

    AlarmEnd = n;

    if (n != nAlarms)
    {
        gotoxy(78, 7);
        SetColours(GREEN, BLACK);
        cputs(">>");
    }
}

/*****************************************************************************/

void DisplayAccept()
{
    int  n, m;
    Equipment *pEqu;
    char  *pSpens;
    static  int AlarmsAccepted = 0;

    switch(pCurrentDisplay->Display)
    {
    case Substat:
        pSpens = CommandWords[1];

        pEqu = pCurrentDisplay->Equipment;

        while (pEqu != NULL)
        {
            if (strcmp(pSpens, pEqu->Spens) == 0)
                break;

            pEqu = pEqu->Next;
        }

        if (pEqu == NULL)
        {
            DisplayError("EQUIPMENT NOT FOUND");
            return;
        }

        pEqu->Status &= ~StatusProtAlarm;
        pEqu->Status &= ~StatusEfiAlarm;
        pEqu->Status &= ~StatusTripped;
        pEqu->Status &= ~StatusLockout;

        for (n = 0; n < nAlarms; n++)
        {
            if (pAlarmList[n].Equipment == pEqu)
            {
                memmove(&pAlarmList[n], &pAlarmList[n + 1],
                        sizeof(AlarmList) * (nAlarms - n));
                AlarmsAccepted++;
                nAlarms--;
                n--;
            }
        }
        break;

    case Alarms:
        m = atoi(CommandWords[1]);

        for (n = 0; n < nAlarms; n++)
        {
            if (m == pAlarmList[n].Number)
                break;
        }

        if (n == nAlarms)
        {
            DisplayError("EQUIPMENT NOT FOUND");
            return;
        }

        if (strcmp(pAlarmList[n].Alarm, "F FEEDER PROTECTION OP") == 0)
            pAlarmList[n].Equipment->Status &= ~StatusProtAlarm;

        if (strcmp(pAlarmList[n].Alarm, "F EARTH FAULT INDICATION") == 0)
            pAlarmList[n].Equipment->Status &= ~StatusEfiAlarm;

        if (strcmp(pAlarmList[n].Alarm, "A C.B. TRIPPED") == 0)
            pAlarmList[n].Equipment->Status &= ~StatusTripped;

        if (strcmp(pAlarmList[n].Alarm, "A A.R. LOCKED OUT") == 0)
            pAlarmList[n].Equipment->Status &= ~StatusLockout;

        memmove(&pAlarmList[n], &pAlarmList[n + 1], sizeof(AlarmList)
                * (nAlarms - n));

        AlarmsAccepted++;
        nAlarms--;
        break;

    default:
        DisplayError("SELECT ALARM LIST FIRST");
        return;
    }

    if (nAlarms == 0)
        AlarmNumber = 1;

    if (AlarmsAccepted > 5)
    {
        for (n = 0; n < nAlarms; n++)
            pAlarmList[n].Number = nAlarms - n;

        AlarmsAccepted = 0;
        AlarmNumber = nAlarms + 1;
    }

    DisplayBanner();
    DisplayRefresh();
}

/*****************************************************************************/

void DisplayBanner()
{
    int  n, m, x, y;
    Equipment *pEqu;

    x = wherex();
    y = wherey();

    SetColours(WHITE, BLACK);
    gotoxy(1, 4);
    clreol();
    gotoxy(1, 5);
    clreol();

    for (n = 0, m = 0; n < nSubstation; n++)
    {
        pEqu = pSubstation[n].Equipment;

        while (pEqu != NULL)
        {
            if ((pEqu->Status & (StatusTripped | StatusEfiAlarm
                                 | StatusProtAlarm | StatusLockout)) != 0)
            {
                gotoxy(((m % 13) * 6) + 1, (m / 13) + 4);
                SetColours(BLACK, RED);
                cputs(pSubstation[n].Spens);

                m++;
                break;
            }

            pEqu = pEqu->Next;
        }
    }

    SetColours(WHITE, BLACK);
    gotoxy(x, y);
}

/*****************************************************************************/

void DisplayAbnormal()
{
    int  n, m;
    time_t  t;
    struct  tm *tp;
    Equipment *pEqu;

    pCurrentDisplay->Display = Abnormal;

    ClearCommand();
    ClearDisplay();

    DisplayHeader("SYSTEM ABNORMALITIES");

    t = time(NULL);
    tp = localtime(&t);
#ifdef DOS
    if (biosprint(2, 0, 0) == 0x90)
        fprintf(stdprn, "%02d/%02d/%02d %02d:%02d:%02d SYSTEM ABNORMALITY REPORT\n\n",
                tp->tm_mday, tp->tm_mon + 1, tp->tm_year % 100,
                tp->tm_hour, tp->tm_min, tp->tm_sec);
#endif
    gotoxy(71, 7);
    SetColours(GREEN, BLACK);
    cputs("PAGE 1");

    for (n = 0, m = 9; n < nSubstation; n++)
    {
        pEqu = pSubstation[n].Equipment->Next;

        while (pEqu != NULL)
        {
            if (pEqu->Spens[0] != 'T')
            {
                if ((pEqu->Status & (StatusOpen | StatusClosed))
                    != StatusClosed)
                {
                    gotoxy(1, m);
                    SetColours(YELLOW, BLACK);
                    cprintf("%s %-14.14s %s    %-16.16s CIRCUIT BREAKER %s",
                            pSubstation[n].Spens,
                            pSubstation[n].Name,
                            pEqu->Spens, pEqu->Name,
                            ((pEqu->Status
                              & (StatusOpen | StatusClosed))
                             == StatusOpen)? "OPEN": "DBI");
#ifdef DOS
                    if (biosprint(2, 0, 0) == 0x90)
                        fprintf(stdprn,
                                "%s %-14.14s %s    %-16.16s CIRCUIT BREAKER %s\n",
                                pSubstation[n].Spens,
                                pSubstation[n].Name,
                                pEqu->Spens, pEqu->Name,
                                ((pEqu->Status
                                  & (StatusOpen | StatusClosed))
                                 == StatusOpen)? "OPEN": "DBI");
#endif
                    m++;
                }

                if ((pEqu->Status & StatusProtection) &&
                    ((pEqu->Status & (StatusProtOff | StatusProtOn))
                     != StatusProtOn))
                {
                    gotoxy(1, m);
                    SetColours(YELLOW, BLACK);
                    cprintf("%s %-14.14s %s    %-16.16s SELECTIVE EARTH FAULT %s",
                            pSubstation[n].Spens,
                            pSubstation[n].Name,
                            pEqu->Spens, pEqu->Name,
                            ((pEqu->Status
                              & (StatusProtOff | StatusProtOn))
                             == StatusProtOff)? "OFF": "DBI");
#ifdef DOS
                    if (biosprint(2, 0, 0) == 0x90)
                        fprintf(stdprn,
                                "%s %-14.14s %s    %-16.16s SELECTIVE EARTH FAULT %s\n",
                                pSubstation[n].Spens,
                                pSubstation[n].Name,
                                pEqu->Spens, pEqu->Name,
                                ((pEqu->Status
                                  & (StatusProtOff | StatusProtOn))
                                 == StatusProtOff)? "OFF": "DBI");
#endif
                    m++;
                }

                if ((pEqu->Status & StatusAutomation) &&
                    ((pEqu->Status & (StatusAutoOff | StatusAutoOn))
                     != StatusAutoOn))
                {
                    gotoxy(1, m);
                    SetColours(YELLOW, BLACK);
                    cprintf("%s %-14.14s %s    %-16.16s AUTO RECLOSE %s",
                            pSubstation[n].Spens,
                            pSubstation[n].Name,
                            pEqu->Spens, pEqu->Name,
                            ((pEqu->Status
                              & (StatusAutoOff | StatusAutoOn))
                             == StatusAutoOff)? "OFF": "DBI");
#ifdef DOS
                    if (biosprint(2, 0, 0) == 0x90)
                        fprintf(stdprn,
                                "%s %-14.14s %s    %-16.16s AUTO RECLOSE %s\n",
                                pSubstation[n].Spens,
                                pSubstation[n].Name,
                                pEqu->Spens, pEqu->Name,
                                ((pEqu->Status
                                  & (StatusAutoOff | StatusAutoOn))
                                 == StatusAutoOff)? "OFF": "DBI");
#endif
                    m++;
                }

                if (pEqu->Status & StatusExternal)
                {
                    gotoxy(1, m);
                    SetColours(YELLOW, BLACK);
                    cprintf("%s %-14.14s %s    %-16.16s EXTERNAL OUTPUT ENABLED",
                            pSubstation[n].Spens,
                            pSubstation[n].Name,
                            pEqu->Spens, pEqu->Name);
#ifdef DOS
                    if (biosprint(2, 0, 0) == 0x90)
                        fprintf(stdprn,
                                "%s %-14.14s %s    %-16.16s EXTERNAL OUTPUT ENABLED\n",
                                pSubstation[n].Spens,
                                pSubstation[n].Name,
                                pEqu->Spens, pEqu->Name);
#endif
                    m++;
                }
            }

            else
            {
                if ((((Equipment *)pEqu->Next)->Spens[0] == 'T') &&
                    (pEqu->Operns != ((Equipment *)pEqu->Next)->Operns))
                {
                    gotoxy(1, m);
                    SetColours(YELLOW, BLACK);
                    cprintf("%s %-14.14s %s    %-16.16s TAP CHANGE OUT OF STEP",
                            pSubstation[n].Spens,
                            pSubstation[n].Name,
                            pEqu->Spens, pEqu->Name);
#ifdef DOS
                    if (biosprint(2, 0, 0) == 0x90)
                        fprintf(stdprn,
                                "%s %-14.14s %s    %-16.16s TAP CHANGE OUT OF STEP\n",
                                pSubstation[n].Spens,
                                pSubstation[n].Name,
                                pEqu->Spens, pEqu->Name);
#endif
                    m++;
                }
            }

            if (m > 46)
                break;

            pEqu = pEqu->Next;
        }

        if (m > 46)
            break;
    }
}

/*****************************************************************************/

void DisplayStatus()
{
    int i;

    pCurrentDisplay->Display = Status;

    DisplayBackground("DISPLAY.006");
    DisplayHeader("SYSTEM STATUS");

    gotoxy(71, 7);
    SetColours(GREEN, BLACK);
    cputs("PAGE 1");

    gotoxy(1, 9);
    SetColours(YELLOW, BLACK);
    cputs("WEST COMPUTER A ONLINE AUTO       STANDBY HOT");

    for (i = 0; i < nConcentrator; i++)
    {
        gotoxy(1, i + 14);
        cprintf("%3d", i + 1);

        if (i == 0)
        {
            gotoxy(7, 14);
            cprintf("%2d", nConcentrator);
        }

        else
        {
            gotoxy(7, i + 14);
            cprintf("%2d", i);
        }

        gotoxy(11, i + 14);
        cprintf("%-13.13s ONLINE", pConcentrator[i].Name);

        gotoxy(34, i + 14);
        cputs("NORM");

        gotoxy(45, i + 14);
        cputs("ON");

        gotoxy(58, i + 14);
        cputs("0");

        gotoxy(60, i + 14);
        cprintf("%6d", getrandom(1024));

        gotoxy(72, i + 14);
        cputs("0");

        gotoxy(74, i + 14);
        cprintf("%6d", getrandom(1024));
    }
}

/*****************************************************************************/

void DisplayConc()
{
    int i, n, m;
    Outstation *pOstn;

    if (CommandNumber == 0)
    {
        DisplayError("CONCENTRATOR NOT FOUND");
        return;
    }

    n = atoi(CommandWords[1]) - 1;

    if ((n >= nConcentrator) || (n < 0))
    {
        DisplayError("CONCENTRATOR NOT FOUND");
        return;
    }

    pCurrentDisplay->Display = Status;

    DisplayBackground("DISPLAY.008");
    DisplayHeader("CONCENTRATOR STATUS");

    gotoxy(71, 7);
    SetColours(GREEN, BLACK);
    cputs("PAGE 1");

    gotoxy(20, 10);
    SetColours(YELLOW, BLACK);
    cprintf("%02d", n + 1);

    gotoxy(30, 10);
    cputs(pConcentrator[n].Name);

    gotoxy(62, 10);
    cputs("ONLINE");

    if (n == 0)
        m = nConcentrator;

    else
        m = n;

    gotoxy(20, 39);
    cprintf("%02d", m);

    gotoxy(30, 39);
    cputs(pConcentrator[m - 1].Name);

    gotoxy(62, 39);
    cputs("ONLINE");

    i = 0;
    pOstn = pConcentrator[n].Outstation;
    while (pOstn != NULL)
    {
        gotoxy(2, 17 + (i * 2));
        cprintf("%3d", ((n + 1) * 10) + i);

        gotoxy(7, 17 + (i * 2));
        cputs(pOstn->Name);

        gotoxy(26, 17 + (i * 2));
        cputs("ONLINE");

        gotoxy(34, 17 + (i * 2));
        cputs("ON");

        gotoxy(40, 17 + (i * 2));
        cputs("OFF");

        gotoxy(46, 17 + (i * 2));
        cputs("OFF");

        gotoxy(53, 17 + (i * 2));
        cputs("OFF");

        pOstn = pOstn->Next;
        i++;
    }

    gotoxy(74, 37);
    cputs("OFF");
}

/*****************************************************************************/

void SetProbTrip()
{
    if (CommandNumber > 0)
        ProbTrip = atoi(CommandWords[1]);

    sleep(2);
    ClearCommand();
}

/*****************************************************************************/

void SetProbReclose()
{
    if (CommandNumber > 0)
        ProbReclose = atoi(CommandWords[1]);

    sleep(2);
    ClearCommand();
}

/*****************************************************************************/

void SetProbCursor()
{
    if (CommandNumber > 0)
        ProbCursor = atoi(CommandWords[1]);

    sleep(2);
    ClearCommand();
}

/*****************************************************************************/

void SetJitterFactor()
{
    if (CommandNumber > 0)
        JitterFactor = atoi(CommandWords[1]);

    sleep(2);
    ClearCommand();
}

/*****************************************************************************/

void SetSerialPort()
{
    if (CommandNumber > 0)
        SerialPort = atoi(CommandWords[1]);

    sleep(2);
    ClearCommand();
}

/*****************************************************************************/

void SetFrequency()
{
    if (CommandNumber > 0)
        Frequency = atoi(CommandWords[1]);

    sleep(2);
    DisplayRefresh();
}

/*****************************************************************************/

void DisplayParameters()
{
    int m;

    pCurrentDisplay->Display = Parameters;

    DisplayBackground("DISPLAY.007");
    DisplayHeader("SYSTEM PARAMETERS");

    gotoxy(71, 7);
    SetColours(GREEN, BLACK);
    cputs("PAGE 1");


    m = (sizeof(Substation) * nSubstation) +
        (sizeof(Equipment) * nEquipment) +
        (sizeof(Concentrator) * nConcentrator) +
        (sizeof(Outstation) + nOutstation) +
        (sizeof(AlarmList) * 64) +
        sizeof(CurrentDisplay);

    gotoxy(48, 14);
    SetColours(GREEN, BLACK);
    cprintf("%2d.%02dK", m / 1024, (m % 1024) / 10);

    gotoxy(48, 16);
    cprintf("%3d", nSubstation);

    gotoxy(48, 18);
    cprintf("%3d", nEquipment);

    gotoxy(48, 23);
    cprintf("%d.%02d%%", ProbTrip / 100, ProbTrip % 100);

    gotoxy(48, 25);
    cprintf("%4d%%", ProbReclose);

    gotoxy(48, 27);
    cprintf("%4d%%", ProbCursor);

    gotoxy(48, 30);
    cprintf("%5d%", JitterFactor);

    gotoxy(48, 33);
    cprintf("COM%d:", SerialPort);
}

/*****************************************************************************/

void DisplayHeader(char *Header)
{
    gotoxy(19, 7);
    SetColours(BLACK, WHITE);
    clreol();

    gotoxy(39 - (strlen(Header) / 2), 7);
    cputs(Header);

    gotoxy(61, 7);
    SetColours(WHITE, BLACK);
    clreol();
}

/*****************************************************************************/

void DisplayStop()
{
    Stopit = TRUE;

    gotoxy(48, 47);
    SetColours(YELLOW, BLACK);
    cputs("STOPPED");

    sleep(2);
    ClearCommand();
}

/*****************************************************************************/

void DisplayStart()
{
    Stopit = FALSE;

    gotoxy(48, 47);
    cputs("       ");

    sleep(2);
    ClearCommand();
}

/*****************************************************************************/

void DisplayExit()
{
    SaveAlarms();
    SaveData();

    DisplayError("CADEC SYSTEM SHUTTING DOWN");

    free(pCurrentDisplay);
    free(pSubstation);
    free(pEquipment);

#ifndef TESTING

    textmode(C80);
    sleep(1);

    SetColours(BLACK, WHITE);
    clrscr();
    gotoxy(1, 1);

    cputs("CADEC SYSTEM SHUTTING DOWN");
    sleep(1);

    cputs("\r\n>@SY0:[1,2]SHUTDOWN.CMD");
    sleep(2);

    cputs("\r\nPROCESSOR HALTING");
    sleep(1);

    cputs("\r\n27700362\r\n@");
    sleep(5);

#endif

    exit(0);
}

/*****************************************************************************/

void ShowPosition(int x, int y)
{
    int c;

    gotoxy(10, 47);
    _setcursortype(_NOCURSOR);
    SetColours(GREEN, BLACK);
    cputs("          ");

    if ((y > 7) && (y < 47))
    {
        gotoxy(x, y);
        c = GetScreenChar();

        gotoxy(10, 47);
        cprintf("%3d%3d%4d", x, y, c);
    }

    gotoxy(x, y);
    _setcursortype(_SOLIDCURSOR);
    SetColours(WHITE, BLACK);
}

/*****************************************************************************/

int GetScreenChar()
{
    return screenchar();
}

/*****************************************************************************/

char *GetScreenStr(int x, int y)
{
    static char s[96];
    int c, i;

    i = 0;

    if (y != 1)
    {
        gotoxy(x, y);
        c = GetScreenChar();
  
        while (isspace(c))
        {
            x++;
            if (x > 80)
                break;

            gotoxy(x, y);
            c = GetScreenChar();
        }

        while (isalnum(c))
        {
            s[i++] = c;
            x++;

            gotoxy(x, y);
            c = GetScreenChar();
        }
    }

    s[i] = '\0';

    return s;
}

/*****************************************************************************/

void ProcessString(int n)
{
    char s[96];

    strcpy(s, Command);
    strcpy(Command, CommandList[n]);
    strcat(Command, " ");
    strcat(Command, s);
}

/*****************************************************************************/

void ProcessPosition(int x, int y, char *c)
{
    char *s;

    s = GetScreenStr(x, y);
    strcat(Command, s);

    gotoxy(CommandIndex + 1, 1);
    cputs(s);
    cputs(c);
}

/*****************************************************************************/

void ProcessKey()
{
    int c, x, y;

    SetColours(WHITE, BLACK);

    TimeOut = 0;
    Dingdong = 0;

    x = wherex();
    y = wherey();

    ShowPosition(CommandIndex + 1, 1);

    if ((c = getch()) == '\0')
    {
        Command[CommandIndex] = '\0';

        switch (c = getch())
        {
        case VK_END + VK_SHIFT:
            ProcessPosition(x, y, "<EXIT>");
            ProcessString(Exit);
            break;

        case VK_F1:
            ProcessPosition(x, y, "<INDEX>");
            ProcessString(Index);
            break;

        case VK_F2:
            ProcessPosition(x, y, "<SUBDIR>");
            ProcessString(Directory);
            break;

        case VK_F3:
            ProcessPosition(x, y, "<SUBSEL>");
            ProcessString(Substat);
            break;

        case VK_F4:
            ProcessPosition(x, y, "<EVENTS>");
            ProcessString(Events);
            break;

        case VK_F5:
            ProcessPosition(x, y, "<ALARMS>");
            ProcessString(Alarms);
            break;

        case VK_F6:
            ProcessPosition(x, y, "<ALMACC>");
            ProcessString(Accept);
            break;

        case VK_F7:
            ProcessPosition(x, y, "<OPEN>");
            ProcessString(Open);
            break;

        case VK_F8:
            ProcessPosition(x, y, "<CLOSE>");
            ProcessString(Close);
            break;

        case VK_F9:
            ProcessPosition(x, y, "<FIND>");
            ProcessString(Find);
            break;

        case VK_F10:
            ProcessPosition(x, y, "<DRAWING>");
            ProcessString(Drawing);
            break;

        case VK_HOME:
            gotoxy(1, 1);
            CommandIndex = 0;
            return;

        case VK_UP:
            if (--y == 0)
                y = 50;
            ShowPosition(x, y);
            return;

        case VK_PRIOR:
            ProcessPosition(x, y, "<PAGEBACK>");
            ProcessString(Pageback);
            break;

        case VK_LEFT:
            if (--x == 0)
                x = 80;
            ShowPosition(x, y);
            return;

        case 76:
            ProcessPosition(x, y, "<TRIP>");
            ProcessString(Trip);
            break;

        case VK_RIGHT:
            if (++x == 81)
                x = 1;
            ShowPosition(x, y);
            return;

        case VK_DOWN:
            if (++y == 51)
                y = 1;
            ShowPosition(x, y);
            return;

        case VK_NEXT:
            ProcessPosition(x, y, "<PAGEON>");
            ProcessString(Pageon);
            break;

        case VK_F21:
            ProcessPosition(x, y, "<STOP>");
            ProcessString(Stop);
            break;

        case VK_F22:
            ProcessPosition(x, y, "<START>");
            ProcessString(Start);
            break;

        case VK_F23:
            ProcessPosition(x, y, "<AUTOFF>");
            ProcessString(Autoff);
            break;

        case VK_F24:
            ProcessPosition(x, y, "<AUTON>");
            ProcessString(Auton);
            break;

        case VK_F9 + VK_SHIFT:
            ProcessPosition(x, y, "<EDB>");
            ProcessString(Data);
            break;

        case VK_F10 + VK_SHIFT:
            ProcessPosition(x, y, "<EDIT>");
            ProcessString(Edit);
            break;

        case 110:
            ProcessPosition(x, y, "<SEFOFF>");
            ProcessString(Protoff);
            break;

        case 111:
            ProcessPosition(x, y, "<SEFON>");
            ProcessString(Proton);
            break;

        case VK_F11:
            ProcessPosition(x, y, "<REFRESH>");
            ProcessString(Refresh);
            break;

        case VK_F12:
            ProcessPosition(x, y, "<DISCAN>");
            ProcessString(Cancel);
            break;

        case VK_F11 + VK_SHIFT:
            ProcessPosition(x, y, "<RESET>");
            ProcessString(Reset);
            break;

        case VK_F12 + VK_SHIFT:
            ProcessPosition(x, y, "<CADEC>");
            ProcessString(Cadec);
            break;

        default:
#ifdef TESTING
            gotoxy(1, 2);
            cprintf("%d", c);
            clreol();
#endif
            gotoxy(1, 1);
            return;
        }

        ProcessCommand();
    }

    else
    {
        if (isprint(c))
        {
            c = toupper(c);

            Command[CommandIndex++] = c;
            putch(c);
        }

        if (c == VK_RETURN)
        {
            if ((y > 8) && (y < 45) &&
                (pCurrentDisplay->Display == Drawing))
            {
                DrawingEdit(x, y);
            }

            else if (y == 49)
            {
                ProcessStatusLine(x, y);
            }

            else
            {
                Command[CommandIndex] = '\0';
                ProcessCommand();
            }
        }

        if (c == VK_BACK)
        {
            if (CommandIndex != 0)
            {
                CommandIndex--;
                putch(c);
            }
        }
    }
}

/*****************************************************************************/

void ProcessCommand()
{
    int n;

    clreol();
    _setcursortype(_NOCURSOR);

    sleep(1);

    DisplayPurpleCursor();
    ProcessWords();

    for (n = 0; n < (sizeof(CommandList) / sizeof(char *)); n++)
        if (strcmp(CommandWords[0], CommandList[n]) == 0)
            break;

    switch(n)
    {
    case Index:
        DisplayIndex();
        break;

    case Directory:
        DisplayDirectory();
        break;

    case Substat:
        DisplaySubstation();
        break;

    case Events:
        DisplayEvents();
        break;

    case Find:
        DisplayFind();
        break;

    case Alarms:
        DisplayAlarms();
        break;

    case Accept:
        DisplayAccept();
        break;

    case Open:
        DisplayOpen();
        break;

    case Close:
        DisplayClose();
        break;

    case Auton:
        DisplayAutOn();
        break;

    case Autoff:
        DisplayAutOff();
        break;

    case Proton:
        DisplayProtOn();
        break;

    case Protoff:
        DisplayProtOff();
        break;

    case Refresh:
        DisplayRefresh();
        break;

    case Cancel:
        DisplayCancel();
        break;

    case Pageback:
        DisplayPageback();
        break;

    case Pageon:
        DisplayPageon();
        break;

    case Status:
        DisplayStatus();
        break;

    case Conc:
        DisplayConc();
        break;

    case Parameters:
        DisplayParameters();
        break;

    case Trip:
        DisplayTrip();
        break;

    case Efi:
        DisplayEfi();
        break;

    case Analog:
        SetAnalogue();
        break;

    case Operns:
        SetOperations();
        break;

    case Probreclose:
        SetProbReclose();
        break;

    case Probcursor:
        SetProbCursor();
        break;

    case Probtrip:
        SetProbTrip();
        break;

    case Setjitter:
        SetJitterFactor();
        break;

    case Setserial:
        SetSerialPort();
        break;

    case Drawing:
        DisplayDrawing();
        break;

    case Edit:
        EditDrawing();
        break;

    case Data:
        DisplayData();
        break;

    case Abnormal:
        DisplayAbnormal();
        break;

    case Setstatus:
        SetStatus();
        break;

    case Setfrequency:
        SetFrequency();
        break;

    case Setexternal:
        SetExternal();
        break;

    case Cadec:
        DisplayCancel();
        DisplayCadec();
        break;

    case Reset:
        DisplayReset();
        DisplayRefresh();
        break;

    case Stop:
        DisplayStop();
        break;

    case Start:
        DisplayStart();
        break;

    case Exit:
        DisplayExit();
        break;

    default:
        DisplayError("COMMAND NOT RECOGNISED");
        break;
    }

    SetColours(WHITE, BLACK);
    _setcursortype(_SOLIDCURSOR);
    gotoxy(1, 1);

    CommandIndex = 0;
}

/*****************************************************************************/

void ProcessWords()
{
    int n,
        m = 0;

    for (n = 0; n < (sizeof(CommandWords) / sizeof(void *)); n++)
        CommandWords[n] = "";

    while (Command[m] == ' ')
        m++;

    for (n = 0; n < (sizeof(CommandWords) / sizeof(void *)); n++)
    {
        CommandWords[n] = &Command[m];

        while ((Command[m] != '\0') && (Command[m] != ' '))
            m++;

        if (Command[m] == '\0')
            break;

        Command[m++] = '\0';

        while (Command[m] == ' ')
            m++;

        if (Command[m] == '\0')
            break;
    }

    CommandNumber = n;
}

/*****************************************************************************/

void DisplayPurpleCursor()
{
    int c, n;
    time_t s, t;

    if (getrandom(100) < ProbCursor)
    {
        SetColours(MAGENTA, BLACK);
        _setcursortype(_SOLIDCURSOR);
        cputs(" \b");

        n = getrandom(60);

        s = time(NULL);
        for (;;)
        {
            if (kbhit())
            {
                Dingdong = 0;

                if ((c =getch()) == '\0')
                    c = getch();

                else if (c == VK_ESCAPE)
                {
                    sleep(1);
                    break;
                }
            }

            t = time(NULL);
            if (s != t)
            {
                s = t;
                ProcessTime(t);

                if (n-- == 0)
                    break;
            }
        }

        SetColours(WHITE, BLACK);
    }
}

/*****************************************************************************/

void DisplayError(char *ErrorText)
{
    movetext(1, 1, 39, 1, 1, 2);
    gotoxy(1, 1);
    clreol();

    SetColours(RED, BLACK);
    gotoxy(40, 2);
    cputs(ErrorText);
    clreol();

    gotoxy(1, 1);
    sleep(1);
}

/*****************************************************************************/

void ProcessTime(time_t t)
{
    DisplayTime(t);

    if (!Stopit)
    {
        ProcessAutoReclose();
        ProcessTrip();

        if ((TimeOut++) == TIMEOUT)
        {
            DisplayCancel();
            DisplayCadec();
        }
    }

    ProcessDingdong();

    if ((t % 15) == 0)
    {
        AdjustAnalogues();
    }

    if ((t % 30) == 0)
    {
        SaveAlarms();
        SaveData();
    }
}

/*****************************************************************************/

void ProcessDingdong()
{
    if (Dingdong != 0)
    {
        if (--Dingdong == 0)
        {
            ExternalOperation(DINGDONG);
            sleep(PULSETIME);
            ExternalOperation(NORMAL);

            Dingdong = DINGDONGTIME - PULSETIME;
        }
    }
}

/*****************************************************************************/

void AdjustAnalogues()
{
    int  n;
    Equipment *pEqu;

    for (n = 0; n < nSubstation; n++)
    {
        pEqu = pSubstation[n].Equipment;

        while (pEqu != NULL)
        {
            if (pEqu->Analog[0] > 0)
                pEqu->Analog[1] = pEqu->Analog[0] + (getrandom(2)?
                                                     +getrandom((pEqu->Analog[0] * JitterFactor) / 100):
                                                     -getrandom((pEqu->Analog[0] * JitterFactor) / 100));

            pEqu = pEqu->Next;
        }
    }

    RefreshDisplay();
}

/*****************************************************************************/

void ProcessTrip()
{
    int  n, m;
    Equipment *pEqu;

    if (getrandom(10000) < ProbTrip)
    {
        m = getrandom(nEquipment);

        for (n = 0; n < nSubstation; n++)
        {
            pEqu = pSubstation[n].Equipment;
            while (pEqu != NULL)
            {
                if (pEqu == &pEquipment[m])
                    break;

                pEqu = pEqu->Next;
            }

            if (pEqu != NULL)
                break;
        }

        if (!(pEqu->Status & StatusTrippable))
            return;

        if ((pEqu->Status & (StatusOpen | StatusClosed)) != StatusClosed)
            return;

        if ((pEqu->Status & StatusProtection)
            && ((pEqu->Status & (StatusProtOn | StatusProtOff))
                == StatusProtOn))
        {
            pEqu->Status |= StatusEfiAlarm;

            LogAlarm(&pSubstation[n], pEqu,
                     "F EARTH FAULT INDICATION");
            LogEvent("  ", &pSubstation[n], pEqu,
                     "F EARTH FAULT INDICATION");
        }

        pEqu->Status |= StatusOpen;
        pEqu->Status &= ~StatusClosed;

        if (pEqu->Status & StatusExternal)
        {
            ExternalOperation(OPEN);
            sleep(PULSETIME);
            ExternalOperation(NORMAL);
        }

        if ((pEqu->Status & StatusAutomation)
            && ((pEqu->Status & (StatusAutoOff | StatusAutoOn))
                == StatusAutoOn))
        {
            pEqu->Count = RECLOSETIME;
            LogEvent("  ", &pSubstation[n], pEqu,
                     "A A.R. IN PROGRESS");
        }

        else
        {
            pSubstation[n].Equipment->Status |= StatusProtAlarm;

            LogAlarm(&pSubstation[n], pSubstation[n].Equipment,
                     "F FEEDER PROTECTION OP");
            LogEvent("  ", &pSubstation[n], pSubstation[n].Equipment,
                     "F FEEDER PROTECTION OP");

            pEqu->Status |= StatusTripped;

            LogAlarm(&pSubstation[n], pEqu, "A C.B. TRIPPED");
            LogEvent("  ", &pSubstation[n], pEqu, "A C.B. TRIPPED");
        }

        RefreshDisplay();
    }
}

/*****************************************************************************/

void ProcessAutoReclose()
{
    int n;
    Equipment *pEqu;

    for (n = 0; n < nSubstation; n++)
    {
        pEqu = pSubstation[n].Equipment;

        while (pEqu != NULL)
        {
            if (pEqu->Count != 0)
                if  (--(pEqu->Count) == 0)
                    DoReclose(&pSubstation[n], pEqu);

            pEqu = pEqu->Next;
        }
    }
}

/*****************************************************************************/

void DoReclose(Substation *pSub, Equipment *pEqu)
{
    if (!(pEqu->Status & StatusAutomation))
        return;

    if ((pEqu->Status & (StatusAutoOn | StatusAutoOff)) != StatusAutoOn)
        return;

    if ((pEqu->Status & (StatusOpen | StatusClosed)) != StatusOpen)
        return;

    if (getrandom(100) < ProbReclose)
    {
        pEqu->Status |= StatusClosed;
        pEqu->Status &= ~StatusOpen;

        if (pEqu->Status & StatusExternal)
        {
            ExternalOperation(CLOSE);
            sleep(PULSETIME);
            ExternalOperation(NORMAL);
        }

        LogEvent("  ", pSub, pEqu, "RECLOSE SUCCESSFUL");
    }

    else
    {
        pSub->Equipment->Status |= StatusProtAlarm;

        LogAlarm(pSub, pSub->Equipment, "F FEEDER PROTECTION OP");
        LogEvent("  ", pSub, pSub->Equipment, "F FEEDER PROTECTION OP");

        pEqu->Status |= StatusLockout;

        LogAlarm(pSub, pEqu, "A A.R. LOCKED OUT");
        LogEvent("  ", pSub, pEqu, "A A.R. LOCKED OUT");
    }

    RefreshDisplay();
}

/*****************************************************************************/

void ProcessStatusLine()
{
    time_t s, t;
    int c, x, y;

    _setcursortype(_NOCURSOR);

    sleep(1);

    gotoxy(1, 49);
    SetColours(YELLOW, BLACK);
    clreol();

    _setcursortype(_SOLIDCURSOR);

    x = 1;
    y = 49;

    s = time(NULL);
    for (;;)
    {
        if (kbhit())
        {
            Dingdong = 0;

            if ((c = getch()) == '\0')
            {
                c = getch();
                continue;
            }

            else
            {
                if (isprint(c))
                {
                    c = toupper(c);
                    SetColours(YELLOW, BLACK);
                    putch(c);

                    if (++x == 81)
                        x = 80;
                }

                else if (c == VK_RETURN)
                {
                    _setcursortype(_NOCURSOR);

                    sleep(1);

                    gotoxy(1, 1);
                    SetColours(WHITE, BLACK);
                    _setcursortype(_SOLIDCURSOR);

                    return;
                }

                else if (c == VK_ESCAPE)
                {
                    _setcursortype(_NOCURSOR);
                    sleep(1);

                    gotoxy(1, 49);
                    clreol();

                    gotoxy(1, 1);
                    SetColours(WHITE, BLACK);
                    _setcursortype(_SOLIDCURSOR);

                    return;
                }

                else if (c == VK_BACK)
                {
                    if (--x == 0)
                        x = 0;
                }
            }

            gotoxy(x, y);
        }

        t = time(NULL);
        if (s != t)
        {
            s = t;
            ProcessTime(t);
        }
    }
}

/*****************************************************************************/

void InitExternal()
{
#ifdef DOS
    _AH = 4;
    _AL = 0;
    _BH = 0;
    _BL = 0;
    _CH = 3;
    _CL = 4;
    _DX = SerialPort;
    _DX--;

    geninterrupt(0x14);

    _AH = 5;
    _AL = 1;
    _BL = 0;
    _DX = SerialPort;
    _DX--;

    geninterrupt(0x14);
#endif
}

/*****************************************************************************/

void ExternalOperation(int Operation)
{
#ifdef DOS
    _AH = 5;
    _AL = 1;
    _DX = SerialPort;
    _DX--;
    _BL = Operation;

    geninterrupt(0x14);
#endif
}

/*****************************************************************************/

void DisplayTime(time_t t)
{
    struct tm *tp;
    int x, y;
    static char timebuf[32];

    x = wherex();
    y = wherey();

    tp = localtime(&t);

    sprintf(timebuf, "%02d/%02d/%02d %02d:%02d:%02d",
            tp->tm_mday, tp->tm_mon + 1, tp->tm_year % 100,
            tp->tm_hour, tp->tm_min, tp->tm_sec);

    _setcursortype(_NOCURSOR);
    SetColours(GREEN, BLACK);
    gotoxy(1, 7);
    cputs(timebuf);

    gotoxy(x, y);
    _setcursortype(_SOLIDCURSOR);
}

/*****************************************************************************/

void DisplayReset()
{
    FILE *File;

    File = fopen("CADEC.DAT", "r");

    if (File == NULL)
    {
        DisplayError("CADEC DATA FILE NOT FOUND");
        return;
    }

    free(pCurrentDisplay);
    free(pSubstation);
    free(pEquipment);

    nSubstation = 0;
    nEquipment = 0;

    pCurrentDisplay = NULL;
    pTransformers = NULL;
    pSubstation = NULL;
    pEquipment = NULL;

    DataLoad(File);
    nAlarms = 0;

    sleep(2);

    DisplayError("DATABASE RESET");
    DisplayBanner();
    DisplayCancel();
}

/*****************************************************************************/

void LoadData()
{
    FILE *File;

    File = fopen("CADEC.SAV", "r");

    if (File == NULL)
        File = fopen("CADEC.DAT", "r");

    if (File == NULL)
    {
        DisplayError("CADEC DATA FILE NOT FOUND");
        return;
    }

    DataLoad(File);
}

/*****************************************************************************/

void DataLoad(FILE *File)
{
    int i, n, m;
    char TextLine[96],
        Item[16];


    while (fgets(TextLine, 95, File) != NULL)
    {
        if (TextLine[0] == '#')
            continue;

        sscanf(TextLine, "%3s", &Item);

        for (i = 0; i < 5; i++)
            if (strncmp(Item,  ItemList[i], 3) == 0)
                break;

        switch (i)
        {
        case 0:
            nSubstation++;
            break;

        case 1:
            nEquipment++;
            break;

        case 2:
        case 3:
        case 4:
            break;

        default:
            DisplayError("INVALID ITEM IN CADEC DATA FILE");
            break;
        }
    }

    if (pCurrentDisplay == NULL)
    {
        pCurrentDisplay = malloc(sizeof(CurrentDisplay));
        if (pCurrentDisplay == NULL)
        {
            fclose(File);
            return;
        }
    }

    if (pSubstation == NULL)
    {
        pSubstation = malloc(sizeof(Substation) * nSubstation);
        if (pSubstation == NULL)
        {
            DisplayError("NOT ENOUGH MEMORY");
            free(pCurrentDisplay);
            fclose(File);
            return;
        }
    }

    if (pEquipment == NULL)
    {
        pEquipment = malloc(sizeof(Equipment) * nEquipment);
        if (pEquipment == NULL)
        {
            DisplayError("NOT ENOUGH MEMORY");
            free(pCurrentDisplay);
            free(pSubstation);
            fclose(File);
            return;
        }
    }

    if (pTransformers == NULL)
    {
        pTransformers = malloc(sizeof(Transformers));
        if (pTransformers == NULL)
        {
            DisplayError("NOT ENOUGH MEMORY");
            free(pCurrentDisplay);
            free(pSubstation);
            free(pEquipment);
            fclose(File);
            return;
        }
    }

    n = 0;
    m = 0;

    rewind(File);

    while (fgets(TextLine, 95, File) != NULL)
    {
        if (TextLine[0] == '#')
            continue;

        sscanf(TextLine, "%3s", &Item);

        for (i = 0; i < 5; i++)
            if (strncmp(Item,  ItemList[i], 3) == 0)
                break;

        switch (i)
        {
        case 0:
            sscanf(TextLine, "%*3s %4s %[ -Z]",
                   &pSubstation[n].Spens, &pSubstation[n].Name);
            pSubstation[n].Equipment = NULL;
            pSubstation[n].Switchgear[0] = '\0';
            pSubstation[n].TapChanger[0] = '\0';
            n++;
            break;

        case 1:
            sscanf(TextLine, "%*3s %2s %4x %d %d %[ -Z]",
                   &pEquipment[m].Spens, &pEquipment[m].Status,
                   &pEquipment[m].Analog[0], &pEquipment[m].Operns,
                   &pEquipment[m].Name);
            pEquipment[m].Next = NULL;
            pEquipment[m].Trans = NULL;
            pEquipment[m].Count = 0;

            pEquipment[m].Analog[1] = pEquipment[m].Analog[0];

            if (pEquipment[m].Spens[0] == 'T')
                pTransformers->Trans[pEquipment[m].Spens[1] - '1'] = &pEquipment[m];

            if (pEquipment[m].Spens[0] == '5')
                pTransformers->Trans[pEquipment[m].Spens[1] - '1']->Trans = &pEquipment[m];

            if (pSubstation[n - 1].Equipment == NULL)
                pSubstation[n - 1].Equipment = &pEquipment[m];
            else
                pEquipment[m - 1].Next = &pEquipment[m];
            m++;
            break;

        case 2:
            sscanf(TextLine, "%*3s %[ -Z]", &pSubstation[n - 1].Switchgear);
            break;

        case 3:
            sscanf(TextLine, "%*3s %[ -Z]", &pSubstation[n - 1].TapChanger);
            break;

        case 4:
            sscanf(TextLine, "%*3s %d %d %d %d", &ProbTrip, &ProbReclose,
                   &ProbCursor, &JitterFactor);
            break;

        default:
            break;
        }
    }

    free(pTransformers);

    fclose(File);
}

/*****************************************************************************/

void SaveData()
{
    int n;
    FILE *File;
    Equipment *pEqu;

    File = fopen("CADEC.SAV", "w");

    if (File == NULL)
    {
        DisplayError("CADEC DATA FILE NOT FOUND");
        return;
    }

    fprintf(File, "#\tCADEC EQUIPMENT\n#\n");
    fprintf(File, "VAL\t%d\t%d\t%d\t%d\n", ProbTrip, ProbReclose,
            ProbCursor, JitterFactor);

    for (n = 0; n < nSubstation; n++)
    {
        fprintf(File, "#\n#\t%s\n#\nSUB\t%s\t%s\n#\n",
                pSubstation[n].Spens, pSubstation[n].Spens,
                pSubstation[n].Name);

        fprintf(File, "SWG\t%s\nTAP\t%s\n#\n",
                pSubstation[n].Switchgear,
                pSubstation[n].TapChanger);

        pEqu = pSubstation[n].Equipment;

        while (pEqu != NULL)
        {
            fprintf(File, "EQU\t%s\t%04X\t%d\t%d\t%s\n",
                    pEqu->Spens, pEqu->Status, pEqu->Analog[0],
                    pEqu->Operns, pEqu->Name);

            pEqu = pEqu->Next;
        }
    }

    fclose(File);
}

/*****************************************************************************/

void LoadAlarms()
{
    int i, n;
    FILE *File;
    char TextLine[96],
        SubSpens[6],
        EquSpens[4];
    Equipment *pNext;

    pAlarmList = malloc(sizeof(AlarmList) * 64);
    if (pAlarmList == NULL)
    {
        DisplayError("NOT ENOUGH MEMORY");
        return;
    }

    File = fopen("ALARMS.LOG", "r");

    if (File == NULL)
    {
        DisplayError("CADEC ALARMS FILE NOT FOUND");
        return;
    }

    n = 0;

    while (fgets(TextLine, 95, File) != NULL)
    {
        if (TextLine[0] == '#')
            continue;

        sscanf(TextLine, "%ld %4s %2s %24[ -Z]",
               &pAlarmList[n].DateTime, &SubSpens, &EquSpens,
               &pAlarmList[n].Alarm);

        for (i = 0; i < nSubstation; i++)
            if (strcmp(SubSpens, pSubstation[i].Spens) == 0)
                break;

        if (i == nSubstation)
            continue;

        pAlarmList[n].Substation = &pSubstation[i];

        pNext = pAlarmList[n].Substation->Equipment;

        while (pNext != NULL)
        {
            if (strcmp(EquSpens, pNext->Spens) == 0)
                break;

            pNext = pNext->Next;
        }

        if (pNext == NULL)
            continue;

        pAlarmList[n].Equipment = pNext;

        n++;
    }

    fclose(File);

    nAlarms = n;
    for (n = 0; n < nAlarms; n++)
        pAlarmList[n].Number = nAlarms - n;

    AlarmNumber = nAlarms + 1;
}

/*****************************************************************************/

void SaveAlarms()
{
    int n;
    FILE *File;

    File = fopen("ALARMS.LOG", "w");

    if (File == NULL)
    {
        DisplayError("CADEC ALARMS FILE NOT FOUND");
        return;
    }

    fprintf(File, "#\tCADEC ALARMS\n#\n");

    for (n = 0; n < nAlarms; n++)
        fprintf(File, "%ld\t%s\t%s\t%s\n",
                pAlarmList[n].DateTime,
                pAlarmList[n].Substation->Spens,
                pAlarmList[n].Equipment->Spens,
                pAlarmList[n].Alarm);

    fclose(File);
}

/*****************************************************************************/

void LoadNetwork()
{
    int m, n;
    FILE *File;
    char item[16],
        buffer[96];

    File = fopen("CADEC.NET", "r");
    if (File == NULL)
    {
        DisplayError("NETWORK FILE NOT FOUND");
        return;
    }

    while (fgets(buffer, 95, File) != NULL)
    {
        if (buffer[0] == '#')
            continue;

        sscanf(buffer, "%3s", &item);

        if (strcmp(item, "CON") == 0)
            nConcentrator++;

        if (strcmp(item, "RTU") == 0)
            nOutstation++;
    }

    pConcentrator = malloc(sizeof(Concentrator) * nConcentrator);
    if (pConcentrator == NULL)
    {
        DisplayError("NOT ENOUGH MEMORY");
        fclose(File);
    }

    pOutstation = malloc(sizeof(Outstation) * nOutstation);
    if (pOutstation == NULL)
    {
        DisplayError("NOT ENOUGH MEMORY");
        fclose(File);
    }

    rewind(File);

    m = n = 0;
    while (fgets(buffer, 95, File) != NULL)
    {
        if (sscanf(buffer, "CON %24[ -Z]", pConcentrator[m].Name) == 1)
        {
            pConcentrator[m].Outstation = NULL;

            m++;
        }

        if (sscanf(buffer, "RTU %24[ -Z]", pOutstation[n].Name) == 1)
        {
            pOutstation[n].Next = NULL;

            if (pConcentrator[m - 1].Outstation == NULL)
                pConcentrator[m - 1].Outstation = &pOutstation[n];

            else
                pOutstation[n - 1].Next = &pOutstation[n];

            n++;
        }
    }

    fclose(File);
}
/*****************************************************************************/

void InitDisplay()
{
#ifndef TESTING

    textmode(C80);
    SetColours(BLACK, WHITE);
    clrscr();

    cputs("RSX11M V4.6 BL42");
    sleep(1);

    cputs(" 4096K");
    sleep(1);

    cputs("\r\n\n>ASN SY0: DU0:");
    sleep(1);

    cputs("\r\n>MOU SY0:");
    sleep(1);

    cputs("\r\n>@SY0:[1,2]STARTUP.CMD");
    sleep(1);

    cputs("\r\nSTARTING NETWORK");
    sleep(1);

    cputs("\r\nMOUNTING DATABASE");
    sleep(2);

    cputs("\r\nSYNCHRONIZING CLOCKS");
    sleep(1);

    cputs("\r\nSTARTING CADEC SYSTEM");
    sleep(2);

#endif

    textmode(C4350);
    SetColours(WHITE, BLACK);
    clrscr();
    sleep(1);

    SetColours(GREEN, BLACK);
    gotoxy(1, 47);
    cputs("DESK 1");

    SetColours(BLACK, RED);
    gotoxy(56, 47);
    cputs("GLOBAL");

    SetColours(GREEN, BLACK);
    gotoxy(63, 47);
    cputs("CONTROL");

    SetColours(BLACK, RED);
    gotoxy(77, 47);
    cputs("0");
    SetColours(BLACK, GREEN);
    cputs("0");
    SetColours(BLACK, BLUE);
    cputs("1");
}

/*****************************************************************************/

void DisplayCadec()
{
    int i;

    DisplayHeader("CADEC");

    gotoxy(71, 7);
    SetColours(GREEN, BLACK);
    cputs("PAGE 1");

    SetColours(GREEN, BROWN);
    for (i = 17; i <= 37; i++)
    {
        gotoxy(1, i);
        clreol();
    }

    SetColours(WHITE, BLACK);
    for (i = 17; i <= 37; i++)
    {
        gotoxy(80, i);
        clreol();
    }

    SetColours(GREEN, BROWN);
    gotoxy(8, 20);
    cputs("лллллллллллл    лллллллл    лллллллллллл  лллллллллл  лллллллллллл");
    gotoxy(8, 21);
    cputs("лллллллллллл    лллллллл    лллллллллллл  лллллллллл  лллллллллллл");
    gotoxy(8, 22);
    cputs("лл        лл    лл    лл    лл        лл  лл          лл        лл");
    gotoxy(8, 23);
    cputs("лл        лл    лл    лл    лл        лл  лл          лл        лл");
    gotoxy(8, 24);
    cputs("лл              лл    лл    лл        лл  лл          лл");
    gotoxy(8, 25);
    cputs("лл              лл    лл    лл        лл  лл          лл");
    gotoxy(8, 26);
    cputs("ллл            лллллллллл   ллл       лл  лллллллллл  ллл");
    gotoxy(8, 27);
    cputs("лллл          лллллллллллл  лллл      лл  лллллллллл  лллл");
    gotoxy(8, 28);
    cputs("лллл          лллл      лл  лллл      лл  лллл        лллл");
    gotoxy(8, 29);
    cputs("лллл          лллл      лл  лллл      лл  лллл        лллл");
    gotoxy(8, 30);
    cputs("лллл          лллл      лл  лллл      лл  лллл        лллл");
    gotoxy(8, 31);
    cputs("лллл      лл  лллл      лл  лллл      лл  лллл        лллл      лл");
    gotoxy(8, 32);
    cputs("лллл      лл  лллл      лл  лллл      лл  лллл        лллл      лл");
    gotoxy(8, 33);
    cputs("лллллллллллл  лллл      лл  лллллллллллл  лллллллллл  лллллллллллл");
    gotoxy(8, 34);
    cputs("лллллллллллл  лллл      лл  лллллллллллл  лллллллллл  лллллллллллл");

    gotoxy(21, 12);
    SetColours(YELLOW, BLACK);
    cputs("CADEC -- COPYRIGHT (C) 1997 BILL FARMER");

    gotoxy(1, 1);
    SetColours(WHITE, BLACK);
}

/*****************************************************************************/

void SetColours(int ForeGround, int BackGround)
{
    textcolour(ForeGround, BackGround);
}

/*****************************************************************************/
