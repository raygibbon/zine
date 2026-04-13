// =====================================================================
// talk.c - A talk client for MS-DOS. Uses the WATTCP library.
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


// ---------------------------------------------------------------------
// Global data
// ---------------------------------------------------------------------

char version[] =
    "Talk version " VERSION ", written by Michael Ringe\r\n";

char usage[] =
     "NAME\n"
     "    Talk version " VERSION "\n"
     "\n"
     "USAGE\n"
     "    talk [-lod] user@{host|alias} [tty] תתתתתתתת Talk to another user\n"
     "    talk [-aqd] תתתתתתתתתתתתתתתתתתתתתתתתתתתתתתתת Wait for talk requests\n"
     "\n"
     "OPTIONS\n"
     "    -l   Log the talk session to " LOGFILENAME "\n"
     "    -o   Use old-style talk protocol\n"
     "    -a   Enter answer mode\n"
     "    -q   Be quiet in answer mode\n"
     "    -d   Set debug mode\n"
     "\n"
     "AUTHOR\n"
     "    (C) Copyright 1994 by Michael Ringe\n"
     "    This program is free software; see the file COPYING for details.\n";

char aamsg1[] = "Hello %s, %60s\n";

char *aamsg2[NUM_MESSAGE] =  {
                 "This talk is running in answer mode.",
                 "If you don't mind talking to a machine you may leave",
                 "a message. Finish by closing the talk connection.",
                 "Your message will be logged and read later.",
                 "-- %s",
                 NULL
               };

char *userarg        = NULL;      // Command line arg, NULL = wait
char  localuser[20]  = "PC-User"; // My user name
char  remoteuser[20] = "";        // Remote user name
char *remotehost     = NULL;      // Remote host name
int   answermode     = 0;         // Operate as answering machine
int   opt_o          = 0;         // Use old protocol
int   debug          = 0;         // WatTCP debug mode
DWORD remoteip       = 0;         // Remote IP address
char  remotetty[20]  = "";        // Terminal to connect to

void (*prev_hook) (const char*,const char*);   // config-file parser routine
tcp_Socket ds;                                 // Talk socket

// ---------------------------------------------------------------------
// Initialization
// ---------------------------------------------------------------------

STATIC void err_usage (void)
{
  puts (usage);
  exit (1);
}

STATIC void my_usr_init (const char *key, const char *val)
{
  if (!strcmp(key,"TALK.LOCALUSER"))
  {
    strncpy (localuser,val,sizeof(localuser));
  }

  else if (!strcmp(key,"TALK.LOGFILE"))
  {
    logfilename = strdup (val);
  }

  else if (!strcmp(key,"TALK.SCREENMODE"))
  {
    if (strstr(val,"split"))  scrnmode = 1;
    if (strstr(val,"autocr")) autocr = 1;
  }

  else if (!strcmp(key,"TALK.COLORS"))
  {
    int num = sscanf (val,
                      "%02Xh,%02Xh,%02Xh,%02Xh,%02Xh/"
                      "%02Xh,%02Xh,%02Xh,%02Xh,%02Xh",
               &attr[0][0],&attr[0][1],&attr[0][2],&attr[0][3],&attr[0][4],
               &attr[1][0],&attr[1][1],&attr[1][2],&attr[1][3],&attr[1][4]);
    if (num <= 4)
    {
      attr[1][0] = attr[0][0];
      attr[1][1] = attr[0][1];
      attr[1][2] = attr[0][2];
      attr[1][3] = attr[0][3];
      attr[1][4] = attr[0][4];
    }
  }

  else if (!strcmp(key,"TALK.ALIAS"))
  {
    char *c = strchr (val,':');
    if (c)
    {
      *c++ = 0;
      if (userarg && !strcmp(val,userarg))
          userarg = strdup (c);
    }
  }

  else if (!strcmp(key,"TALK.MESSAGE"))
  {
    static int n = 0;

    if (n < NUM_MESSAGE-1)
    {
      aamsg2[n++] = strdup (val);
      aamsg2[n] = NULL;
    }
  }
  else if (prev_hook)
         (*prev_hook) (key, val);
}


