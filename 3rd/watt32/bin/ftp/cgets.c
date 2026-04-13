/*
 *  cgets.c - line input w/editing
 *  this code is released to the public domain
 *  written by Jon Burchmore
 *  modifications & enhancements by Bob Stout
 *
 *  This is as close to ANSI compliant C that I could come, but it was made
 *  on an IBM compatible computer, so I designed it for that platform.
 *
 *  Rewritten/enhanced by G. Vanem 1997
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <dos.h>
#include <time.h>

#include "ftp.h"

#ifdef __HIGHC__
  #define min(x,y)  _min(x,y)
  #define max(x,y)  _max(x,y)
#else
  #undef  min
  #undef  max
  #define min(x,y)  (((x) < (y)) ? (x) : (y))
  #define max(x,y)  (((x) > (y)) ? (x) : (y))
#endif

#define LoByte(x)   ((unsigned char)((x) & 0xFF))
#define HiByte(x)   ((unsigned char)((unsigned short)(x) >> 8))

#define NUM_HISTORY 16

#if (NUM_HISTORY) & (NUM_HISTORY-1)
#error NUM_HISTORY must be 2**n
#endif

int tab_size      = 8;
int password_mode = 0;

#ifdef TEST
static struct text_info ti;
#endif

static char *history [NUM_HISTORY];  /* command recall ring-buffer   */
static int   hist_idx = 0;           /* index for recalling commands */
static int   save_idx = 0;           /* index for saving commands    */
static int   insmode  = 1;           /* edit insert mode             */

static int (*user_hook)(void) = NULL;

static void  AddToHistory (const char *temp);
static char *PrevHistory  (void);
static char *NextHistory  (void);
static BOOL  LegalPassKey (int c);
static void  BadKey       (void);

/*
 *  Arguments: retBuf   = Buffer to receive string
 *             maxlen   = Size of buffer
 *             hist_add = TRUE if history recall should be used
 *             string   = Default input string
 *             hook     = Pointer to user-hook called while waiting
 *                        for keys
 *
 *  Returns: length of string placed in 'retBuf'.
 */
