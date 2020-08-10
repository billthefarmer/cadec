/* cadec.h */

/*****************************************************************************/

#undef DOS
#undef TESTING

#define	textcolour	textcolor

#define	TIMEOUT		300
#define	PULSETIME	2
#define RECLOSETIME	20
#define	CONFIRMTIME	30
#define	DINGDONGTIME	30

#define	NORMAL		0
#define	OPEN		1
#define	CLOSE		2
#define	DINGDONG	3

/*****************************************************************************/

enum Boolean
    {FALSE, TRUE};

/*****************************************************************************/

enum StatusBits
    {
     StatusOpen	= 1,
     StatusClosed	= 2,
     StatusProtOff	= 4,
     StatusProtOn	= 8,
     StatusAutoOff	= 16,
     StatusAutoOn	= 32,
     StatusTripped	= 64,
     StatusEfiAlarm	= 128,
     StatusProtAlarm	= 256,
     StatusLockout	= 512,
     StatusExtInput	= 1024,
     StatusExternal	= 2048,
     StatusTrippable	= 4096,
     StatusProtection = 8192,
     StatusAutomation = 16384
    };

/*****************************************************************************/

enum Codes
    {Index,	// [F1 ]	GIN
     Directory,	// [F2 ]	SSD
     Substat,	// [F3 ]	SST
     Events,	// [F4 ]	EVT
     Alarms,	// [F5 ]	ALM
     Accept,	// [F6 ]	ALA
     Open,	// [F7 ]	OPE
     Close,	// [F8 ]	CLS
     Auton,	// [F8 ]	AON
     Autoff,	// [F7 ]	AOF
     Proton,	// [F8 ]	PON
     Protoff,	// [F7 ]	POF
     Refresh,	// [F11]	RFR
     Cancel,	// [F12]	DCN
     Pageback,	//		PGB
     Pageon,	//		PGO
     Find,	// [F9 ]	FND
     Status,	//		SYS
     Parameters,//		SPR
     Trip,	//		TRP
     Efi,	//		EFI
     Analog,	//		ANA
     Operns,	//		OPS
     Probreclose,//		PSR
     Probcursor,//		PPC
     Probtrip,	//		PTR
     Drawing,	// [F10]	DRW
     Abnormal,	//		SAR
     Setstatus,	//		ZAP
     Setjitter,	//		SJF
     Setserial,	//		SSP
     Setfrequency,//		SFR
     Setexternal,//		EXT
     Data,	// [F9 ]	EDB
     Edit,	// [F10]	EDR
     Conc,	//		DCS
     Cadec,	//		CAD
     Reset,	//		RES
     Stop,	//		STP
     Start,	//		STA
     Exit,	//		SHT
     Init};

/*****************************************************************************/

typedef struct _Equipment
{
    char Spens[4];
    char Name[24];
    short Status;
    short Analog[2];
    short Operns;
    short Count;
    void *Trans;
    void *Next;
} Equipment;

/*****************************************************************************/

typedef struct _Coords
{
    Equipment *Equipment;
    short  xEqu;
    short  yEqu;
    short  xAna;
    short  yAna;
} Coords;

/*****************************************************************************/

typedef struct _Substation
{
    char  Spens[6];
    char  Name[24];
    char  Switchgear[16];
    char  TapChanger[16];
    Equipment *Equipment;
} Substation;

/*****************************************************************************/

typedef struct _CurrentDisplay
{
    Substation *Substation;
    Equipment *Equipment;
    Equipment *Next;
    int  Function;
    int  Display;
    int  Page;
} CurrentDisplay;

/*****************************************************************************/

typedef struct _Transformers
{
    Equipment *Trans[4];
} Transformers;

/*****************************************************************************/

typedef struct _AlarmList
{
    time_t  DateTime;
    int  Number;
    Substation *Substation;
    Equipment *Equipment;
    char  Alarm[26];
} AlarmList;

/*****************************************************************************/

typedef struct _Concentrator
{
    char Name[24];
    void *Outstation;
} Concentrator;

/*****************************************************************************/

