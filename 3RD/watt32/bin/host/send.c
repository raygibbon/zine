/*
 * Copyright (c) 1985, 1989 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that: (1) source distributions retain this entire copyright
 * notice and comment, and (2) distributions including binaries display
 * the following acknowledgement:  ``This product includes software
 * developed by the University of California, Berkeley and its contributors''
 * in the documentation or other materials provided with the distribution
 * and in all advertising materials mentioning features or use of this
 * software. Neither the name of the University nor the names of its
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#include <stdio.h>
#include <errno.h>
#include <setjmp.h>
#include <signal.h>
#include <time.h>

#include <netinet/in.h>
#include <sys/types.h>    /* not always automatically included */
#include <sys/param.h>
#include <sys/socket.h>
#include <arpa/nameser.h>
#include <arpa/inet.h>
#include <resolv.h>
#include <tcp.h>

#if defined (__GNUC__)
#include <unistd.h>
#endif

#include "port.h"    /* various portability definitions */
#include "conf.h"    /* various configuration definitions */

#ifdef __DJGPP__
#  define setalarm(n) alarm((unsigned int)(n))
#else
#  define setalarm(n) ((void)0)
#endif

char *dbprefix = DBPREFIX;  /* prefix for debug messages to stdout */

static int timeout;              /* connection read timeout */
static struct sockaddr_in from;  /* address of inbound packet */
static struct sockaddr *from_sa = (struct sockaddr *)&from;

int  _res_connect (int, struct sockaddr_in *, int);
int  _res_write   (int, struct sockaddr_in *, char *, char *, int);
int  _res_read    (int, struct sockaddr_in *, char *, char *, int);
int  _recv_sock   (int, char *, int);
void _res_perror  (struct sockaddr_in *, char *, char *);


/*
** _RES_CONNECT -- Connect to a stream (virtual circuit) socket
** ------------------------------------------------------------
**
**  Returns:
**    0 if successfully connected.
**    -1 in case of failure or timeout.
**
**  Note that we use _res.retrans to override the default
**  connect timeout value.
*/

static jmp_buf timer_buf;

static sigtype_t timer (int sig)
{
  longjmp (timer_buf, 1);
}


int _res_connect (int sock, struct sockaddr_in *addr, int addrlen)
{
  if (setjmp(timer_buf))
  {
    errno = ETIMEDOUT;
    setalarm (0);
    return (-1);
  }

#ifdef SIGALRM
  signal (SIGALRM, timer);
  setalarm (_res.retrans);
#endif

  if (connect(sock, (struct sockaddr *)addr, addrlen) < 0)
  {
    if (errno == EINTR)
        errno = ETIMEDOUT;
    setalarm (0);
    return (-1);
  }
  setalarm (0);
  return (0);
}

/*
** _RES_WRITE -- Write the query buffer via a stream socket
** --------------------------------------------------------
**
**  Returns:
**    Length of buffer if successfully transmitted.
**    -1 in case of failure (error message is issued).
**
**  The query is sent in two steps: first a single word with
**  the length of the buffer, followed by the buffer itself.
*/

int _res_write (int sock, struct sockaddr_in *addr,
                char *host, char *buf, int bufsize)
{
  u_short len;

/*
 * Write the length of the query buffer.
 */
  putshort((u_short)bufsize, (u_char *)&len);

  if (write_s(sock, (char *)&len, INT16SZ) != INT16SZ)
  {
    _res_perror (addr, host, "write query length");
    return (-1);
  }

/*
 * Write the query buffer itself.
 */
  if (write_s(sock, buf, bufsize) != bufsize)
  {
    _res_perror (addr, host, "write query");
    return (-1);
  }

  return (bufsize);
}

/*
** _RES_READ -- Read the answer buffer via a stream socket
** -------------------------------------------------------
**
**  Returns:
**    Length of (untruncated) answer if successfully received.
**    -1 in case of failure (error message is issued).
**
**  The answer is read in two steps: first a single word which
**  gives the length of the buffer, followed by the buffer itself.
**  If the answer is too long to fit into the supplied buffer,
**  only the portion that fits will be stored, the residu will be
**  flushed, and the truncation flag will be set.
**
**  Note. The returned length is that of the *un*truncated answer,
**  however, and not the amount of data that is actually available.
**  This may give the caller a hint about new buffer reallocation.
*/

