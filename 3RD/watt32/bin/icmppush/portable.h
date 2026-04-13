/*
 * Portability hacks for ICMP-push
 */

#ifndef __PORTABLE_H
#define __PORTABLE_H

#if defined(__DJGPP__)   || defined(__HIGHC__) || \
    defined(__WATCOMC__) || defined(unix)
  #include <unistd.h>
#endif

/* DOS targets using Waterloo tcp/ip
 */
#if defined(__DJGPP__) || defined(__BORLANDC__) || \
    defined(__HIGHC__) || defined(__WATCOMC__)
  #include <dos.h>
  #include <tcp.h>

  #if !defined(__DJGPP__) /* All DOS targets except djgpp lack these */
    #define sigemptyset(sig_set)   ((void)0)
    #define sigaddset(sig_set,sig) ((void)0)
    #define sigprocmask(a,b,c)     ((void)0)
    #define sigsetjmp(a,b)         (0)
  #endif
#endif

#ifdef WATT32
  #include <sys/socket.h>
  #include <time.h>
#else
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <sys/time.h>
#endif

#if defined(SOLARIS)
  #include <sys/sockio.h>

  extern char *sys_errlist[];
  extern int inet_aton (char *, struct in_addr *);
#endif
                         
#if defined(WATT32)
  #define BSD     /* include all macros in <sys/ioctl.h> */
  #define ioctl(s,cmd,arg)  ioctlsocket(s,cmd,(char *)(arg))
  #define close(s)          close_s(s)
#endif


/* Everybody (?) have these
 */
#include <net/if.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <signal.h>
#include <setjmp.h>

#ifndef SIG_BLOCK
#define SIG_BLOCK   1
#endif

#ifndef SIG_SETMASK
#define SIG_SETMASK 2
#endif

#ifndef SIG_UNBLOCK
#define SIG_UNBLOCK 3
#endif
  
#endif   /* __PORTABLE_H */
