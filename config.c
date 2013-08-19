/*
    CONFIG.C    OS/2 System Configuration Utility
    Copyright (C) 1989 Ziff Davis Communications
    PC Magazine * Ray Duncan
 
    This is the "long" version with on-line help.
 
    Compile:    MAKE CONFIG
 
    Usage:      CONFIG <Enter>              edits CONFIG.SYS in root
                                            directory of boot drive
                or
 
                CONFIG <Filename> <Enter>   edits specified file
*/
 
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include <malloc.h>
 
#include "config.h"
#include "confhelp.h"
 
char iname[80];                             // name of input file
char oname[80];                             // name of output file
FILE *ifile;                                // input file stream
FILE *ofile;                                // output file stream
 
struct _kbdinfo {                           // KbdCharIn info structure
    unsigned char charcode;                 // ASCII character code
    unsigned char scancode;                 // keyboard scan code
    char status;                            // misc. status flags
    char reserved1;                         // reserved byte
    unsigned shiftstate;                    // keyboard shift state
    long timestamp;                         // character timestamp
    } kbdinfo;
 
struct _modeinfo {                          // display mode information
    int len;                                // length of structure
    char type;                              // display type
    char colors;                            // available colors
    int cols;                               // width of screen
    int rows;                               // height of screen
    } modeinfo;
 
struct _curinfo {                           // cursor information
    int start;                              // starting line
    int end;                                // ending line
    int width;                              // width
    unsigned attr;                          // attribute (visible/hidden)
    } curinfo;
 
struct _directives {
    char *name;                             // directive name
    unsigned helpid;                        // help text resource ID
    } directives [] = {
    "",             IDH_GENERAL,
    "BREAK",        IDH_BREAK,
    "BUFFERS",      IDH_BUFFERS,
    "CODEPAGE",     IDH_CODEPAGE,
    "COUNTRY",      IDH_COUNTRY,
    "DEVICE",       IDH_DEVICE,
    "DEVINFO",      IDH_DEVINFO,
    "DISKCACHE",    IDH_DISKCACHE,
    "FCBS",         IDH_FCBS,
    "IFS",          IDH_IFS,
    "IOPL",         IDH_IOPL,
    "LIBPATH",      IDH_LIBPATH,
    "MAXWAIT",      IDH_MAXWAIT,
    "MEMMAN",       IDH_MEMMAN,
    "PAUSEONERROR", IDH_PAUSEONERROR,
    "PRIORITY",     IDH_PRIORITY,
    "PROTSHELL",    IDH_PROTSHELL,
    "PROTECTONLY",  IDH_PROTECTONLY,
    "REM",          IDH_REM,
    "RMSIZE",       IDH_RMSIZE,
    "RUN",          IDH_RUN,
    "SET",          IDH_SET,
    "SHELL",        IDH_SHELL,
    "SWAPPATH",     IDH_SWAPPATH,
    "THREADS",      IDH_THREADS,
    "TIMESLICE",    IDH_TIMESLICE,
    "TRACE",        IDH_TRACE,
    "TRACEBUF",     IDH_TRACEBUF, };
 
int DIRMAX = sizeof(directives)/sizeof(directives[0]);
 
int totln;                                  // total lines in file
int curln;                                  // line no. being edited
char *ln[MAXLN][2];                         // pointers to parsed directives
char namestr[16];                           // working copy of name string
char valstr[256];                           // working copy of value string
int dirix;                                  // index to 'directives' array
int valix;                                  // index to value string
 
int insflag;                                // TRUE=insert, FALSE=overwrite
int edmode;                                 // BROWSE, EDNAME, or EDVAL
 
int curx;                                   // cursor X coordinate
int cury;                                   // cursor Y coordinate
int lpp;                                    // lines per page
int lps;                                    // total rows on screen
int cpl;                                    // columns per line
int cstart;                                 // original cursor start line
 
