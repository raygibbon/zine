// =====================================================================
// talk.h - Header file for talk.
//
// (C) 1993, 1994 by Michael Ringe, Institut fuer Theoretische Physik,
// RWTH Aachen, Aachen, Germany (michael@thphys.physik.rwth-aachen.de)
//
// This program is free software; see the file COPYING for details.
// =====================================================================

#ifndef __TALK_H
#define __TALK_H

#if !defined(VERSION)
#define VERSION "[1.4]"
#endif

#include <conio.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <tcp.h>

#ifdef __HIGHC__
#include <mw/conio.h>
#endif


// ---------------------------------------------------------------------
// Debugging
// ---------------------------------------------------------------------

#ifdef DEBUG
  #define STATIC
  #define DB_printf(s)   cprintf s
  #define DB_ipsplit(ip) (int)((ip) >>24) & 0xFF,(int)((ip)>>16) & 0xFF,\
                         (int)((ip) >> 8) & 0xFF,(int)(ip) & 0xFF
#else
  #define STATIC         static
  #define DB_printf(s)
  #define DB_ipsplit(ip) 0
#endif

#ifdef NONET
  #define tcp_tick(s)            1
  #define sock_putc(s,ch)        ((void)0)
  #define sock_fastread(s,b,sz)  0
  #define sock_init()            tcp_config(NULL)
  #define resolve(h)             0x13141516
#endif

// ---------------------------------------------------------------------
// Ports used by talk
// ---------------------------------------------------------------------

#define MY_DATA_PORT  1050       // Our local data port
#define MY_CTL_PORT   1051       // Our local control port
#define TALK_PORT      518       // Talk daemon port
#define O_TALK_PORT    517       // Old style talk


// ---------------------------------------------------------------------
// Various default values
// ---------------------------------------------------------------------

#define LOGFILENAME        "talk.log"  // Log file name
#define TIMEOUT            900         // Time out
#define ANSWERMODE_TIMEOUT 120         // Time out in answer mode
#define NUM_MESSAGE        11

// ---------------------------------------------------------------------
// Global data
// ---------------------------------------------------------------------

extern char       localuser[];    // Local user name
extern char       remoteuser[];   // Remote user name
extern char       remotetty[];    // Remote terminal
extern char      *remotehost;     // Remote machine name
extern char      *userarg;        // Argument on command line
extern DWORD      remoteip;       // Remote IP address
extern char       version[];      // Version string
extern tcp_Socket ds;             // Socket for talking
extern int        answermode;     // Operate as answering machine

// ---------------------------------------------------------------------
// Utility functions (util.c):
// ---------------------------------------------------------------------

extern char *dostime    (void);
extern void setsockaddr (struct sockaddr_in *s, WORD port, DWORD ip);


// ---------------------------------------------------------------------
// Screen and keyboard i/o (screen.c)
// ---------------------------------------------------------------------

// Key codes returned by readkey()

#define CR      0x000D
#define ESC     0x001B
#define ALT_R   0x1300
#define ALT_S   0x1F00
#define ALT_L   0x2600
#define ALT_C   0x2E00
#define F1      0x3B00
#define HOME    0x4700
#define LEFT    0x4B00
#define DEL     0x5300
#define C_LEFT  0x7300

// Global data

extern int  quiet;              // Quiet mode
extern int  log;                // Log session to log file
extern int  scrnmode;           // Screen mode: 0=single window, 1=split
extern int  autocr;             // Automatically insert CR's
extern int  quiet;              // Quiet mode
extern char *logfilename;	// Log file name

// Screen colors

#define PAL_SIZE  5             // Palette size
#define P_LOCAL   0             // Palette indexes
#define P_REMOTE  1
#define P_STATUS  2
#define P_STATUSL 3
#define P_STATUSR 4
#define A_NORM    7

extern int attr[2][PAL_SIZE];

// Edit characters.

#define EC_SIZE   3
#define EC_ERASE  0     // Erase char
#define EC_KILL   1     // Kill line
#define EC_WERASE 2	// Erase word

extern char my_ec[3], his_ec[3];

// Functions

void init_video  (void);
void wselect     (int win);
void cleanup     (int i);
void init_screen (void);
void openlog_talk(int mode);
WORD readkey     (void);
void myputch     (char ch);


// ---------------------------------------------------------------------
// Talk daemon (message.c)
// ---------------------------------------------------------------------

int wait_invite (void);
int invite      (void);
int o_invite    (void);

#endif
