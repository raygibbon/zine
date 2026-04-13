#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <fcntl.h>
#include <dos.h>
#include <io.h>

#include "ftp.h"

static int   init_done = 0;

static int   statusOfs[LastColumn+1] = { 1 };
static char *statusLine    = NULL;
static char  statusTxt[80] = "             ³"\
                             "                          ³"\
                             "                  ³"\
                             "         ³"\
                             "        ";

/*-----------------------------------------------------------------------*/

void SetColour (enum TextDestination dest)
{
  switch (dest)
  {
    case CtrlText:
         textattr (cfg.colour.ctrl_fg+16*cfg.colour.ctrl_bg);
         break;
    case UserText:
         textattr (cfg.colour.user_fg+16*cfg.colour.user_bg);
         break;
    case DataText:
         textattr (cfg.colour.data_fg+16*cfg.colour.data_bg);
         break;
    case WarnText:
         textattr (cfg.colour.warn_fg+16*cfg.colour.warn_bg);
         break;
  }
}

void xputs (enum TextDestination dest, const char *buf)
{
  char *tab;

  SetColour (dest);
  do
  {
    tab = strchr (buf, '\t');
    if (tab)
    {
      *tab++ = '\0';
      cputs ("        ");
    }
    cputs ((char*)buf);
    buf = tab;
  }
  while (tab);
}

void xputch (enum TextDestination dest, int ch)
{
  SetColour (dest);

  if (ch == '\n')
  {
    putch ('\r');
    putch (ch);
  }
  else if (ch == '\t')
       cputs ("        ");
  else putch (ch);
}

int xprintf (enum TextDestination dest, const char *fmt, ...)
{
  va_list args;
  char    buf [LIN_SIZE];
  int     len;

  va_start (args, fmt);
  len = VSPRINTF (buf, fmt, args);

  xputs (dest, buf);
  va_end (args);
  return (len);
}

STATIC int WattPrintf (const char *fmt, ...)
{
  va_list args;
  char    buf [LIN_SIZE];
  int     len;

  va_start (args, fmt);
  len = VSPRINTF (buf, fmt, args);

  SetColour (WarnText);
  cputs (buf);
  va_end (args);
  return (len);
}

STATIC void PutChar (char c)
{
  char buf[2];

  SetColour (WarnText);
  buf[0] = c;
  buf[1] = 0;
  cputs (buf);
}


/*
 * Ansi test taken from C-Snippets:
 *
 *  Donated to the public domain 96-11-12 by
 *  Tom Torfs (tomtorfs@www.dma.be, 2:292/516@fidonet)
 */
#define ANSI_REPORT()      printf ("\x1B[6n")

STATIC int ANSI_test (void)
{
  DWORD timer;
  int   ch, x_pos, y_pos;

  /* Try a ANSI screen report
   */
  fputs ("\x1B[6n\b\b\b\b    \b\b\b\b", stdout);

  timer = set_timeout (100);   /* max 100 msec wait */

  while ((ch = fgetc(stdin)) == EOF &&
         !chk_timeout(timer))
        ;

  if (ch != '\x1B')
     return (0);
  return (fscanf(stdin,"[%d;%dR",&y_pos,&x_pos) == 2);
}

/*-----------------------------------------------------------------------*/

STATIC void SetStatusLine (void)
{
  if (cfg.status_line >= 1)
     window (1, cfg.status_line+1, cfg.scr_width, cfg.scr_height);

  if (cfg.status_line >= 25)
  {
    if (cfg.status_line != cfg.scr_height)
        cfg.status_line = cfg.scr_height;
    window (1, 1, cfg.scr_width, cfg.scr_height-1);
  }
}

STATIC void SetBackground (void)
{
  if (cfg.colour.user_bg > 7 || cfg.colour.data_bg > 7 ||
      cfg.colour.ctrl_bg > 7 || cfg.colour.warn_bg > 7 ||
      cfg.colour.stat_bg > 7)
       SetIntensity (TRUE);
  else SetIntensity (FALSE);
}