unsigned normcell = 0x0720;                 // blank cell, normal video
unsigned revcell = 0x7020;                  // blank cell, reverse video
char normattr = 0x07;                       // normal (white on black)
char revattr  = 0x70;                       // reverse (black on white)
 
char *copyright = "OS/2 Configuration Editor (C) 1989 Ziff Davis";
 
char format1[80];                           // format string for directives
char format2[80];                           // format for filename line
 
char helpstr1[80];                          // help string, browse mode
char helpstr2[80];                          // help string, name editing
char helpstr3[80];                          // help string, value editing
char *help[] = { helpstr1, helpstr2, helpstr3 };
 
main(int argc, char *argv[])
{
    unsigned gseg, lseg;                    // info segment selectors
    char far *ginfo;                        // global info segment pointer
 
    if(argc < 2)                            // get name of file to edit
    {                                       // else \CONFIG.SYS boot drive
        strcpy(iname, "c:\\config.sys");
        DosGetInfoSeg(&gseg, &lseg);        // get info segment selectors
        (long) ginfo = (long) gseg << 16;   // make far pointer
        iname[0] = ginfo[0x24]+'a'-1;       // boot drive into filename
    }
    else strcpy(iname, argv[1]);
 
    VioGetCurType(&curinfo, 0);             // get cursor information
    cstart = curinfo.start;                 // save cursor start line
 
    modeinfo.len = sizeof(modeinfo);        // get display information
    VioGetMode(&modeinfo, 0);
    lps = modeinfo.rows;                    // rows on screen
    lpp = modeinfo.rows-3;                  // lines per editing page
    cpl = modeinfo.cols;                    // columns per line
 
    if(cpl < 80)                            // reject 40 column modes
        errexit("Use display mode with at least 80 columns.");
 
    makestrings();                          // build format strings
 
    readfile();                             // read CONFIG.SYS file
 
    signon();                               // display sign-on info
 
    if(editfile())                          // edit in-memory info
 
        writefile();                        // write modified file
 
    cls();                                  // clear screen and exit
}
 
/*
    Edit the parsed in-memory copy of the CONFIG.SYS file.
*/
int editfile(void)
{
    edmode = BROWSE;                        // initialize editing mode
    setcurtype(edmode);                     // initialize cursor state
    showpage();                             // display first page
    showstatus(help[edmode]);               // and help line
 
    while(! KbdCharIn(&kbdinfo, WAIT, 0))
    {
        switch(toupper(kbdinfo.charcode))
        {
            case 'A':                       // add new line at
                addline();                  // end of file
                break;
 
            case 'C':                       // change current line
                changeline();
                break;
 
            case 'D':                       // delete current line
                deleteline();
                break;
 
            case 'I':                       // insert new line at
                insertline();               // current line
                break;
 
            case 'R':                       // revert to original file
                if(ask("Revert to original file")) restore();
                break;
 
            case 'X':                       // exit, write file
                if(ask("Write modified file and exit")) return(TRUE);
                break;
 
            case ESC:                       // exit, don't write file
                if(ask("Quit without writing file")) return(FALSE);
                break;
 
            case 0:                         // extended keycodes
            case 0xe0:
                switch(kbdinfo.scancode)
                {
                    case HOME:              // go to start of file
                        curln = 0;
                        cury = 0;
                        showpage();
                        break;
 
                    case UP:                // move one line towards
                        lineup();           // start of file
                        break;
 
                    case DOWN:              // move one line towards
                        linedown();         // end of file
                        break;
 
                    case PGUP:              // move one page towards
                        pageup();           // start of file
                        break;
 
                    case PGDN:              // move one page towards
                        pagedown();         // end of file
                        break;
 
                    case END:               // go to end of file
                        curln = totln-1;
                        cury = min(lpp-1, totln-1);
                        showpage();
                        break;
 
                    default:                // unrecognized extended key
                        blap();
                }
                break;
 
            default:                        // unrecognized normal key
                blap();
        }
    }
}
 