int cgets_ed (char *retBuf, int maxlen, int hist_add,
              char *string, int (*hooker)(void))
{
  char  temp[80];            /* Working buffer                   */
  int   done;                /* Non-Zero if done                 */
  int   pos;                 /* Our position within the string   */
  int   len;                 /* The current length of the string */
  int   i;                   /* Temporary variable               */
  int   recall = hist_add;   /* history recall if add allowed    */
  int   home_x = wherex();
  int   home_y = wherey();
  char *hist;

  user_hook = hooker;

top:
  memset (temp, 0, sizeof(temp));
  done = 0;
  if (!string)
     string = "";

  len = pos = strlen (string);
  if (len)
     _strncpy (temp, string, min(len,maxlen));

  cprintf ("%s", string);

  while (!done)
  {
    int c = getch_extended();

    if (password_mode && !LegalPassKey(c))
    {
      BadKey();
      continue;
    }

    switch (c)
    {
      case Key_ESC:
           if (len)
           {
             gotoxy (home_x, home_y);
             clreol();
             temp[0] = 0;
           }
           hist_idx = save_idx;
           pos = len = 0;
           break;

      case Key_LTARROW:
#ifndef WIN32
      case Key_PLTARROW:
#endif
           if (pos)
              gotoxy (--pos+home_x, home_y);
           break;

      case Key_RTARROW:
#ifndef WIN32
      case Key_PRTARROW:
#endif
           if (pos != len && pos != maxlen)
              gotoxy (++pos+home_x, home_y);
           break;

      case Key_UPARROW:
#ifndef WIN32
      case Key_PUPARROW:
#endif
           if (recall && ((hist = PrevHistory()) != NULL))
           {
             gotoxy (home_x, home_y);
             clreol();
             string = hist;
             goto top;
           }
           break;

      case Key_DNARROW:
#ifndef WIN32
      case Key_PDNARROW:
#endif
           if (recall && ((hist = NextHistory()) != NULL))
           {
             gotoxy (home_x, home_y);
             clreol();
             string = hist;
             goto top;
           }
           break;

      case Key_HOME:
#ifndef WIN32
      case Key_PHOME:
#endif
           gotoxy (home_x, home_y);
           pos = 0;
           break;

      case Key_END:
#ifndef WIN32
      case Key_PEND:
#endif
           if (pos < len)
              gotoxy (home_x+len, home_y);
           pos = len;
           break;

      case Key_INS:
#ifndef WIN32
      case Key_PINS:
#endif
           insmode ^= 1;
           _setcursortype (insmode ? _NORMALCURSOR : _SOLIDCURSOR);
           break;

      case Key_DEL:
#ifndef WIN32
      case Key_PDEL:
#endif
           if (pos < len)
           {
             for (i = pos; i < len; i++)
                 temp[i] = temp[i+1];
             gotoxy (home_x, home_y);
             clreol();
             cprintf (temp);
             len--;
             gotoxy (home_x+pos, home_y);
           }
           break;

      case Key_BACKSPC:
           if (pos && pos != len)
           {
             for (i = pos-1; i < len; i++)
                 temp[i] = temp[i+1];
             pos--;
             len--;
             gotoxy (home_x+pos, home_y);
             clreol();
             cprintf (temp+pos);
             gotoxy (home_x+pos, home_y);
           }
           else if (pos)
           {
             cputs ("\b \b");
             pos = --len;
           }
           break;

      case Key_ENTER:
      case Key_PADENTER:
      case Key_NL :
           done = 1;
           break;

      default:
           if (pos == maxlen)
              continue;

           if (c & 0xFF00)  /* Illegal extended character */
           {
#ifndef WIN32
             BadKey();
#endif
             continue;
           }
           if (!insmode || pos == len)
           {
             temp[pos++] = (char)c;
             if (pos > len)
                len++;
             putch (password_mode ? '*' : c);
             gotoxy (home_x+pos, home_y);
             continue;
           }
           if (len < maxlen)
           {
             for (i = len++; i >= pos; i--)
                 temp[i+1] = temp[i];
             temp[pos++] = (char)c;
             if (password_mode)
                  putch ('*');
             else cprintf (temp+pos-1);
             gotoxy (home_x+pos, home_y);
             continue;
           }
    }
  }

  temp[len] = '\0';
  _strncpy (retBuf, temp, maxlen);
  if (hist_add && len > 0)
     AddToHistory (retBuf);

  return (len);
}

/*
 * Free list of history strings
 */
static void FreeHistory (void)
{
  unsigned i;

  for (i = 0; i < DIM(history); i++)
      if (history[i])
         free (history[i]);
  memset (&history, 0, sizeof(history));
}

/*
 * Add editbuffer (str) to history ring-buffer
 */
static void AddToHistory (const char *str)
{
  char *h = history [save_idx];
  static BOOL init = FALSE;

  if (!init)
  {
    memset (&history, 0, sizeof(history));
    atexit (FreeHistory);
    init = TRUE;
  }

  if (!h)
  {
    h = strdup (str);
    if (!h)
       return;
  }
  else if (strlen(h) < strlen(str))
  {
    free (h);
    history [save_idx] = NULL;
    h = strdup (str);
    if (!h)
       return;
  }
  else
    strcpy (h, str);

  history [save_idx++] = h;
  save_idx &= (NUM_HISTORY-1);
  hist_idx = save_idx;
}

/*
 * Find a previous commmand in history ring-buffer. Backward search.
 */
static char *PrevHistory (void)
{
  int idx   = (hist_idx-1) & (NUM_HISTORY-1);
  int loops = 0;

  while (!history[idx] && loops < NUM_HISTORY)
  {
    --idx;
    idx &= (NUM_HISTORY-1);
    loops++;
  }
  hist_idx = idx;
  return (history[idx]);
}

