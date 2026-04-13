/*
 * Copyright (c) 1993 Regents of the University of California.
 * All rights reserved.
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
 *
 *	@(#)tftpsubs.h	5.1 (Berkeley) 5/16/93
 */

#ifndef __TFTP_H
#define __TFTP_H

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <memory.h>
#include <string.h>
#include <signal.h>
#include <process.h>
#include <limits.h>

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/syslog.h>
#include <netinet/in.h>
#include <arpa/tftp.h>
#include <arpa/inet.h>
#include <netdb.h>

#if defined(__MSDOS__)
#include <tcp.h>
#endif

#if defined(unix) || defined(__HIGHC__) || defined(__WATCOMC__)
#include <unistd.h>
#endif

#define PKTSIZE (SEGSIZE + 4)

struct bf {
       int  counter;           /* size of data in buffer, or flag */
       int  errno;             /* errno after read */
       char buf[PKTSIZE];      /* room for data packet */
     };

/* read ahead struct
 */
struct ra {
       int nextone;            /* index of next buffer to use */
       int current;            /* index of buffer in use */
       int newline;            /* fillbuf: in middle of newline expansion */
       int prevchar;           /* putbuf: previous char (cr check) */
       struct bf bfs[2];
     };

extern  char *errstr;
extern  int   debug;


/*
 * Prototypes for read-ahead/write-behind subroutines for tftp user and
 * server.
 */
struct tftphdr *r_init (struct ra *);
struct tftphdr *w_init (struct ra *);

int  write_behind (struct ra *, FILE *, int);
void read_ahead   (struct ra *, FILE *, int);
int  writeit      (struct ra *, FILE *, struct tftphdr **, int, int);
int  readit       (struct ra *, FILE *, struct tftphdr **, int);

char *setbootptab (char *);
int   goodclient  (DWORD);
void  log         (int, char *, ...);

#endif