/*
    Add a new line at the end of file, allocating heap space for
    two null strings: the directive's name and its value.
*/
void addline(void)
{
    if(totln == MAXLN)                      // don't allow add operation
    {                                       // if line array already full
        blap();
        return;
    }
 
    curln = totln++;                        // move to end of file
    newline();                              // create an empty line
    cury = min(totln-1, lpp-1);             // adjust cursor position
    showpage();                             // update display
    changeline();                           // now edit the new line
}
 
/*
    Insert a new (empty) line before the current line, allocating heap
    space for two null strings: the directive's name and its value.
*/
void insertline(void)
{
    int i;                                  // scratch variable
 
    if(totln == MAXLN)                      // don't allow insert if
    {                                       // line array already full
        blap();
        return;
    }
 
    if(totln == 0)                          // if empty file, use
    {                                       // the add new line
        addline();                          // routine instead
        return;
    }
 
    for(i = totln-1; i >= curln; i--)       // OK to insert...
    {
        ln[i+1][0] = ln[i][0];              // move current line and
        ln[i+1][1] = ln[i][1];              // subsequent lines down
    }
 
    totln++;                                // bump total line count
    newline();                              // current becomes empty line
    showpage();                             // update the display
    changeline();                           // now edit the new line
}
 
/*
    Create a new (empty) line.  Heap space is allocated for
    two null strings: the directive's name and its value.
*/
void newline(void)
{                                           // allocate space from heap
    if(((ln[curln][0] = malloc(strlen("")+1)) == NULL) ||
       ((ln[curln][1] = malloc(strlen("")+1)) == NULL))
        errexit("out of heap space.");
 
    strcpy(ln[curln][0], "");               // assign null strings as
    strcpy(ln[curln][1], "");               // initial name and value
}
 
/*
    Change the name and/or value strings of the current line.
*/
void changeline(void)
{
    char buff[256];                         // scratch buffer
    int i;                                  // scratch variable
 
    edmode = EDNAME;                        // start in name field
    insflag = FALSE;                        // insert initially off
 
    memset(valstr, 0, sizeof(valstr));
    strcpy(namestr, ln[curln][0]);          // make local copies of
    strcpy(valstr, ln[curln][1]);           // name and value strings
 
    dirix = chkname(namestr);               // initialize name string index
    valix = 0;                              // initialize value string index
 
    if(dirix<0)                             // if unrecognized name...
    {
        if(ask("Unknown directive.  Erase it"))
        {
            dirix = 0;                      // optionally zap out
            strcpy(namestr, "");            // the current line
            strcpy(valstr, "");
        }
    }
 
    highlite(edmode);                       // position highlight
    showstatus(help[edmode]);               // update help line
 
    while(TRUE)
    {
        i = max(0, valix-(cpl-F2COL-1));    // display current line
        sprintf(buff, format1, curln+1, namestr, &valstr[i]);
        VioWrtCharStr(buff, min(strlen(buff), cpl), cury, 0, 0);
 
        if(edmode == EDVAL)                 // position blinking cursor
        {                                   // if editing value string
            curx = min(F2COL+valix, cpl-1);
            setcurpos(curx, cury);
        }
 
        KbdCharIn(&kbdinfo, WAIT, 0);
 
        switch(kbdinfo.charcode)
        {
            case ESC:                       // Escape key
                if(! ask("Discard changes to this line")) break;
                edmode = BROWSE;
                showline(curln, cury);      // restore previous text
                setcurtype(edmode);         // hide blinking cursor
                highlite(edmode);           // highlight entire line
                showstatus(help[edmode]);   // display help line
                return;
 
            case ENTER:                     // Enter key
                if(! ask("Accept changes to this line")) break;
                edmode = BROWSE;
 
                                            // copy new text to heap
                ln[curln][0] = realloc(ln[curln][0], strlen(namestr)+1);
                ln[curln][1] = realloc(ln[curln][1], strlen(valstr)+1);
                if((ln[curln][0] == NULL) || (ln[curln][1] == NULL))
                    errexit("out of heap space.");
                strcpy(ln[curln][0], namestr);
                strcpy(ln[curln][1], valstr);
 
                showline(curln, cury);      // restore previous text
                setcurtype(edmode);         // hide blinking cursor
                highlite(edmode);           // highlight entire line
                showstatus(help[edmode]);   // display help line
                return;
 
            case 'H':                       // H or h key
            case 'h':
                showhelp(dirix);            // load and display help
                break;                      // text resource
 
            case TAB:                       // tab key
                if((edmode == EDVAL) || (dirix <= 0)) blap();
                else                        // if not there already
                {
                    edmode = EDVAL;         // move to value field
                    valix = 0;              // init. index to string
                    curx = F2COL;           // set cursor position
                    setcurpos(curx, cury);
                    setcurtype(edmode);     // and cursor appearance
                    highlite(edmode);       // highlight value field
                    showstatus(help[edmode]);   // display help info
                }
                break;
 
            case 0:                         // extended key code
            case 0xe0:
                if(kbdinfo.scancode == BACKTAB)
                {
                    if(edmode == EDNAME) blap();    // backtab key
                    else
                    {
                        valix = 0;          // left-align value string
                        edmode = EDNAME;    // move to name field
                        setcurtype(edmode); // hide blinking cursor
                        highlite(edmode);   // highlight name field
                        showstatus(help[edmode]); // display help info
                    }
                    break;
                }
 
            default:                        // other keys field-specific
                if(edmode == EDNAME) changename();
                else changeval();
        }
    }
}
 
