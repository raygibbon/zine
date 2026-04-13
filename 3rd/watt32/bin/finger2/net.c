/*
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Tony Nardo of the Johns Hopkins University/Applied Physics Lab.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <netdb.h>
#include <tcp.h>

extern int lflag;
static tcp_Socket sock;

void netfinger (char *name)
{
  char   c, lastc;
  struct servent *sp;
  char  *host;
  int    status  = 0;
  WORD   port    = 79;
  DWORD  host_ip = 0;

  /* If this is a local request */
  if ((host = strrchr(name, '@')) == NULL)
     return;

  *host++ = 0;

  if ((host_ip = lookup_host(host,NULL)) == 0L)
  {
    printf ("finger: unknown host: %s\n", host);
    return;
  }

#if !defined(__SMALL__) && !defined(DOS386)
  if ((sp = getservbyname("finger","tcp")) != NULL)
     port = intel16 (sp->s_port);
#endif

  if (!tcp_open (&sock,0,host_ip,port,NULL))
  {
    puts ("finger: tcp_open() failed");
    return;
  }

  printf ("waiting...");
  fflush (stdout);
  sock_wait_established (&sock,sock_delay,NULL,&status);
  printf ("\r");

  sock_printf (&sock,"%s%s\r\n", lflag ? "/W " : "", rip(name));

  sock_close (&sock);        /* close sending side.... */

  /*
   * Read from the remote system; once we're connected, we assume some
   * data.  If none arrives, we hang until the user interrupts.
   *
   * If we see a <CR> or a <CR> with the high bit set, treat it as
   * a newline; if followed by a newline character, only output one
   * newline.
   *
   * Otherwise, all high bits are stripped; if it isn't printable and
   * it isn't a space, we can simply set the 7th bit.  Every ASCII
   * character with bit 7 set is printable.
   */
  lastc = 0;
  while (sock_read(&sock,&c,1) > 0)
  {
    c &= 0x7f;
    if (c == '\r')
    {
      if (lastc == '\r')    /* ^M^M - skip dupes */
         continue;
      c = '\n';
      lastc = '\r';
    }
    else
    {
      if (!isprint(c) && !isspace(c))
         c |= 0x40;
      if (lastc != '\r' || c != '\n')
         lastc = c;
      else
      {
        lastc = '\n';
        continue;
      }
    }
    putchar (c);
  }
  if (lastc != '\n')
     putchar ('\n');
  putchar ('\n');
  return;

sock_err:
  switch (status)
  {
    case 1 : puts ("\nHost closed");
             break;
    case -1: printf ("\nError: %s\n",sockerr(&sock));
             break;
  }
  return;

}
