/*
 * Portability hacks for rdate
 */

#if defined(__DJGPP__) || defined(__HIGHC__) || defined(__WATCOMC__) || defined(unix)
#include <unistd.h>
#endif

#if defined(__DJGPP__) || defined(unix)
  #define HAVE_ALARM

  #include <signal.h>
  #include <setjmp.h>
#endif

#if defined(__DJGPP__) || !defined(unix)
  #include <dos.h>
  #include <tcp.h>

  #define read    read_s
  #define close   close_s

  int stime (time_t *tp);
#endif