/*
    Handle editing of directive's name field only.
*/
void changename(void)
{
    if((kbdinfo.charcode == 0) || (kbdinfo.charcode == 0xe0))
    {
        switch(kbdinfo.scancode)
        {
            case DOWN:                      // advance to next
                dirix++;                    // directive name
                if(dirix == DIRMAX) dirix = 0;
                strcpy(namestr, directives[dirix].name);
                break;
 
            case UP:                        // back up to previous
                dirix--;                    // directive name
                if(dirix <0) dirix = DIRMAX - 1;
                strcpy(namestr, directives[dirix].name);
                break;
 
            default:                        // unrecognized extended key
                blap();
        }
    }
    else blap();                            // unrecognized normal key
}
 
/*
    Handle editing of directive's value string only.
*/
void changeval(void)
{
    int vlen = strlen(valstr);              // get current length
 
    while((vlen > 0) && (valstr[vlen-1] == BL))
        valstr[--vlen] == 0;                // trim trailing blanks
 
    if((kbdinfo.charcode == 0) || (kbdinfo.charcode == 0xe0))
    {
        switch(kbdinfo.scancode)            // extended key?
        {
            case RIGHT:                     // move right one character
                valix = min(valix++, vlen);
                break;
 
            case LEFT:                      // move left one character
                valix = max(valix--, 0);
                break;
 
            case HOME:                      // go to start of line
                valix = 0;
                break;
 
            case END:                       // go to end of line
                valix = vlen;
                break;
 
            case CTRLEND:                   // clear to end of line
                memset(&valstr[valix], 0, sizeof(valstr)-valix);
                break;
 
            case INS:                       // toggle insert flag
                insflag ^= TRUE;
                setcurtype(edmode);
                break;
 
            case DEL:                       // delete char under cursor
                strcpy(&valstr[valix], &valstr[valix+1]);
                break;
 
            default:                        // unrecognized extended key
                blap();
        }
    }
    else if((kbdinfo.charcode >= BL) &&     // printable ASCII character
            (kbdinfo.charcode <= TILDE) &&  // and string not full?
            (valix < sizeof(valstr)-1) &&
            !(insflag && (vlen > (sizeof(valstr)-2))))
    {
        if(insflag)                         // if insert on, open up string
            memmove(&valstr[valix+1], &valstr[valix], sizeof(valstr)-valix-1);
        valstr[valix++] = kbdinfo.charcode; // store new character
    }
    else if((kbdinfo.charcode == BS) &&     // backspace key and not
            (valix != 0))                   // at start of line?
    {
        valix--;                            // back up one character
        if(insflag)
            memmove(&valstr[valix], &valstr[valix+1], sizeof(valstr)-valix-1);
        else valstr[valix] = BL;
    }
    else blap();                            // unrecognized normal key
}
 