int _res_read (sock, addr, host, buf, bufsize)
     int    sock;
     struct sockaddr_in *addr;  /* the server address to connect to */
     char  *host;               /* name of server to connect to */
     char  *buf;                /* location of buffer to store answer */
     int    bufsize;            /* maximum size of answer buffer */
{
  u_short len;
  char   *buffer;
  int     buflen;
  int     reslen;
  int     n;

  /* set stream timeout for _recv_sock() */
  timeout = READTIMEOUT;

/*
 * Read the length of answer buffer.
 */
  buffer = (char *)&len;
  buflen = INT16SZ;

  while (buflen > 0 && (n = _recv_sock(sock, buffer, buflen)) > 0)
  {
    buffer += n;
    buflen -= n;
  }

  if (buflen != 0)
  {
    _res_perror (addr, host, "read answer length");
    return (-1);
  }

/*
 * Terminate if length is zero.
 */
  len = _getshort ((u_char *)&len);
  if (len == 0)
     return (0);

/*
 * Check for truncation.
 * Do not chop the returned length in case of buffer overflow.
 */
  reslen = 0;
  if ((int)len > bufsize)
     reslen = len - bufsize;

/*
 * Read the answer buffer itself.
 * Truncate the answer is the supplied buffer is not big enough.
 */
  buffer = buf;
  buflen = (reslen > 0) ? bufsize : len;

  while (buflen > 0 && (n = _recv_sock(sock, buffer, buflen)) > 0)
  {
    buffer += n;
    buflen -= n;
  }

  if (buflen != 0)
  {
    _res_perror (addr, host, "read answer");
    return (-1);
  }

/*
 * Discard the residu to keep connection in sync.
 */
  if (reslen > 0)
  {
    HEADER *bp = (HEADER *)buf;
    char resbuf[PACKETSZ];

    buffer = resbuf;
    buflen = (reslen < sizeof(resbuf)) ? reslen : sizeof(resbuf);

    while (reslen > 0 && (n = _recv_sock(sock, buffer, buflen)) > 0)
    {
      reslen -= n;
      buflen = (reslen < sizeof(resbuf)) ? reslen : sizeof(resbuf);
    }

    if (reslen != 0)
    {
      _res_perror (addr, host, "read residu");
      return (-1);
    }

    if (_res.options & RES_DEBUG)
       printf ("%sresponse truncated to %d bytes\n", dbprefix, bufsize);

    /* set truncation flag */
    bp->tc = 1;
  }

  return (len);
}

/*
** _RECV_SOCK -- Read from stream or datagram socket with timeout
** -------------------------------------------------------------
**
**  Returns:
**    Length of buffer if successfully received.
**    -1 in case of failure or timeout.
**  Inputs:
**    The global variable ``timeout'' should have been
**    set with the desired timeout value in seconds.
**  Outputs:
**    Sets ``from'' to the address of the packet sender.
*/

int _recv_sock (int sock, char *buffer, int buflen)
{
  int fromlen, n;

  if (setjmp(timer_buf))
  {
    errno = ETIMEDOUT;
    setalarm (0);
    return (-1);
  }

#ifdef SIGALRM
  signal (SIGALRM, timer);
  setalarm (timeout);
#endif

reread:
  /* fake an error if nothing was actually read */
  fromlen = sizeof (from);
  n = recvfrom (sock, buffer, buflen, 0, from_sa, &fromlen);
  if (n < 0 && errno == EINTR)
     goto reread;
  if (n == 0)
     errno = ECONNRESET;
  setalarm (0);
  return (n);
}


void _res_perror (struct sockaddr_in *addr, /* the server address to connect to */
                  char *host,               /* name of server to connect to */
                  char *message)            /* perror message string */
{
  int save_errno = errno;         /* preserve state */

  /* prepend server address and name */
  if (addr) printf ("%s ", inet_ntoa(addr->sin_addr));
  if (host) printf ("(%s) ", host);

  errno = save_errno;
  perror (message);
  errno = save_errno;
}
