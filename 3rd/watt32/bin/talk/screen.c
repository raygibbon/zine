// =====================================================================
// screen.c - User interface for talk.
//
// (C) 1993, 1994 by Michael Ringe, Institut fuer Theoretische Physik,
// RWTH Aachen, Aachen, Germany (michael@thphys.physik.rwth-aachen.de)
//
// This program is free software; see the file COPYING for details.
// =====================================================================

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <dos.h>
#include <tcp.h>
#include "talk.h"

enum { SINGLE_WIN, SPLIT_WIN };

// Global data

int  quiet    = 0;                  // Quiet mode
int  log      = 0;                  // Log session to log file
int  scrnmode = SINGLE_WIN;         // Screen mode: single/split window
int  autocr   = 0;                  // Insert CR's in single window mode
char *logfilename = LOGFILENAME;    // Log file name
FILE *logf;                         // Log file

int attr[2][PAL_SIZE] = {
     { 0x0F,0x70,0x70,0x7F,0x70 },  // Color set for single window mode
     { 0x0F,0x0F,0x70,0x70,0x70 }   // Color set for split screen mode
   };

// The following is used to handle multiple windows

static int  nlines;                 // Number of screen lines
static int  midline;                // Status line in 'split' mode
static int  botw1;                  // Bottom line of window 1
static int  topw2;                  // Top line of window 2

static int  x[2]  = {1,1};          // Saved cursor position
static int  y[2]  = {1,1};
static int  where = -1;             // Current window
static int  ystatus;                // Status line position
static BYTE st[80][2];              // Status line buffer
static int  newcolors = 1;          // Color change flag


// Edit characters. By convention the first three characters each
// talk transmits after connection are the three edit characters.

char my_ec[3]  = { 0x7f, 0x15, 0x17 };
char his_ec[3] = { 0,    0,    0    };


// ---------------------------------------------------------------------
// init_video() - Initialize the screen i/o routines
// ---------------------------------------------------------------------

void init_video (void)
{
  struct text_info ti;

  gettextinfo (&ti);           // Get screen parameters
  textattr (A_NORM);
  nlines = ti.screenheight;

  midline = (nlines + 1) / 2;
  botw1 = midline - 1;
  topw2 = midline + 1;
}


// ---------------------------------------------------------------------
// lwrite() - Write one character to the log file. mode == 0: local,
// mode == 1: remote
// ---------------------------------------------------------------------

static void lwrite (int mode, char ch)
{
  static char buf[100];
  static int lmode = -1;
  static int lcount = 0;

  if (logf == NULL)
     return;

  if (mode != lmode || lcount >= 77 || ch == '\n')
  {
    if (lcount > 0)      // Flush buffer
    {
      buf [lcount]   = '\n';
      buf [lcount+1] = 0;
      fputs (lmode ? ">" : "<",logf);
      fputs (buf,logf);
      lcount = 0;
    }
    lmode = mode;
  }
  if (ch == 0 || ch == '\n')
     return;
  if (ch == 8 || ch == 127)
  {
    if (lcount > 0)
        lcount--;
  }
  else
    buf [lcount++] = ch;
}


// ---------------------------------------------------------------------
// openlog_talk() - Open and close log file.
// modes: 0=close, 1=open, 2=toggle
// ---------------------------------------------------------------------

void openlog_talk (int mode)
{
  if (mode == 2)
      mode = (logf == NULL);

  if (logf != NULL)
  {
    lwrite (-1,0);   // Flush log file
    fclose (logf);
  }
  if (mode)
  {
    if ((logf = fopen (logfilename,"a+t")) != NULL)
    {
      fprintf (logf,"\n***** %s@%s (%s) *****\n",remoteuser,
               remotehost,dostime());
      lwrite (-1,0);
    }
  }
  else
    logf = NULL;
}


// ---------------------------------------------------------------------
// aputs() - Fill buffer with char/attribute pairs
// ---------------------------------------------------------------------