/*
    Delete the current line, also releasing its heap space.
*/
void deleteline(void)
{
    int i;                                  // scratch variable
 
    if(totln == 0)                          // bail out if no lines exist
    {
        blap();
        return;
    }
 
    free(ln[curln][0]);                     // give back heap memory
    free(ln[curln][1]);
 
    for(i = curln; i < (totln-1); i++)
    {
        ln[i][0] = ln[i+1][0];              // close up array of
        ln[i][1] = ln[i+1][1];              // pointers to parsed lines
    }
 
    totln--;                                // adjust total line count
 
    curln = max(0, min(curln, totln-1));    // ensure current line valid
 
    if(totln < lpp)                         // adjust current line and
    {                                       // cursor position as needed
        curln = min(curln, totln-1);
        cury = min(cury, totln-1);
    }
    else if((totln-curln) < (lpp-cury))
    {
        cury = min(cury+1, lpp-1);
    }
 
    showpage();                             // update the display
}
 
/*
    Restore in-memory copy of CONFIG.SYS file to its original state.
*/
void restore(void)
{
    int i;                                  // scratch variable
 
    for(i = 0; i < totln-1; i++)
    {
        free(ln[i][0]);                     // give back heap memory
        free(ln[i][1]);
    }
 
    readfile();                             // re-read the original file
    showpage();                             // update the display
}
 
/*
    Search the directives table to match the given name.
    Return an index to the table if the directive name is
    valid, or -1 if the name can't be matched.
*/
int chkname(char *p)
{
    int i;
 
    for(i = 0; i < DIRMAX; i++)
    {
        if(! strcmp(directives[i].name, p)) return i;
    }
    return(-1);
}
 
/*
    Read the CONFIG.SYS file into memory, parsing each line
    into its two components: the directive's name and the value.
*/
void readfile(void)
{
    char buff[256];                         // temporary input buffer
    char *p, *q;                            // scratch pointers
 
    totln = 0;                              // initialize line count
    curln = 0;                              // initialize curln line
    cury = 0;                               // initialize cursor position
 
    if((ifile = fopen(iname, "r+"))==NULL)  // open input file
        errexit("file not found or read-only.");
 
    while(fgets(buff, 256, ifile) != NULL)  // read next line
    {
        if(totln == MAXLN)
            errexit("too many lines in file.");
 
        p = strtok(buff, " =\x0a");         // parse out the name
        q = strtok(NULL, "\x0a");           // get pointer to remainder
 
        if(strcmp(strupr(p), "REM"))        // if not REM scan off
        {                                   // leading delimiters
            while((*q == ' ') || (*q == '=')) q++;
        }
                                            // allocate heap space
        if(((ln[totln][0] = malloc(strlen(p)+1)) == NULL) ||
           ((ln[totln][1] = malloc(strlen(q)+1)) == NULL))
            errexit("out of heap space.");
 
        strcpy(ln[totln][0], p);            // copy parsed directive name
        strcpy(ln[totln++][1], q);          // and value to the heap
    }
 
    fclose(ifile);                          // close input file
}
 
