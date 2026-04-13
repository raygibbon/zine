#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>      
#include <netdb.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "portable.h"

/* difference between Unix time and net time */
#define BASE1970        2208988800UL

#define TRY_INTVL 2

int max_try = 3;

#ifdef HAVE_ALARM
jmp_buf jmp;

void alarm_handler(int signum)
{
  signal(signum, alarm_handler);
  alarm(TRY_INTVL);
  longjmp(jmp, 1);
}
#endif

time_t
RemoteDate_UDP(char *host)
{
  struct  hostent *him;       /* host table entry */
  struct  servent *timeServ;  /* sevice file entry */
  struct  sockaddr_in sin;    /* socket address */
  int     fd;                 /* network file descriptor */
  time_t  unixTime;           /* time in Unix format */
  u_char  netTime[4];         /* time in network format */
  int     i;                  /* loop variable */
  int     n, try;

  if ((him = gethostbyname(host)) == NULL) {
     fprintf(stderr, "rdate: Unknown host %s\n", host);
     return(-1);
  }

  if ((timeServ = getservbyname("time","udp")) == NULL) {
     fprintf(stderr, "rdate: time/udp: unknown service\n");
     return(-1);
  }

  if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
     perror("rdate");
     return(-1);
  }

  sin.sin_family = him->h_addrtype;
  memcpy(&sin.sin_addr, him->h_addr, him->h_length);
  sin.sin_port = timeServ->s_port;

#ifdef HAVE_ALARM
  /* read in the response, with an alarm */

  signal(SIGALRM, alarm_handler);
  alarm(TRY_INTVL);
#endif

  try = 0;
  while (1)
  {
    if (
#ifdef HAVE_ALARM
        setjmp(jmp) &&
#endif
        ++try == max_try) {
       fprintf(stderr, "rdate: timed out waiting for data from %s\n", host);
       break;
    }
    memset (&netTime, 0, sizeof(netTime)); /* what should we send? */

    i = sendto(fd, &netTime, sizeof(netTime), 0,
               (struct sockaddr*)&sin, sizeof(sin));
    fputc('.',stderr);

    if (i < 0) {
       perror("sendto");
       break;
    }

    i = sizeof(sin);
    n = recvfrom(fd, netTime, sizeof(netTime), 0, (struct sockaddr*)&sin, &i);

    if (n == sizeof(netTime))  {
       unixTime = ((time_t)netTime[0] << 24 |
                   (time_t)netTime[1] << 16 |
                   (time_t)netTime[2] << 8  |
                   (time_t)netTime[3] << 0  ) - BASE1970;
       close(fd);
       return (unixTime);
    }
    if (errno == EINTR)
         fprintf(stderr, "rdate: interrupted\n");
    else fprintf(stderr, "rdate: partial data read from %s\n", host);
  }
  close(fd);
  return (-1);
}

time_t
RemoteDate(char *host)
{
  struct  hostent *him;       /* host table entry */
  struct  servent *timeServ;  /* sevice file entry */
  struct  sockaddr_in sin;    /* socket address */
  int     fd;                 /* network file descriptor */
  time_t  unixTime;           /* time in Unix format */
  u_char  netTime[4];         /* time in network format */
  int     i;                  /* loop variable */

  if ((him = gethostbyname(host)) == NULL) {
     fprintf(stderr, "rdate: Unknown host %s\n", host);
     return(-1);
  }

  if ((timeServ = getservbyname("time","tcp")) == NULL) {
     fprintf(stderr, "rdate: time/tcp: unknown service\n");
     return(-1);
  }

  if ((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("rdate");
    return(-1);
  }

  sin.sin_family = him->h_addrtype;
  memcpy(&sin.sin_addr, him->h_addr, him->h_length);
  sin.sin_port = timeServ->s_port;

  if (connect(fd, (const struct sockaddr*)&sin, sizeof(sin)) < 0) {
     perror("rdate");
     close(fd);
     return(-1);
  }

  /* read in the response */
  for (i = 0; i < 4; ) {
    int l = read(fd, (char*)&netTime[i], 4-i);
    if (l <= 0) {
      perror("rdate");
      close(fd);
      return(-1);
    }
    i += l;
  }

  close(fd);

  unixTime = ((time_t)netTime[0] << 24 |
              (time_t)netTime[1] << 16 |
              (time_t)netTime[2] << 8  |
              (time_t)netTime[3] << 0  ) - BASE1970;

  return (unixTime);
}

int 
main(int argc, char *argv[])
{
  int o, mode = 0;
  int debug = 0;
  time_t t;

  while ((o = getopt(argc, argv, "?dspum:")) != EOF)
    switch (o) {
      case 'd':
           debug++;
           break;
      case 'p':
           mode |= 1;
           break;
      case 's':
           mode |= 2;
           break;
      case 'u':
           mode |= 4;
           break;
      case 'm':
           max_try = atoi(optarg);
           break;

      default:
      usage:
           fprintf(stderr, "Usage: rdate [-d] [-s] [-p] [-u] [-m max-try] <host> ...\n");
      return 1;
    }

  if (optind >= argc)
     goto usage;
  if ((mode & 3) == 0)
     mode |= 1;

#ifdef WATT32
  if (debug)
     dbug_init();
  sock_init();
#endif

  for (o = optind; o < argc; ++o) {
    t = (mode & 4) ? RemoteDate_UDP(argv[o]) : RemoteDate(argv[o]);
    if (t == (time_t)-1)
      continue;
    if (mode & 1)
      printf("[%s]\t%s", argv[o], ctime(&t));
    if (mode & 2)
      if (stime(&t) < 0) {
        perror("rdate");
        return 1;
      }
  }
  return 0;
}


#if defined (__DJGPP__)
int stime (time_t *tp)
{
  struct timeval tv;

  tv.tv_sec  = *tp;
  tv.tv_usec = 0;
  if (settimeofday(&tv) < 0) {
    errno = EINVAL;
    return (-1);
  }
  return (0);
}

#elif defined (WATT32) && !defined(__BORLANDC__)
int stime (time_t *tp)
{
  struct dosdate_t date;
  struct dostime_t time;
  struct tm        tm = *localtime (tp);

  date.year  = tm.tm_year;
  date.month = tm.tm_mon;
  date.day   = tm.tm_mday;

  time.hour    = tm.tm_hour;
  time.minute  = tm.tm_min;
  time.second  = tm.tm_sec;
  time.hsecond = 0;

  if (_dos_setdate(&date) || _dos_settime(&time)) {
    errno = EINVAL;
    return (-1);
  }
  return (0);
}
#endif