/*
 * Find a previous commmand in history ring-buffer. Forward search.
 */
static char *NextHistory (void)
{
  int idx   = (hist_idx+1) & (NUM_HISTORY-1);
  int loops = 0;

  while (!history[idx] && loops < NUM_HISTORY)
  {
    ++idx;
    idx &= (NUM_HISTORY-1);
    loops++;
  }
  hist_idx = idx;
  return (history[idx]);
}

static void BadKey (void)
{
#if 1
   putch ('\a');
#else
#ifdef WIN32
  MessageBeep (MB_ICONEXCLAMATION);
#else
  sound (2000);
  delay (10);
  nosound();
#endif
#endif
}

#if defined(TEST)

static const char *KeyName (int key)
{
  int i;
  static char buf[2];
  static const struct {
         int          value;
         const char  *name;
       } keys[] = {
         { Key_ESC        ,"ESC"            },
         { Key_ENTER      ,"Enter"          },
         { Key_TAB        ,"Tab"            },
         { Key_BACKSPC    ,"BackSpace"      },
         { Key_NL         ,"NewLine"        },
         { Key_LFEED      ,"LineFeed"       },
         { Key_FFEED      ,"FormFeed"       },

         { Key_F1         ,"F1",            },
         { Key_F2         ,"F2",            },
         { Key_F3         ,"F3",            },
         { Key_F4         ,"F4",            },
         { Key_F5         ,"F5",            },
         { Key_F6         ,"F6",            },
         { Key_F7         ,"F7",            },
         { Key_F8         ,"F8",            },
         { Key_F9         ,"F9",            },
         { Key_F10        ,"F10",           },
         { Key_F11        ,"F11",           },
         { Key_F12        ,"F12",           },
         { Key_CF1        ,"Ctrl-F1",       },
         { Key_CF2        ,"Ctrl-F2",       },
         { Key_CF3        ,"Ctrl-F3",       },
         { Key_CF4        ,"Ctrl-F4",       },
         { Key_CF5        ,"Ctrl-F5",       },
         { Key_CF6        ,"Ctrl-F6",       },
         { Key_CF7        ,"Ctrl-F7",       },
         { Key_CF8        ,"Ctrl-F8",       },
         { Key_CF9        ,"Ctrl-F9",       },
         { Key_CF10       ,"Ctrl-F10",      },
         { Key_CF11       ,"Ctrl-F11",      },
         { Key_CF12       ,"Ctrl-F12",      },
         { Key_SF1        ,"Shift-F1",      },
         { Key_SF2        ,"Shift-F2",      },
         { Key_SF3        ,"Shift-F3",      },
         { Key_SF4        ,"Shift-F4",      },
         { Key_SF5        ,"Shift-F5",      },
         { Key_SF6        ,"Shift-F6",      },
         { Key_SF7        ,"Shift-F7",      },
         { Key_SF8        ,"Shift-F8",      },
         { Key_SF9        ,"Shift-F9",      },
         { Key_SF10       ,"Shift-F10",     },
         { Key_SF11       ,"Shift-F11",     },
         { Key_SF12       ,"Shift-F12",     },
         { Key_AF1        ,"Alt-F1",        },
         { Key_AF2        ,"Alt-F2",        },
         { Key_AF3        ,"Alt-F3",        },
         { Key_AF4        ,"Alt-F4",        },
         { Key_AF5        ,"Alt-F5",        },
         { Key_AF6        ,"Alt-F6",        },
         { Key_AF7        ,"Alt-F7",        },
         { Key_AF8        ,"Alt-F8",        },
         { Key_AF9        ,"Alt-F9",        },
         { Key_AF10       ,"Alt-F10",       },
         { Key_AF11       ,"Alt-F11",       },
         { Key_AF12       ,"Alt-F12",       },

         { Key_INS        ,"Ins"            },
         { Key_DEL        ,"Del"            },
         { Key_HOME       ,"Home"           },
         { Key_END        ,"End"            },
         { Key_PGUP       ,"PgUp"           },
         { Key_PGDN       ,"PgDn"           },
         { Key_UPARROW    ,"Up"             },
         { Key_DNARROW    ,"Down"           },
         { Key_LTARROW    ,"Left"           },
         { Key_RTARROW    ,"Right"          },
         { Key_PADMIDDLE  ,"Pad5"           },

         { Key_PADEQ      ,"PadEq"          },
         { Key_PADPLUS    ,"Pad+"           },
         { Key_PADMINUS   ,"Pad-"           },
         { Key_PADASTERISK,"Pad*"           },
         { Key_PADSLASH   ,"Pad/"           },
         { Key_PADENTER   ,"PadEnter"       },

         { Key_CEND       ,"Ctrl-End"       },
         { Key_CDNARROW   ,"Ctrl-Down"      },
         { Key_CPGDN      ,"Ctrl-PgDn"      },
         { Key_CLTARROW   ,"Ctrl-Left"      },
         { Key_CPADMIDDLE ,"Ctrl-Pad5"      },
         { Key_CRTARROW   ,"Ctrl-Right"     },
         { Key_CHOME      ,"Ctrl-Home"      },
         { Key_CUPARROW   ,"Ctrl-Up"        },
         { Key_CPGUP      ,"Ctrl-PgUp"      },
         { Key_CINS       ,"Ctrl-Ins"       },
         { Key_CDEL       ,"Ctrl-Del"       },

         { Key_PINS       ,"Ins-"           },
         { Key_PDEL       ,"Del-"           },
         { Key_PHOME      ,"Home-"          },
         { Key_PEND       ,"End-"           },
         { Key_PPGUP      ,"PgUp-"          },
         { Key_PPGDN      ,"PgDn-"          },
         { Key_PUPARROW   ,"Up-"            },
         { Key_PDNARROW   ,"Down-"          },
         { Key_PLTARROW   ,"Left-"          },
         { Key_PRTARROW   ,"Right-"         },

         { Key_CPEND      ,"Ctrl-End-"      },
         { Key_CPDNARROW  ,"Ctrl-Down-"     },
         { Key_CPPGDN     ,"Ctrl-PgDn-"     },
         { Key_CPLTARROW  ,"Ctrl-Left-"     },
         { Key_CPRTARROW  ,"Ctrl-Right-"    },
         { Key_CPHOME     ,"Ctrl-Home-"     },
         { Key_CPUPARROW  ,"Ctrl-Up-"       },
         { Key_CPPGUP     ,"Ctrl-PgUp-"     },
         { Key_CPINS      ,"Ctrl-Ins-"      },
         { Key_CPDEL      ,"Ctrl-Del-"      },

         { Key_ALTPPLUS   ,"Alt-Pad+"       },
         { Key_ALTPMINUS  ,"Alt-Pad-"       },
         { Key_ALTPASTRSK ,"Alt-Pad*"       },
         { Key_ALTPEQUALS ,"Alt-Pad="       },
         { Key_ALTPSLASH  ,"Alt-Pad/"       },
         { Key_ALTPENTER  ,"Alt-PadEnter"   },

         { Key_ALTBACKSPC ,"Alt-BackSpace"  },
         { Key_CTRLBACKSPC,"Ctrl-BackSpace" },
         { Key_SHIFTTAB   ,"Shift-Tab"      },
         { Key_CTRLTAB    ,"Ctrl-Tab"       },
         { Key_ALTESC     ,"Alt-ESC"        },

         { Key_ALT1       ,"Alt-1"          },
         { Key_ALT2       ,"Alt-2"          },
         { Key_ALT3       ,"Alt-3"          },
         { Key_ALT4       ,"Alt-4"          },
         { Key_ALT5       ,"Alt-5"          },
         { Key_ALT6       ,"Alt-6"          },
         { Key_ALT7       ,"Alt-7"          },
         { Key_ALT8       ,"Alt-8"          },
         { Key_ALT9       ,"Alt-9"          },
         { Key_ALT0       ,"Alt-0"          },
         { Key_ALTMINUS   ,"Alt--"          },
         { Key_ALTEQUALS  ,"Alt-="          },

         { Key_ALTQ       ,"Alt-Q"          },
         { Key_ALTW       ,"Alt-W"          },
         { Key_ALTE       ,"Alt-E"          },
         { Key_ALTR       ,"Alt-R"          },
         { Key_ALTT       ,"Alt-T"          },
         { Key_ALTY       ,"Alt-Y"          },
         { Key_ALTU       ,"Alt-U"          },
         { Key_ALTI       ,"Alt-I"          },
         { Key_ALTO       ,"Alt-O"          },
         { Key_ALTP       ,"Alt-P"          },
         { Key_ALTLBRACE  ,"Alt-["          },
         { Key_ALTRBRACE  ,"Alt-]"          },

         { Key_ALTA       ,"Alt-A"          },
         { Key_ALTS       ,"Alt-S"          },
         { Key_ALTD       ,"Alt-D"          },
         { Key_ALTF       ,"Alt-F"          },
         { Key_ALTG       ,"Alt-G"          },
         { Key_ALTH       ,"Alt-H"          },
         { Key_ALTJ       ,"Alt-J"          },
         { Key_ALTK       ,"Alt-K"          },
         { Key_ALTL       ,"Alt-L"          },
         { Key_ALTCOLON   ,"Alt-:"          },
         { Key_ALTQUOTE   ,"Alt-'"          },
         { Key_ALTENTER   ,"Alt-CR"         },

         { Key_ALTZ       ,"Alt-Z"          },
         { Key_ALTX       ,"Alt-X"          },
         { Key_ALTC       ,"Alt-C"          },
         { Key_ALTV       ,"Alt-V"          },
         { Key_ALTB       ,"Alt-B"          },
         { Key_ALTN       ,"Alt-N"          },
         { Key_ALTM       ,"Alt-M"          },
         { Key_ALTCOMMA   ,"Alt-,"          },
         { Key_ALTPERIOD  ,"Alt-."          },
         { Key_ALTSLASH   ,"Alt-/"          },
         { Key_ALTBSLASH  ,"Alt-\\"         },
         { Key_ALTTILDE   ,"Alt-~"          },
         { 0              ,"??"             }
       };

  for (i = 0; keys[i].value; i++)
      if (key == keys[i].value)
         return (keys[i].name);

  buf[0] = (char)key;
  buf[1] = '\0';
  return (buf);
}
#endif