/*
    Write modified file.
*/
void writefile(void)
{
    int i;                                  // scratch variables
    char c, *p;
 
    showstatus("Writing file...");
 
    strcpy(oname, iname);                   // build temporary filename
    if(! (p = strchr(oname, '.')))          // clip off old extension
        p = strchr(oname, '\0');            // (if any) and add .$$$
    strcpy(p, ".$$$");
 
    if((ofile = fopen(oname, "w"))==NULL)   // open output file
        errexit("can't create output file.");
 
    for(i = 0; i < totln; i++)              // write next line
    {
        if((! strcmp(ln[i][0], "REM")) ||   // use = delimiter unless
           (! strcmp(ln[i][0], "SET")) ||   // REM or SET directive
           (strlen(ln[i][0]) == 0))
             c = ' ';
        else c = '=';
 
        if(! fprintf(ofile, "%s%c%s\n", ln[i][0], c, ln[i][1]))
            errexit("can't write file.  Disk full?");
    }
 
    fclose(ofile);                          // close output file
 
    strcpy(p, ".bak");                      // build .BAK filename
    unlink(oname);                          // delete previous .BAK file
    rename(iname, oname);                   // make original file .BAK
    strcpy(p, ".$$$");                      // give new file same name
    rename(oname, iname);                   // as original file
}
 
/*
    Move cursor down one line; if cursor is already at end of screen,
    scroll down through the file until the end of file is reached.
*/
void linedown(void)
{
    if((curln == (totln-1)) || (totln == 0))
    {
        blip();                             // if already at end of
        return;                             // file, do nothing
    }
 
    highlite(-1);                           // hide the highlight bar
    curln++;                                // advance through file
 
    if(cury != (lpp-1))                     // if not at end of screen,
    {                                       // move cursor down
        cury++;
    }
    else                                    // otherwise scroll screen
    {
        VioScrollUp(0, 0, lpp-1, cpl-1, 1, &normcell, 0);
        showline(curln, lpp-1);
    }
 
    highlite(0);                            // display the highlight bar
}
 
/*
    Move cursor up one line; if cursor is already at top of screen,
    scroll up through the file until the start of file is reached.
*/
void lineup(void)
{
    if((curln == 0) || (totln == 0))
    {
        blip();                             // if already at start
        return;                             // of file, do nothing
    }
 
    highlite(-1);                           // hide the highlight bar
    curln--;                                // back up through file
 
    if(cury != 0)                           // if not at start of
    {                                       // screen, move cursor up
        cury--;
    }
    else                                    // otherwise scroll screen
    {
        VioScrollDn(0, 0, lpp-1, cpl-1, 1, &normcell, 0);
        showline(curln, 0);
    }
 
    highlite(0);                            // display the highlight bar
}
 
/*
    Scroll display down by one page.  Cursor is left in same relative
    position on the screen unless we bump into end of file or the
    number of lines in the file is less than the number of lines
    per page.
*/
void pagedown(void)
{
    if((curln == (totln-1)) || (totln == 0))
    {
        blip();                             // if already at end of
        return;                             // file, do nothing
    }
 
    curln += lpp;                           // advance through file
 
    if((curln >= totln) || ((curln+lpp-cury) >= totln))
    {
        curln = totln-1;                    // if already in last page
        cury = min(lpp-1, totln-1);         // force last line, move
    }                                       // cursor to bottom
 
    showpage();                             // rewrite entire screen
}
 
/*
    Scroll display up by one page.  Cursor is left in same relative
    position on the screen unless we bump into start of file or the
    number of lines in the file is less than the number of lines
    per page.
*/
void pageup(void)
{
    if((curln == 0) || (totln == 0))
    {
        blip();                             // if already at start
        return;                             // of file, do nothing
    }
 
    curln -= lpp;                           // back up through file
 
    if((curln <0) || ((curln-cury) < 0))
    {
        curln = 0;                          // if already in first page,
        cury = 0;                           // force cursor to top
    }
 
    showpage();                             // rewrite entire screen
}
 
