/*
 *  pop.c  --  WatTCP POP-mail libraries
 *
 *  Copyright (C) 1992  Walter Andrew Nolan
 *
 *                      wan@eng.ufl.edu
 *
 *                      ECS-MIS
 *                      College of Engineering
 *                      University of Florida
 *
 *                      Version 1.00
 *
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <memory.h>
#include <tcp.h>

#include "md5.h"
#include "charset.h"
#include "popsmtp.h"

#ifdef __TURBOC__
#pragma warn -pro
#endif

#define blankline(line) (strspn(line," \n\t\r") == strlen(line))
#define doubledot(line) (line[0] == '.' && line[1] == '.')
#define msgend(line)    (line[0] == '.' && line[1] == 0)

static char *ParseHeader (const char *buf, FILE *fil);
static void  ClearHeader (void);


/*
 * Global Variables
 */

int    POP_debugOn  = 0;
struct Header msg_header;


/*
 * Local Variables
 */

static int  lastMessageGot;
static int  status, connected;
static char rx_buf [TEMP_BUF_LEN];
static char tx_buf [TEMP_BUF_LEN];
static char timestamp[100] = "";

static int WaitResp (tcp_Socket *sock)
{
  int   len;
  char *p = rx_buf;

  sock_wait_input (sock, sock_delay, NULL, &status);
  len = sock_gets (sock, rx_buf, sizeof(rx_buf));

  while (p < rx_buf+len)
  {
    char *ctrl_z = strchr (p, 26);   /* remove ^Z from buffer */

    if (ctrl_z)
    {
      *ctrl_z++ = ' ';
      p = ctrl_z;
    }
    else
      break;
  }

  if (POP_debugOn)
     (*_printf) (">>> %s\r\n",rx_buf);
  return (1);

sock_err:
  (*_printf) ("Error: %s\r\n",sockerr(sock));
  connected = 0;
  return (0);
}

static int CommandResp (int echo, tcp_Socket *sock, const char *fmt, ...)
{
  if (fmt)
  {
    va_list args;

    va_start (args, fmt);
    vsprintf (tx_buf, fmt, args);
    if (echo && POP_debugOn)
       printf (">>> %s\n", tx_buf);

    sock_puts (sock, tx_buf);
    va_end (args);
  }
  return WaitResp (sock);
}


tcp_Socket *pop_init (const char *host)
{
  tcp_Socket *sock = NULL;
  char       *s1, *s2;
  DWORD       ip = lookup_host (host, NULL);

  if (ip == 0L)
  {
    (*_printf) ("Cannot resolve host %s\r\n", host);
    return (NULL);
  }

  sock = malloc (sizeof(*sock));
  if (!sock)
  {
    (*_printf) ("Malloc failed\r\n");
    return (NULL);
  }

  if (!tcp_open(sock,0,ip,POP_PORT,NULL))
  {
    (*_printf) ("Unable to connect to host %s\r\n", host);
    return (NULL);
  }

  sock_mode (sock, TCP_MODE_ASCII);
  sock_wait_established (sock, sock_delay, NULL, &status);

  if (!WaitResp(sock))     /* get POP server greeting */
     return (NULL);

  connected = 1;

  if (strncmp(rx_buf,"+OK",3))
  {
    (*_printf) ("Host %s unavailable\r\n", host);
    pop_shutdown (sock, 1);
    return (NULL);
  }

  /*
   * Parse the POP-server greeting message. Find the timestamp, e.g.
   * "<process-ID.clock@hostname>" and store it for the APOP login
   */

  s1 = strchr (rx_buf, '<');
  s2 = strchr (rx_buf, '>');
  if (s1 && s2 && s2 > s1 && (s2-s1) < sizeof(timestamp))
  {
    *(s2+1) = 0;
    strcpy (timestamp, s1);
  }

  lastMessageGot = 0;
  return (sock);

sock_err:
  (*_printf) ("Error: %s\r\n", sockerr(sock));
  return (NULL);
}

/*
 * Make a MD5 digests of string
 */