static void aputs (int pos,int len, WORD attr, char *s)
{
  WORD *d = (WORD *) st + pos;
  while (*s != 0 && len-- > 0)
        *d++ = (attr << 8) | (BYTE) *s++;
}


// ---------------------------------------------------------------------
// wselect() - Select window (0 = local, 1 = remote)
// ---------------------------------------------------------------------

void wselect (int win)
{
  if (quiet || where == win)
     return;

  // Save cursor position
  if (where != -1)
  {
    x [scrnmode ? where : 0] = wherex();
    y [scrnmode ? where : 0] = wherey();
  }

  // Change window
  if (win == -1)
  {
    window (1,1,80,nlines);
    textattr (A_NORM);
  }
  else
  {
    textattr (attr[scrnmode][win]);
    if (scrnmode == SINGLE_WIN)
         window (1,1,80,nlines-1);
    else window (1,win ? topw2 : 1,80,win ? nlines : botw1);

    // Restore cursor position
    gotoxy (x[scrnmode ? win : 0],y[scrnmode ? win : 0]);
  }

  where = win;
}


// ---------------------------------------------------------------------
// cleanup() - Clean up screen
// ---------------------------------------------------------------------

void cleanup (int i)
{
  if (quiet)
     return;
  wselect (-1);
  gotoxy (1,nlines);
       if (i == 1)  cputs ("\r\nYou closed.");
  else if (i == 2)  cputs ("\r\nConnection closed by remote user.");
  clreol();
  if (answermode)
     cputs ("\n\r");
}


// ---------------------------------------------------------------------
// init_screen() - Initialize windows
// ---------------------------------------------------------------------

void init_screen (void)
{
  wselect (-1);
  switch (scrnmode)
  {
    case SINGLE_WIN:
         ystatus = nlines;
         wselect(0);
         clrscr();
         break;
    case SPLIT_WIN:
         ystatus = midline;
         wselect(0);
         clrscr();
         wselect(1);
         clrscr();
         break;
  }
  newcolors = 1;
}


// ---------------------------------------------------------------------
// abswherey() - Returns absolute cursor Y position
// ---------------------------------------------------------------------

static int abswherey (void) 
{
  union REGS r;

  r.x.bx = 0;
  r.x.ax = 0x0300;
  int86 (0x10,&r,&r);
  return (r.x.dx >> 8) + 1;
}

// ---------------------------------------------------------------------
// myputch() - Print a character to the current window. Handles special
//             characters.
// ---------------------------------------------------------------------

void myputch (char ch)
{
  static int last = -1;

  if (logf)
     lwrite (where == 1,ch);

  if (quiet)
     return;

  if (scrnmode == SINGLE_WIN && autocr && where != last &&
      last != -1 && ch != '\n')
     cputs ("\r\n");

  last = where;
  if (ch == '\n')                     // Newline
  {
    cputs ("\r\n");
    last = -1;                        // Suppress autocr
  }
  else if (ch == my_ec[EC_ERASE])     // Erase character
  {
    cputs ("\b \b");
  }
  else if (ch == my_ec[EC_KILL])      // Clear line
  {
    gotoxy (1,wherey());
    clreol();
    last = -1;
  }
  else if (ch == my_ec[EC_WERASE])    // Erase word
  {
    char b[80][2];
    int i;
    int y = abswherey();
    gettext (1,y,80,y,b);
    for (i = wherex()-1; i > 0 && b[i][0] == ' '; --i);
    while (i > 0 && b[i-1][0] != ' ')
         --i;
    gotoxy (i+1,wherey());
    clreol();
  }
  else if (ch == 9)                   // Tab
  {
    int i = 8 - (wherex() - 1) % 8;
    while (i-- > 0)
          putch(' ');
  }
  else
    putch(ch);
}