/*
    Update the entire display.  If there are not enough lines
    to fill the page, just clear the remainder.
*/
void showpage(void)
{
    int i, j;                               // scratch variables
 
    j = min(totln, lpp);                    // find last line to display
 
    for(i = 0; i < j; i++)                  // display all lines
        showline(curln-cury+i, i);
 
    if(j < lpp)                             // clear remainder of screen
        VioScrollUp(j, 0, lpp-1, cpl-1, lpp-j, &normcell, 0);
 
    highlite(BROWSE);                       // highlight current line
}
 
/*
    Display line "l" on the screen on the specified row "y".
*/
void showline(int l, int y)
{
    char buff[256];                         // scratch buffer
 
    sprintf(buff, format1, l+1, ln[l][0], ln[l][1]);
    VioWrtCharStrAtt(buff, min(strlen(buff), cpl), y, 0, &normattr, 0);
}
 
/*
    Display status or help message centered on last line of screen.
*/
void showstatus(char *msg)
{
    VioScrollUp(lpp+2, 0, lpp+2, cpl-1, 1, &normcell, 0);
    VioWrtCharStr(msg, strlen(msg), lpp+2, max(0, (cpl-strlen(msg))/2), 0);
}
 
/*
    Display prompt, accept Y/y/N/n keys only.
    Return TRUE (Y) or FALSE (N) to caller.
*/
int ask(char *msg)
{
    char c, p[80], *q;                      // scratch variables & ptrs
    int i = cpl;                            // length of line to save
 
    q = malloc(cpl+1);                      // allocate some heap space
 
    VioReadCharStr(q, &i, lpp+2, 0, 0);     // save current status line
    q[i] = 0;                               // and make it ASCIIZ
 
    sprintf(p, "%s? (Y/N)", msg);           // format the prompt
 
    do
    {
        blip();                             // wake up the user
        showstatus(p);                      // display prompt
        KbdCharIn(&kbdinfo, WAIT, 0);       // get key from user
        c = toupper(kbdinfo.charcode);
 
    } while((c != 'Y') && (c != 'N'));      // retry if not Y or N
 
    showstatus(q);                          // restore original status line
    free(q);                                // give back heap space
    return(c == 'Y');                       // result is TRUE or FALSE
}
 
/*
    Build miscellaneous formatting and help strings.
*/
void makestrings(void)
{
    sprintf(format1,                        // built format for directives
        "%%%dd %c %%%d.%ds %c %%-%d.%ds",
        F1COL-3,                            // width of first column
        VBAR,                               // graphics divider character
        F2COL-F1COL-3, F2COL-F1COL-3,       // width of second column
        VBAR,                               // graphics divider character
        cpl-F2COL, cpl-F2COL);              // width of third column
 
    sprintf(format2,                        // built format for filename
        "%s  %%%d.%ds",                     // and copyright line
        copyright,
        cpl - strlen(copyright) - 2,
        cpl - strlen(copyright) - 2);
 
    sprintf(helpstr1,"%c %c %s %s",         // build help string for
        UPARROW, DNARROW,                   // browse mode
        "<PgUp> <PgDn> <Home> <End>",
        "Add Change Delete Insert Revert eXit <Esc>");
 
    sprintf(helpstr2,"%c %c %s",            // build help string for
        UPARROW, DNARROW,                   // name string editing
        "<Tab> Help <Enter> <Esc>");
 
    sprintf(helpstr3,"%c %c %s %s",         // build help string for
        LTARROW, RTARROW,                   // value string editing
        "<Home> <End> <Ins> <Del> <Backsp> <Ctrl-End>",
        "<Shift-Tab> Help <Enter> <Esc>");
}
 
void signon(void)
{
    char *msg = malloc(cpl);                // allocate heap memory
    char *divider = malloc(cpl);
 
    memset(divider, HBAR, cpl);             // build horizontal divider
    sprintf(msg, format2, iname);           // build copyright message
 
    VioWrtCharStr(divider, cpl, lpp, 0, 0); // display horizontal divider
    VioWrtCharStr(msg, cpl, lpp+1, 0, 0);   // display copyright message
 
    free(msg);                              // give back heap memory
    free(divider);
}
 
