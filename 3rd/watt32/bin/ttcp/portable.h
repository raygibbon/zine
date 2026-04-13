/*
 * Portability hacks for ttcp
 */

#if defined(__DJGPP__) || defined(__HIGHC__) || defined(unix)
  #include <unistd.h>
  #include <sys/time.h>           /* struct timeval */
  #define HAVE_SIGPIPE
  #undef HZ
  #define HZ 18
#endif

#if defined(__WATCOMC__)
#include <unistd.h>
#endif

#if defined(__DJGPP__) || defined(__HIGHC__) || \
    defined(__BORLANDC__) || defined(__WATCOMC__)
  #define HAVE_GETTIMEOFDAY

  #include <stdlib.h>
  #include <string.h>
  #include <time.h>
  #include <memory.h>
  #include <dos.h>
  #include <tcp.h>

  #define SYSV     /* to pull the correct headers */
  #define read     read_s
  #define write    write_s
  #define close    close_s
  #define select   select_s
  #define times    net_times

  struct rusage {
         struct timeval ru_utime;
         struct timeval ru_stime;
       };
#endif

#if !defined(__DJGPP__) && defined(WATT32)
  #define bcopy(a,b,n)  memcpy((b),(a),(n))
  #define bzero(a,n)    memset((a),0,(n))
#endif

#if defined(unix)
  #if defined(SYSV)
    #include <sys/times.h>
    #include <sys/param.h>
  #else
    #define HAVE_GETTIMEOFDAY
    #define HAVE_GETRUSAGE
    #include <sys/resource.h>
  #endif
#endif

#ifndef ENOBUFS
#define ENOBUFS EINTR
#endif

#ifndef RUSAGE_SELF
#define RUSAGE_SELF 0
#endif