static char *MD5String (const char *string)
{
  int     i;
  MD5_CTX context;
  BYTE    digest[16];
  static  char result[40];

  MD5Init (&context);
  MD5Update (&context, (BYTE*)string, strlen(string));
  MD5Final (digest, &context);
  for (i = 0; i < 16; i++)
      sprintf (result+2*i, "%02x", (unsigned)digest[i]);
  return (result);
}

int pop_login (tcp_Socket *sock, int apop, const char *userid, const char *password)
{
  int rc = 0;

  if (apop)
  {
    char buffer [200];

    if (!timestamp[0])
    {
      (*_printf) ("Didn't find timestamp for APOP login.\r\n");
      return (0);
    }

    sprintf (buffer, "%s%s", timestamp, password);
    rc = CommandResp (1, sock, "APOP %s %s", userid, MD5String(buffer));
    if (!rc)
       return (0);

    if (strncmp(rx_buf,"+OK",3))
    {
      (*_printf) ("Authentication of user %s failed.\r\n", userid);
      return (0);
    }
  }
  else
  {
    rc = CommandResp (0, sock, "USER %s", userid);
    if (!rc)
       return (0);

    if (strncmp(rx_buf,"+OK",3))
    {
      (*_printf) ("Mailbox %s does not exist\r\n", userid);
      return (0);
    }

    rc = CommandResp (0, sock, "PASS %s", password);
    if (!rc)
       return (0);

    if (strncmp(rx_buf,"+OK",3))
    {
      (*_printf) ("Password incorrect\r\n");
      return (0);
    }
  }
  return (1);
}

static int getnumbers (const char *ascii, long *d1, long *d2)
{
  char *p = strchr (ascii, ' '); /* Numbers are separated by white space */

  if (!p)
     return (0);

  /* skip space */
  while (*p == ' ')
        p++;

  *d1 = atol (p);
  p = strchr (p, ' ');
  if (!p)
     return (1);

  /* skip space */
  while (*p == ' ')
        p++;

  *d2 = atol (p);
  return (2);
}

int pop_status (tcp_Socket *sock, long *count, long *totallength)
{
  int rc = CommandResp (0, sock, "STAT");

  if (!rc || strncmp(rx_buf,"+OK",3) ||
      getnumbers(rx_buf,count,totallength) < 2)
  {
    (*_printf) ("Unknown error.\r\n");
    return (0);
  }
  return (1);
}


long pop_length (tcp_Socket *sock, long msg_num, long *size)
{
  long dummy;
  int  rc = CommandResp (0, sock, "LIST %ld", msg_num);

  if (!rc || strncmp(rx_buf,"+OK",3) ||
      getnumbers(rx_buf,&dummy,size) < 2)
  {
    (*_printf) ("No message #%ld\r\n", msg_num);
    return (0);
  }
  return (*size);
}


int pop_list (tcp_Socket *sock, long *msg_nums, long *sizes)
{
  int i  = 0;
  int rc = CommandResp (0, sock, "LIST");

  if (!rc || strncmp(rx_buf,"+OK",3))
  {
    (*_printf) ("Unknown error.\r\n");
    return (0);
  }

  do
  {
    if (!WaitResp(sock))
       return (0);
  }
  while (blankline(rx_buf));

  while (rx_buf[0] != '.')
  {
    if (sscanf(rx_buf, "%ld %ld", &msg_nums[i], &sizes[i]) < 2)
    {
      (*_printf) ("Unknown error.\r\n");
      return (0);
    }
    if (!WaitResp(sock))
       return (0);
    i++;
  }
  return (i);
}