/*
    Hide or display the highlight on the "current line"
    according to the argument "mode":
     -1     = remove the highlight from the current line
     BROWSE = highlight the entire current line
     EDNAME = highlight the name field of the current line
     EDVAL  = highlight the value string field of the current line
*/
void highlite(int mode)
{
    if(totln == 0) return;                  // if empty file, do nothing
 
                                            // remove current highlight
    VioWrtNAttr((char far *) &normattr, cpl, cury, F0COL, 0);
 
    switch(mode)
    {
        case 0:                             // highlight entire line
            VioWrtNAttr((char far *) &revattr, cpl, cury, F0COL, 0);
            break;
 
        case 1:                             // highlight name field
            VioWrtNAttr((char far *) &revattr, 12, cury, F1COL, 0);
            break;
 
        case 2:                             // highlight value field
            VioWrtNAttr((char far *) &revattr, cpl-F2COL, cury, F2COL, 0);
            break;
    }
}
 
/*
    Position the blinking (hardware) cursor.
*/
void setcurpos(int x, int y)
{
    VioSetCurPos(y, x, 0);
}
 
/*
    Hide or reveal cursor and set cursor size
    according to the current editing mode.
*/
void setcurtype(int mode)
{
    curinfo.attr = ((mode == EDVAL) ? VISIBLE : HIDDEN );
    curinfo.start = (insflag ? 0 : cstart);
    VioSetCurType(&curinfo, 0);
}
 
/*
    Clear the screen, restore cursor to its normal appearance,
    and place cursor in the home position (upper left corner).
*/
void cls(void)
{
    VioScrollUp(0, 0, -1, -1, -1, &normcell, 0);
    curinfo.attr = VISIBLE;
    curinfo.start = cstart;
    VioSetCurType(&curinfo, 0);
    setcurpos(0, 0);
}
 
/*
    Clear screen, display error message, exit with non-zero return code.
*/
void errexit(char *p)
{
    cls();
    fprintf(stderr, "\nCONFIG.EXE (%s):  %s\n", iname, p);
    exit(1);
}
 
/*
    Make a mildly obnoxious error beep.
*/
void blip(void)
{
    DosBeep(880,50);
}
 
/*
    Make a moderately obnoxious error beep.
*/
void blap(void)
{
    DosBeep(440,100);
}
 
/*
    Loads and displays help text resource for the specified directive.
*/
void showhelp(int dirix)
{
    unsigned sel;
    unsigned long segsize;
    char far *p;
    char far *q;
 
                                            // load help text resource
    if(DosGetResource(0, IDH_TEXT, directives[dirix].helpid, &sel))
        errexit("Can't load help text resource");
 
    DosSizeSeg(sel, &segsize);              // get size of resource segment
 
    (long) p = (long) sel << 16; q = p;     // make far pointers to resource
 
                                            // find length of help text
    while((*q != '\x1a') && (*q != '\x00') && ((q-p) < (unsigned) (segsize-1)))
        q++;
 
                                            // clear display area
    VioScrollUp(0, 0, lpp-1, cpl-1, lpp, &normcell, 0);
    setcurtype(BROWSE);                     // hide cursor
    setcurpos(0,0);                         // position hidden cursor
    VioWrtTTY(p, q-p, 0);                   // display help text
 
    showstatus("Press any key to proceed!");    // wait for user
    blip();                                     // to press a key
    KbdCharIn(&kbdinfo, WAIT, 0);
 
    showpage();                             // restore display
    showstatus(help[edmode]);               // and help line
    highlite(edmode);                       // position highlight bar
    setcurtype(edmode);                     // maybe reveal cursor
}
 