typedef struct _Outstation
{
    char Name[24];
    void *Next;
} Outstation;

/*****************************************************************************/

extern char *ProgramName,
    Command[],
    *CommandList[],
    *ItemList[],
    *CommandWords[];

/*****************************************************************************/

extern int CommandIndex,
    CommandNumber,
    nSubstation,
    nEquipment,
    nAlarms,
    TimeOut,
    Stopit,
    AlarmNumber,
    Frequency,
    ProbTrip,
    ProbReclose,
    ProbCursor,
    JitterFactor,
    SerialPort;

/*****************************************************************************/

extern Substation *pSubstation;
extern Equipment *pEquipment;
extern Coords  *pCoords;
extern Transformers *pTransformers;
extern CurrentDisplay *pCurrentDisplay;
extern AlarmList *pAlarmList;

/*****************************************************************************/

char *GetScreenStr(int x, int y);

/*****************************************************************************/

int DisplayConfirm(void),
    Namecmp(Substation *s, Substation *t),
    Spenscmp(Substation *s, Substation *t),
    GetScreenChar(void);

/*****************************************************************************/

void main(int argc, char *argv[]),
    DoCadec(void),
    LoadData(void),
    DataLoad(FILE *File),
    SaveData(void),
    LoadAlarms(void),
    SaveAlarms(void),
    LoadNetwork(void),
    ProcessKey(void),
    ProcessTime(time_t t),
    DisplayTime(time_t t),
    ProcessTrip(void),
    ProcessAutoReclose(void),
    ProcessDingdong(void),
    DoReclose(Substation *s, Equipment *e),
    DisplayHeader(char *h),
    DisplayBanner(void),
    DisplayStatus(void),
    DisplayParameters(void),
    DisplayPageon(void),
    DisplayPageback(void),
    DisplayExit(void),
    DisplayIndex(void),
    IndexDisplay(void),
    DisplayDirectory(void),
    DisplaySubstation(void),
    SubstationDisplay(void),
    SubstationOne(void),
    SubstationTwo(void),
    SubstationThree(void),
    DisplayDrawing(void),
    DrawingDisplay(void),
    DisplayData(void),
    DataDisplay(void),
    DataOne(void),
    DataTwo(void),
    DisplayOpen(void),
    DisplayClose(void),
    DisplayProtOn(void),
    DisplayProtOff(void),
    DisplayAutOn(void),
    DisplayAutOff(void),
    DisplayTrip(void),
    DisplayEfi(void),
    SetAnalogue(void),
    SetOperations(void),
    SetStatus(void),
    SetFrequency(void),
    LogEvent(char *p, Substation *s, Equipment *e, char *t),
    DisplayEvents(void),
    EventsDisplay(void),
    DisplayFind(void),
    LogAlarm(Substation *s, Equipment *e, char *a),
    DisplayAlarms(void),
    AlarmsDisplay(void),
    DisplayAccept(void),
    DisplayRefresh(void),
    RefreshDisplay(void),
    DisplayReset(void),
    DisplayStop(void),
    DisplayStart(void),
    DisplayCancel(void),
    DisplayAbnormal(void),
    ClearDisplay(void),
    ClearCommand(void),
    ProcessCommand(void),
    SetProbTrip(void),
    SetProbReclose(void),
    SetProbCursor(void),
    SetJitterFactor(void),
    SetSerialPort(void),
    SetExternal(void),
    DisplayPurpleCursor(void),
    AdjustAnalogues(void),
    ProcessWords(void),
    DisplayConc(void),
    DisplayCadec(void),
    DisplayError(char *e),
    InitDisplay(void),
    InitExternal(void),
    ExternalOperation(int o),
    ProcessStatusLine(),
    ProcessPosition(int x, int y, char *s),
    ProcessString(int n),
    EditDrawing(void),
    DrawingEdit(int x, int y),
    UpdateEquipment(char *Equ, int x, int y),
    UpdateAnalog(char *Equ, int x, int y),
    SaveDrawing(void),
    ShowPosition(int x, int y),
    SetColours(int f, int b);

/*****************************************************************************/