/*--------------------------------------------------------------*/

#if defined(__DJGPP__)
  static int djgpp_yield (void)
  {
    __dpmi_regs r;
    r.x.ax = 0x1680;
    r.x.ss = r.x.sp = r.x.flags = 0;
    __dpmi_simulate_real_mode_interrupt (0x2F, &r);
    return (r.h.al != 0x80);
  }
#elif defined(__WATCOMC__) && defined(MSDOS) && defined(__386__)
  static int rmode_yield (void)
  {
    union REGS r;
    r.x.eax = 0x1680;
    int386 (0x2F, &r, &r);
    return (r.h.al != 0x80);
  }
#elif defined(MSDOS)
  static int rmode_yield (void)
  {
    union REGS r;
    r.x.ax = 0x1680;
    int86 (0x2F, &r, &r);
    return (r.h.al != 0x80);
  }
#endif

static void system_yield (void)
{
  static int init = 0;
  static int (*yield)(void) = NULL;

#if defined(WIN32)
  Sleep (10);

#elif defined(__HIGHC__)
  if (!init)
     dpmi_yield = 1;       /* calls int2f/1680 in kbhit() */

#elif defined (__DJGPP__)
  if (!init && djgpp_yield())
     yield = djgpp_yield;

#elif defined (MSDOS)
  if (!init && rmode_yield())
     yield = rmode_yield;
#endif

  init = 1;
  if (yield)
    (*yield)();
}