STATIC int init (int argc, char **argv)
{
  int ch;

  while ((ch = getopt(argc,argv,"?alqodD")) != EOF)
     switch(ch)
     {
       case 'a': answermode = 1;
                 break;
       case 'l': log = 1;
                 break;
       case 'q': quiet = 1;
                 break;
       case 'o': opt_o = 1;
                 break;
       case 'd': debug = 1;
                 break;
       case 'D': debug = 2;
                 break;
       default : err_usage();
     }

  argc -= optind;
  argv += optind;

  switch (argc)      // Process remaining arguments
  {
    case 2 : strncpy (remotetty,argv[1],sizeof(remotetty));
             /* fall-through */
    case 1 : userarg = argv[0];
             if (answermode)
                err_usage();
    case 0 : break;
    default: err_usage();
  }

  if (!answermode)   // Ignore -q if not in answer mode
     quiet = 0; 

  if (debug)
  {
    tcp_set_debug_state (debug);
    dbug_init();
  }

  // Start up wattcp
  prev_hook = usr_init;
  usr_init  = my_usr_init;
  sock_init();
  init_video();

  if (answermode)   // Enter message
  {
    int n;
    char tmp[100];
    userarg = NULL;
    log = 1;
    cprintf ("Enter message (up to %d lines, end with a single '.'),\r\n"
             "or just press <ENTER> for a default message:\r\n",
             NUM_MESSAGE-1);
    for (n = 0; n < 10; ++n)
    {
      *tmp = 79;
      cgets (tmp);
      cputs ("\r\n");
      if ((tmp[1] == 1 && tmp[2] == '.') || (n == 0 && tmp[1] == 0))
         break;
      aamsg2[n]   = strdup (tmp+2);
      aamsg2[n+1] = NULL;
    }
  }

  // Split argument into user and host name
  if (userarg)
  {
    char *c = strchr (userarg,'@');
    if (!c)
       err_usage();
    *c++ = 0;
    strncpy (remoteuser,userarg,sizeof(remoteuser));
    remotehost = c;

    // Look up remote IP address
    if ((remoteip = resolve(remotehost)) == 0)
    {
      cprintf ("Error: Cannot resolve '%s'\r\n",remotehost);
      return 1;
    }
    DB_printf (("Remote IP: %d.%d.%d.%d\r\n",DB_ipsplit(remoteip)));

  }
  DB_printf (("Local user: %s\r\nRemote user: %s\r\nRemote host: %s\r\n",
             localuser,remoteuser,remotehost));
  return 0;
}

// ---------------------------------------------------------------------
// exchange_edit_chars() - Exchange the three edit characters with the
//                         remote talk.
// ---------------------------------------------------------------------

#ifdef NONET
#define exchange_edit_chars()  ((void)0)
#else
  STATIC void exchange_edit_chars (void)
  {
    sock_write (&ds,my_ec,3);
    sock_read (&ds,his_ec,3);
    DB_printf (("Remote edit characters: ERASE=%x, KILL=%x, WERASE=%x\r\n",
               his_ec[0],his_ec[1],his_ec[2]));
  }
#endif

// ---------------------------------------------------------------------
// talk() - This function does the actual talking
// ---------------------------------------------------------------------

STATIC int talk (void)
{
  static char buf[500];
  WORD   ch;
  int    i, inactive = 0;

  clrscr();
  exchange_edit_chars();
  DB_printf (("Entering talk(), press return:\r\n"));
  gets (buf);

  if (!quiet)
  {
    sound (2000);
    delay (100);
    nosound();
    init_screen();
  }
  cprintf ("Connected to %s@%s at %s.\r\n",remoteuser,remotehost,dostime());

  // If in answer mode, write the message to the remote screen
  // Using sock_printf here would use another 3kB memory.
  if (answermode)
  {
    int n;
    sprintf (buf,aamsg1,remoteuser,dostime());
    sock_puts (&ds,buf);
    for (n = 0; n < 10 && aamsg2[n] != NULL; ++n)
    {
      sprintf (buf,aamsg2[n],localuser);
      sock_puts (&ds,buf);
      sock_putc (&ds,'\n');
    }
  }

  // Main loop, repeat until ESC pressed or socket closed
  while (tcp_tick(&ds))
  {
    // Process keyboard input
    if ((ch = readkey()) != 0)
    {
      if (ch == ESC)     // User break
         return 1;  
      inactive = 0;

      // If it's a normal character, write it to both
      // local screen and remote host.
      if (ch < 0x100)
      {
        wselect (0);
        myputch (ch);

        // Translate edit characters
        for (i = 0; i < EC_SIZE; ++i)
            if (ch == my_ec[i])
            {
              ch = his_ec[i];
              break;
            }
        sock_putc (&ds,ch);
      }
    }

    // Process input from remote host
#ifdef NONET
    i = 0;
    if (ch == ALT_C)
    {
      i = 17;
      strcpy (buf," 123 4567 199 8xxx");
      ch = 0;
    }
#else
    i = sock_fastread (&ds,buf,sizeof(buf));
#endif
    if (i > 0)
    {
      int k;
      wselect (1);
      inactive = 0;
      for (k = 0; k < i; ++k)
          myputch (buf[k]);
    }
    if (!inactive)
        ip_timer_init((udp_Socket *)&ds,answermode ?
                      ANSWERMODE_TIMEOUT : TIMEOUT);
    inactive = 1;
    if (ip_timer_expired(&ds))
    {
      sock_puts (&ds,"time-out");
      return 1;
    }
  }
  return 2;           // Closed by remote party
}


// ---------------------------------------------------------------------
// main()
// ---------------------------------------------------------------------

int main (int argc, char **argv)
{
  int status = 0;

  if (init(argc,argv))
     return (1);

#ifdef NONET
  talk();
  openlog_talk (0);
  cleanup (1);
#else

  if (userarg != NULL)
  {
    if (opt_o)
       status = o_invite();
    else
    {
      status = invite();
      if (status == 1)
          status = o_invite();
    }
    if (status != 0 && status != 2)
       return(1);
  }

  do
  {
    if (status != 2)
        status = wait_invite();
    if (status == 27)
       break;
    if (status == 0 || status == 2)
    {
      if (log)
         openlog_talk(1);
      status = talk();
      openlog_talk (0);
      sock_abort (&ds);
      sock_close (&ds);
      cleanup (status);
      status = 0;
    }
  }
  while (answermode);

#endif
  return status;
}      