/*-----------------------------------------------------------------------*/

int ScreenInit (void)
{
  struct text_info ti;
  static BOOL first_time = TRUE;

  CONIO_INIT();
  SetBackground();

  gettextinfo (&ti);
  cfg.scr_width  = ti.screenwidth;
  cfg.scr_height = ti.screenheight;

  if (first_time)
  {
    int i,j,k;

    first_time = FALSE;

    CONIO_INIT();

    cfg.colour.dos_fg = ti.attribute & 0xF;
    cfg.colour.dos_bg = ti.attribute >> 4;

    statusLine = malloc (2*cfg.scr_width);
    if (!statusLine)
       cfg.status_line = 0;
    else for (i = j = 0, k = 1; i < cfg.scr_width; j++, i++)
         {
           statusLine[2*i]   = statusTxt[j];
           statusLine[2*i+1] = cfg.colour.stat_fg + 16*cfg.colour.stat_bg;
           if (statusTxt[j] == '³' && k < LastColumn)
               statusOfs[k++] = j + 2;
         }

    statusOfs [LastColumn] = cfg.scr_width+1;
    gotoxy (1, 1);
    SetStatusLine();
    textattr (cfg.colour.user_fg + 16*cfg.colour.user_bg);

    clrscr();
    _outch  = PutChar;
    _printf = WattPrintf;
#ifdef MSDOS
    int29_init();
#endif
  }
#if defined(__DJGPP__)
  else
  {
    int x = wherex();
    int y = wherey();

    SetStatusLine();
    gotoxy (x, y);
  }
#endif

  StatusLine (ProgramVer, FTP_VERSION);
  StatusLine (HostName, " ");
  init_done = 1;
  return (1);
}

int ScreenExit (void)
{
  if (!init_done)
     return (0);

  textcolor (cfg.colour.dos_fg);
  textbackground (cfg.colour.dos_bg);
  window (1, 1, cfg.scr_width, cfg.scr_height);
  gotoxy (1, cfg.scr_height);
  puts (" \b");
  cputs (" \b");
  SetIntensity (FALSE);
  if (statusLine)
     free (statusLine);
  statusLine = NULL;
  return (1);
}

int StatusLine (enum StatusColumn column, const char *fmt, ...)
{
  va_list args;
  char    buf [100];
  int     len, i, j;

  if (!cfg.status_line)
     return (0);

  if (column >= LastColumn)
     return (0);

  va_start (args, fmt);

  if (!fmt)
       len = 0;
  else len = VSPRINTF (buf, fmt, args);

  i = statusOfs [column];
  for (j = 0; i < statusOfs[column+1]-2; j++, i++)
  {
    if (j < len)
         statusLine[2*i] = buf[j];
    else statusLine[2*i] = ' ';
  }
  va_end (args);
  return puttext (1, cfg.status_line, cfg.scr_width,
                  cfg.status_line, statusLine);
}

void StatusRedo (void)
{
  if (cfg.status_line && statusLine)
     puttext (1, cfg.status_line, cfg.scr_width, cfg.status_line, statusLine);
}

int StatusFill (const char *fmt, ...)
{
  va_list args;
  char   *buf, *scr;
  int     len, i;

  if (!cfg.status_line || !fmt)
     return (0);

  va_start (args, fmt);
  buf = alloca (cfg.scr_width);
  scr = alloca (cfg.scr_width << 1);
  len = vsprintf (buf, fmt, args);

  for (i = 0; i < cfg.scr_width; i++)
  {
    if (i < len)
         scr[2*i] = buf[i];
    else scr[2*i] = ' ';
    scr[2*i+1] = cfg.colour.stat_fg + 16*cfg.colour.stat_bg;
  }
  va_end (args);
  return puttext (1, cfg.status_line, cfg.scr_width,
                  cfg.status_line, scr);
}