/*--------------------------------------------------------------*/

int getch_extended (void)
{
  int key;

#if defined(WIN32)
again:
  while (!kbhit())
  {
    system_yield();
    if (user_hook && !(*user_hook)())
       return (0);
  }
  key = getch();
#if 1
  if (key == 0xE0)
  {
    key = 0x100 + getch();
    if (key == 0x100 + VK_SHIFT)
       goto again;
  }
#endif

#else
  union REGS regs;

  while (!kbhit())
  {
    system_yield();
    if (user_hook && !(*user_hook)())
       return (0);
  }

  regs.h.ah = 0x10;

#ifdef __WATCOMC__
  int386 (0x16, &regs, &regs);
  key = regs.w.ax;
#else
  int86 (0x16, &regs, &regs);
  key = regs.x.ax;
#endif

  switch (LoByte(key))
  {
    case 0:
         key = HiByte(key) + 0x100;
         break;

    case 0xE0:
         key = HiByte(key) + 0x200;
         break;

    default:
         if (HiByte(key) == 0xE0)
            key = LoByte(key) + 0x200;
         else
         {
           if (ispunct(LoByte(key)) && HiByte(key) > 0x36)
                key = LoByte(key) + 0x200;
           else key = LoByte(key);
         }
  }
#endif  /* WIN32 */

#if defined(TEST) && 1
  {
    int x = wherex();
    int y = wherey();

    gotoxy (1, ti.screenheight);
    cprintf ("key = %04Xh  %-20.20s", key, KeyName(key));
    gotoxy (x, y);
  }
#endif
  return (key);
}

