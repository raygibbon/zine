/*
 *  Copyright (C) 1997, 1998 Olivetti & Oracle Research Laboratory
 *
 *  This is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this software; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
 *  USA.
 */

/*
 * sockets.c - functions to deal with sockets.
 */

#include <errno.h>
#include <assert.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <tcp.h>

#include "vncview.h"

static void PrintInHex(char *buf, int len);

Bool errorMessageFromReadExact = True;


/*
 * socket buffer routines (NB: global, don't use for different sockets!)
 * The buffer must be large enough to fully accomodate the largest request
 * For non-multithreaded clients, large buffers can help avoid blocking
 * of other clients which share the same display (if the non-mt client is
 * slow at updating the screen).
 */
#define SOCKET_BUFSIZE (512<<10)
static char sbuf[SOCKET_BUFSIZE];
static int sbuf_lo=0, sbuf_hi=0, sbuf_size=0;
static sockType sbuf_sock;

#ifndef MIN
#define MIN(_a,_b) ((_a)<(_b)?(_a):(_b))
#endif

#ifndef MAX
#define MAX(_a,_b) ((_a)>(_b)?(_a):(_b))
#endif

Bool sockets_init(void)
{
  int s = socket(AF_INET,SOCK_STREAM,0);
  if (s < 0)
  {
    perror("socket");
    return False;
  }
  close (s);

  /* high resolution timers in Watt-32 is incompatible with
   * Allegro's use of the same 8254 timer-chip
   */
  hires_timer (0); 
  return True;
}

int socket_data_available(sockType sock)
{
  int w = 0;

  if (sbuf_size > 0)
     return sbuf_size;

  if (ioctlsocket(sock, FIONREAD, (char*)&w) < 0)
     w = 0;
  return w;
}

void read_buffered_exact(sockType sock, char *buf, int n)
{
  int l;

  assert (sbuf_size == ((sbuf_hi-sbuf_lo+SOCKET_BUFSIZE) % SOCKET_BUFSIZE));

  /* first, read enough bytes from the network (blocking) */
  while(sbuf_size < n)
  {
    int can_read, needed, r;

    needed = n-sbuf_size;
    can_read = MIN(SOCKET_BUFSIZE-sbuf_size,SOCKET_BUFSIZE-sbuf_hi);

    l = MIN(can_read,needed);
    r = read_s(sock,sbuf+sbuf_hi,l);
    if (r < 0)
       exit(3); /* yuck */

    sbuf_size += r;
    sbuf_hi += r;
    if (sbuf_hi == SOCKET_BUFSIZE)
        sbuf_hi = 0;
  }
  l = MIN(n,SOCKET_BUFSIZE-sbuf_lo);
  memcpy(buf,sbuf+sbuf_lo,l);
  n -= l;
  sbuf_lo += l;
  sbuf_size -= l;
  if (n > 0)
  {
    memcpy(buf+l,sbuf,n);
    sbuf_lo += n;
    sbuf_size -= n;
  }
  sbuf_lo %= SOCKET_BUFSIZE;

  if (sbuf_size == 0)
     sbuf_lo = sbuf_hi = 0;
}

/*
 * Read an exact number of bytes, and don't return until you've got them.
 */

Bool
ReadExact(sockType sock, char *buf, int n)
{
  read_buffered_exact(sock,buf,n);
  if (debug && !in_gfx_mode)
     PrintInHex(buf,n);
  return True;
}


/*
 * Write an exact number of bytes, and don't return until you've sent them.
 */

Bool
WriteExact(sockType sock, char *buf, int n)
{
  int i = 0;
  int j;

  while (i < n)
  {
    j = write_s(sock, buf+i,n-i);
    if (j <= 0)
    {
      if (in_gfx_mode)
         return False;
      if (j < 0)
      {
        fprintf(stderr,programName);
        perror(": write");
      }
      else
        fprintf(stderr,"%s: write failed\n",programName);
      return False;
    }
    i += j;
  }
  return True;
}


/*
 * ConnectToTcpAddr connects to the given TCP port.
 */

sockType
ConnectToTcpAddr(unsigned int host, int port)
{
  sockType sock;
  int status=0;
  struct sockaddr_in addr;
  int one = 1;

  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);
  addr.sin_addr.s_addr = host;

  sock = socket(AF_INET, SOCK_STREAM, 0);
  if (sock < 0) {
      fprintf(stderr,programName);
      perror(": ConnectToTcpAddr: socket");
      return -1;
  }

  if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
      fprintf(stderr,programName);
      perror(": ConnectToTcpAddr: connect");
      close(sock);
      return -1;
  }

#if 1
  if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY,
                 (char *)&one, sizeof(one)) < 0) {
      fprintf(stderr,programName);
      perror(": ConnectToTcpAddr: setsockopt");
      close(sock);
      return -1;
  }
#endif

  return sock;
}


/*
 * StringToIPAddr - convert a host string to an IP address.
 */

int
StringToIPAddr(const char *str, unsigned int *addr)
{
  struct hostent *hp = gethostbyname (str);

  if (!hp)
     return (0);
  *addr = *(unsigned int*)hp->h_addr;
  return (*addr != 0);
}


/*
 * Test if the other end of a socket is on the same machine.
 */

Bool
SameMachine(sockType sock)
{
  struct sockaddr_in peeraddr, myaddr;
  int addrlen = sizeof(struct sockaddr_in);

  getpeername(sock, (struct sockaddr *)&peeraddr, &addrlen);
  getsockname(sock, (struct sockaddr *)&myaddr, &addrlen);

  return (peeraddr.sin_addr.s_addr == myaddr.sin_addr.s_addr);
}


/*
 * Print out the contents of a packet for debugging.
 */

static void PrintInHex (char *buf, int len)
{
  int i, j;
  char c, str[17];

  str[16] = 0;

  fprintf(stderr,"ReadExact: ");

  for (i = 0; i < len; i++)
  {
    if ((i % 16 == 0) && (i != 0))
       fprintf(stderr,"           ");

    c = buf[i];
    str[i % 16] = (((c > 31) && (c < 127)) ? c : '.');
    fprintf(stderr,"%02x ",(unsigned char)c);
    if ((i % 4) == 3)
       fprintf(stderr," ");
    if ((i % 16) == 15)
       fprintf(stderr,"%s\n",str);
  }
  if ((i % 16) != 0)
  {
    for (j = i % 16; j < 16; j++)
    {
      fprintf(stderr,"   ");
      if ((j % 4) == 3)
         fprintf(stderr," ");
    }
    str[i % 16] = 0;
    fprintf(stderr,"%s\n",str);
  }
  fflush(stderr);
}