int pop_header (tcp_Socket *sock, long msg_num, FILE *fil)
{
  int rc     = CommandResp (0, sock, "TOP %ld 0", msg_num);
  int tmp    = POP_debugOn;
  int header = 1;

  if (!rc || strncmp(rx_buf,"+OK",3))
  {
    (*_printf) ("Unknown error.\r\n");
    return (0);
  }
  SetCharSet (NULL);
  ClearHeader();

  while (rx_buf[0] != '.')
  {
    char *str = rx_buf;

    if (!WaitResp(sock))
    {
      POP_debugOn = tmp;
      return (0);
    }
    if (blankline(rx_buf))
       POP_debugOn = header = 0;

    if (header)
       ParseHeader (rx_buf,fil);

    if (doubledot(str))
       str++;

    if (msg_header.charset == ISO_8859)
       decode_ISO8859 (str);

    if (fil && fprintf(fil,"%s\r\n",str) == EOF)
       return (0);
  }
  POP_debugOn = tmp;
  return (1);
}


int pop_getf (tcp_Socket *sock, FILE *fil, long msg_num)
{
  int   rc;
  int   header  = 1;
  char *str;

  SetCharSet (NULL);
  ClearHeader();

  rc = CommandResp (0,sock,"RETR %lu",msg_num);
  if (!rc || strncmp(rx_buf,"+OK",3))
  {
    (*_printf) ("No message #%ld\r\n", msg_num);
    return (0);
  }

  do
  {
    str = rx_buf;
    if (!WaitResp(sock))
       return (0);

    if (blankline(str))
       header = 0;

    if (header)
       ParseHeader (str,fil);

    if (doubledot(str))
       str++;

    if (msg_header.charset == ISO_8859)
       decode_ISO8859 (str);

    if (!msgend(rx_buf) && fprintf(fil,"%s\r\n",str) == EOF)
       return (0);
  }
  while (!msgend(rx_buf));

  return (1);
}


int pop_get_file (tcp_Socket *sock, char *fname, long msg_num)
{
  FILE *fil = fopen (fname,"r");
  return (fil ? pop_getf (sock,fil,msg_num) : 0);
}


int pop_delete (tcp_Socket *sock, long msg_num)
{
  int rc = CommandResp (0,sock,"DELE %ld", msg_num);
  if (!rc || strncmp(rx_buf,"+OK",3))
  {
    (*_printf) ("No message #%ld\r\n", msg_num);
    return (0);
  }
  return (1);
}


int pop_shutdown (tcp_Socket *sock, int fast)
{
  if (connected)
     sock_puts (sock,"QUIT");

  sock_close (sock);
  connected = 0;

  if (fast)
  {
    sock_abort(sock);
    return (1);
  }

  if (POP_debugOn)
     (*_printf) ("Closing connection..",rx_buf);
  fflush (stdout);
  sock_wait_closed (sock,sock_delay,NULL,&status);

sock_err:
  return (1);
}

int pop_get_nextf (tcp_Socket *sock, FILE *f)
{
  if (!pop_getf(sock, f, ++lastMessageGot))
     return (0);

  if (!pop_delete(sock, lastMessageGot))
     return (0);

  return (1);
}

static char *_strcpy (char *dest, const char *src, int len)
{
  while (*src == ' ')
        ++src;
  len = min (len-1, strlen(src));
  memcpy (dest, src, len);
  dest [len] = 0;
  return (dest);
}

static char *ParseHeader (const char *buf, FILE *fil)
{
  if (!strncmp(buf,"To: ",4))
     return _strcpy (msg_header.to, buf+4, sizeof(msg_header.to));

  if (!strncmp(buf,"From: ",6))
     return _strcpy (msg_header.from, buf+6, sizeof(msg_header.from));

  if (!strncmp(buf,"Date: ",6))
     return _strcpy (msg_header.date, buf+6, sizeof(msg_header.date));

  if (!strncmp(buf,"Subject: ",9))
     return _strcpy (msg_header.subj, buf+9, sizeof(msg_header.subj));

  if (!strncmp(buf,"Reply-To: ",10))
     return _strcpy (msg_header.reply, buf+10, sizeof(msg_header.reply));

  if (!strnicmp(buf,"Content-type: ",14))
     msg_header.charset = SetCharSet (buf+14);

  return (NULL);
}

static void ClearHeader (void)
{
  memset (&msg_header,0,sizeof(msg_header));
}