/*--------------------------------------------------------------*/

static BOOL LegalPassKey (int c)
{
  unsigned i;
  static const int illegal_key[] = {
               Key_LTARROW,  Key_PLTARROW,
               Key_RTARROW,  Key_PRTARROW,
               Key_UPARROW,  Key_DNARROW,
               Key_HOME,     Key_PHOME,
               Key_END,      Key_PEND,
               Key_INS,      Key_PINS,
               Key_DEL,      Key_PDEL,
               Key_CEND,     Key_CPEND,
               Key_CRTARROW, Key_CPRTARROW,
               Key_CLTARROW, Key_CPLTARROW,
               Key_TAB
             };
  for (i = 0; i < DIM(illegal_key); i++)
      if (c == illegal_key[i])
         return (FALSE);
  return (TRUE);
}

/*----------------------------------------------------------------*/

#if defined(TEST)

char *_strncpy (char *dest, const char *src, size_t len)
{
  len = min (len, strlen(src));
  memcpy (dest, src, len);
  dest [len] = '\0';
  return (dest);
}

#if 0
void my_yield (void)
{
  static long loop = 0;
  int    x = wherex();
  int    y = wherey();

  delay (100);
  gotoxy (1, 13);
  cprintf ("loop = %ld", loop);
  loop++;
  gotoxy (x, y);
}
#endif

int main (void)
{
  char my_string[60] = { 0 };
  int  i, loop = 0;

#ifdef __HIGHC__
  InstallExcHandler (NULL);
#endif

  gettextinfo (&ti);
  textattr (YELLOW+16*BLACK);
  clrscr();

  while (my_string[0] != 'q')
  {
    cprintf ("%d: ",++loop);
    cgets_ed (my_string, sizeof(my_string)-1,1, NULL, NULL);
#if 0
    cprintf ("\r\ncgets_ed() returned: `%s'\r\n"
             "hist_idx = %d, save_idx = %d\r\n",
             my_string, hist_idx, save_idx);
    for (i = 0; i < NUM_HISTORY; i++)
        cprintf ("history[%2d] = `%s'\r\n", i, history[i]);
#endif
  }
  return (0);
}

#endif /* TEST */
