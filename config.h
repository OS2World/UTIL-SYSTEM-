/*
    CONFIG.H    Miscellaneous constants and function 
                prototypes for CONFIG.C
    Copyright (C) 1989 Ray Duncan
*/

#define TRUE    1                           // logical values
#define FALSE   0

#define WAIT    0                           // parameters for KbdCharIn 
#define NOWAIT  1

#define VISIBLE 0                           // parameters for VioSetCurType
#define HIDDEN  -1  

#define BROWSE  0                           // possible program "modes"
#define EDNAME  1
#define EDVAL   2

#define ESC     0x1b                        // normal keycodes
#define BS      0x08
#define TAB     0x09
#define ENTER   0x0d
#define TILDE   0x7e
#define BL      0x20

#define UP      0x48                        // extended key codes 
#define DOWN    0x50                    
#define RIGHT   0x4d
#define LEFT    0x4b
#define HOME    0x47
#define END     0x4f
#define CTRLEND 0x75
#define INS     0x52
#define DEL     0x53
#define PGUP    0x49
#define PGDN    0x51
#define BACKTAB 0x0f

#define VBAR    0xb3                        // graphics characters
#define HBAR    0xc4
#define UPARROW 0x18
#define DNARROW 0x19
#define RTARROW 0x1a
#define LTARROW 0x1b

#define MAXLN   256                         // maximum number of lines
                                            // allowed in CONFIG.SYS

#define F0COL   0                           // field 0 starting column
#define F1COL   6                           // field 1 starting column
#define F2COL   21                          // field 2 starting column

#define API unsigned extern far pascal      // OS/2 function prototypes

API DosBeep(int, unsigned);
API DosGetInfoSeg(unsigned far *, unsigned far *);
API DosGetResource(unsigned, int, int, unsigned far *);
API DosSizeSeg(unsigned, unsigned long far *);
API KbdCharIn(void far *, unsigned, unsigned);
API VioGetCurType(void far *, unsigned);
API VioGetMode(void far *, unsigned);
API VioScrollUp(int, int, int, int, int, unsigned far *, unsigned);
API VioScrollDn(int, int, int, int, int, unsigned far *, unsigned);
API VioSetCurType(void far *, unsigned);
API VioSetCurPos(int, int, unsigned);
API VioReadCharStr(char far *, int far *, int, int, unsigned);
API VioWrtCharStr(char far *, int, int, int, unsigned);
API VioWrtCharStrAtt(char far *, int, int, int, char far *, unsigned);
API VioWrtNAttr(char far *, int, int, int, unsigned);
API VioWrtTTY(char far *, int, unsigned);

void addline(void);                         // local function prototypes
int  ask(char *);
void blap(void);
void blip(void);
void changeline(void);
void changename(void);
void changeval(void);
int  chkname(char *);
void cls(void);
void deleteline(void);
int  editfile(void);
void errexit(char *);
void highlite(int);
void insertline(void);
void linedown(void);
void lineup(void);
void makestrings(void);
void newline(void);
void pagedown(void);
void pageup(void);
void readfile(void);
void restore(void);
void setcurpos(int, int);
void setcurtype(int);
void showhelp(int);
void showline(int, int);
void showpage(void);
void showstatus(char *p);
void signon(void);
void writefile(void);