// ---------------------------------------------------------------------
// update_screen() - Called periodically to update the status line
// ---------------------------------------------------------------------

static void update_screen (void)
{
  static BYTE h = 0, m = 0;
  struct dostime_t t;

  _dos_gettime (&t);
  if (t.minute != m || newcolors)
  {
    char buf[60];
    sprintf (buf,"%s@%s",remoteuser,remotehost);
    buf[40] = 0;
    aputs (0,80,attr[scrnmode][P_STATUS],
           "         ³                                        ³"
           "F1-Help   ESC-Quit³    ³hh:mm");
    if (scrnmode == SPLIT_WIN)
    {
      aputs ( 0,1,attr[scrnmode][P_STATUSL],"\x18");
      aputs (10,1,attr[scrnmode][P_STATUSR],"\x19");
    }
    aputs (   scrnmode, 9-scrnmode,attr[scrnmode][P_STATUSL],localuser);
    aputs (10+scrnmode,40-scrnmode,attr[scrnmode][P_STATUSR],buf);
    h = t.hour;
    m = t.minute;
    if (logf)    aputs (70,1,attr[scrnmode][P_STATUS],"L");
    if (autocr)  aputs (71,1,attr[scrnmode][P_STATUS],"R");
    sprintf (buf,"%2.2d:%2.2d",h,m);
    aputs (75,5,attr[scrnmode][P_STATUS],buf);
    puttext (1,ystatus,80,ystatus,st);
    newcolors = 0;
  }
}


// ---------------------------------------------------------------------
// help() - Display a help screen
// ---------------------------------------------------------------------

static void help (void)
{
  void *screen = malloc (nlines*2*80);

  if (!screen)
  {
    putch (7);
    return;
  }
  wselect (-1);
  gettext (1,1,80,nlines,screen);
  window (1,1,80,nlines);
  textattr (15);
  clrscr();
  cputs (version);

  gotoxy (1,5);
  cputs ("CONTROL KEYS\r\n"
         "\r\n"
         "Close connection .......... ESC\r\n"
         "Display help .............. F1\r\n"
         "Toggle display mode ....... Alt-S\r\n"
         "Toggle autocr ............. Alt-R\r\n"
         "Toggle log file ........... Alt-L\r\n"
         "\r\n"
         "EDIT KEYS\r\n"
         "\r\n"
         "Erase character ........... Del, Backspace, Left\r\n"
         "Erase word ................ Ctrl-Left, Ctrl-W\r\n"
         "Clear line ................ Home, Ctrl-U\r\n" );
  gotoxy (1,25);
  cputs ("Press any key to return");
  getch();
  puttext (1,1,80,nlines,screen);
  wselect (0);
  free (screen);
}


// ---------------------------------------------------------------------
// readkey() - Read one key from keyboard. Returns values > 0xFF for
//             extended keys.
// ---------------------------------------------------------------------

WORD readkey (void)
{
  WORD ch;

  update_screen();
  if (!kbhit())
     return 0;

  if (quiet)  // Ignore all except ESC
  {
    if (getch() == ESC)
         return ESC;
    else return 0;
  }

  // Input character
  if ((ch = getch()) == 0)
     ch = (getch() << 8);    // extended char

  // Handle some special cases
  switch (ch)
  {
    case F1:
         help();
         return 0;
    case ALT_S:
         scrnmode ^= 1;
         init_screen();
         return 0;
    case ALT_L:
         openlog_talk(2);
         newcolors = 1;      // Redraw status line
         return 0;
    case ALT_R:
         autocr = !autocr;
         newcolors = 1;      // Redraw status line
         return 0;
    case 8:
    case DEL:
    case LEFT:
         ch = my_ec[EC_ERASE];
         break;
    case C_LEFT:
         ch = my_ec[EC_WERASE];
         break;
    case HOME:
         ch = my_ec[EC_KILL];
         break;
    case CR:
         ch = 10;
         break;
  }
  return ch;
}

