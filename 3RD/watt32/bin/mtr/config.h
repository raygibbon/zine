/*
 * config.w32 for Watt-32 (djgpp/Watcom/MSVC)
 */

#ifndef __MTR_CONFIG_H
#define __MTR_CONFIG_H

#include <string.h>
#include <conio.h>    /* kbhit() */

#ifdef __WATCOMC__
  #define strcasecmp stricmp
  #include <process.h>
#endif

#ifndef WATT32_NO_GETOPT
#define WATT32_NO_GETOPT  /* don't use getopt in Watt-32 */
#endif

#include <tcp.h>          /* Watt-32 functions */

#undef byte
#undef word
#undef dword

/* Define if you don't have the GTK+ libraries available.  */
#define NO_GTK

#define select select_s
#define close  close_s

#define HAVE_SYS_TYPES_H
#define HAVE_CURSES_H
#define HAVE_ATTRON

#ifdef __DJGPP__
#define HAVE_SYS_TIME_H
#endif

#ifdef _MSC_VER
  #define strcasecmp stricmp
#else
  #define HAVE_UNISTD_H
#endif

/*  Define the version string.  */
#define VERSION "0.4.1"

/*  Find the proper type for 8 bits  */

typedef unsigned char   uint8;
typedef unsigned short  uint16;
typedef unsigned long   uint32;

#endif
