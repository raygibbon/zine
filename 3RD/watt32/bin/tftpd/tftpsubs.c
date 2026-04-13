/* 
 * Copyright (c) 1994, 1995, 1997
 *      The Regents of the University of California.  All rights reserved.
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
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
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

/* Simple minded read-ahead/write-behind subroutines for tftp user and server.  Written originally with multiple
 * buffers in mind, but current implementation has two buffer logic wired in.
 * 
 * Todo:  add some sort of final error check so when the write-buffer is finally flushed, the caller can detect if the
 * disk filled up (or had an i/o error) and return a nak to the other side.
 * 
 * Jim Guyton 10/85 */

#include "tftp.h"
                                /* Values for bf.counter  */
#define BF_ALLOC -3		/* alloc'd but not yet filled */
#define BF_FREE  -2		/* free */
/* [-1 .. SEGSIZE] = size of data in the data buffer */

static struct tftphdr *rw_init (struct ra *, int);

struct tftphdr *w_init (struct ra *rp)
{
  return rw_init (rp, 0);	/* write-behind */
}

struct tftphdr *r_init (struct ra *rp)
{
  return rw_init (rp, 1);	/* read-ahead */
}

/*
 * init for either read-ahead or write-behind
 * x = zero for write-behind, one for read-head
 */
static struct tftphdr *rw_init (struct ra *rp, int x)
{
  rp->newline  = 0;               /* init crlf flag */
  rp->prevchar = -1;
  rp->current  = 0;
  rp->bfs[0].counter = BF_ALLOC;  /* pass out the first buffer */
  rp->bfs[1].counter = BF_FREE;
  rp->nextone = x;                /* ahead or behind? */
  return (struct tftphdr*) rp->bfs[0].buf;
}

/*
 * Have emptied current buffer by sending to net and getting ack.
 * Free it and return next buffer filled with data.
 */
int readit (struct ra *rp, FILE *f, struct tftphdr **dpp, int convert)
{
  struct bf *b;

  rp->bfs[rp->current].counter = BF_FREE;  /* free old one */
  rp->current = !rp->current;              /* "incr" current */

  b = &rp->bfs[rp->current];               /* look at new buffer */
  if (b->counter == BF_FREE)               /* if it's empty */
    read_ahead (rp, f, convert);           /* fill it */

  *dpp = (struct tftphdr *) b->buf;        /* set caller's ptr */
  return (b->counter);
}

/* 
 * fill the input buffer, doing ascii conversions if requested
 * conversions are  lf -> cr,lf  and cr -> cr, nul
 */
void read_ahead (struct ra *rp, FILE *f, int convert)
{
  struct bf      *b;
  struct tftphdr *dp;
  int    i, c;
  char  *p;

  b = &rp->bfs[rp->nextone];	/* look at "next" buffer */
  if (b->counter != BF_FREE)
  {
    /* Restore possible errno */
    errno = b->errno;
    return;
  }
  rp->nextone = !rp->nextone;	/* "incr" next buffer ptr */

  dp = (struct tftphdr*) b->buf;

  if (!convert)
  {
    errno = 0;
    b->counter = read (fileno (f), dp->th_data, SEGSIZE);
    b->errno = errno;
    return;
  }

  p = dp->th_data;
  errno = 0;
  for (i = 0; i < SEGSIZE; i++)
  {
    if (rp->newline)
    {
      if (rp->prevchar == '\n')
           c = '\n';              /* lf to cr,lf */
      else c = '\0';              /* cr to cr,nul */
      rp->newline = 0;
    }
    else
    {
      c = getc (f);
      if (c == EOF)
	break;
      if (c == '\n' || c == '\r')
      {
	rp->prevchar = c;
	c = '\r';
	rp->newline = 1;
      }
    }
    *p++ = c;
  }
  if (ferror (f))
       b->counter = -1;
  else b->counter = (int) (p - dp->th_data);
  b->errno = errno;
}

#ifdef ALLOW_WRITE
/*
 * Update count associated with the buffer, get new buffer from the queue.
 * Calls write_behind only if next buffer not available.
 */
int writeit (struct ra *rp, FILE *file,
             struct tftphdr **dpp, int ct, int convert)
{
  rp->bfs[rp->current].counter = ct;  /* set size of data to write */
  rp->current = !rp->current;         /* switch to other buffer */
  if (rp->bfs[rp->current].counter != BF_FREE)	/* if not free */
     write_behind (rp, file, convert);          /* flush it */
  rp->bfs[rp->current].counter = BF_ALLOC;	/* mark as alloc'd */
  *dpp = (struct tftphdr*)rp->bfs[rp->current].buf;
  return (ct);                   /* this is a lie of course */
}

/* 
 * Output a buffer to a file, converting from netascii if requested.
 * CR,NUL -> CR  and CR,LF => LF.
 * Note spec is undefined if we get CR as last byte of file or a
 * CR followed by anything else.  In this case we leave it alone.
 */
int write_behind (struct ra *rp, FILE *file, int convert)
{
  struct bf      *b;
  struct tftphdr *dp;
  char  *buf, *p;
  int    count, ct, c;

  b = &rp->bfs[rp->nextone];
  if (b->counter < -1)		/* anything to flush? */
     return (0);                /* just nop if nothing to do */

  count = b->counter;		/* remember byte count */
  b->counter = BF_FREE;		/* reset flag */
  dp = (struct tftphdr*) b->buf;
  rp->nextone = !rp->nextone;	/* incr for next time */
  buf = dp->th_data;

  if (count <= 0)
     return (-1);               /* nak logic? */

  if (convert == 0)
    return write (fileno(file), buf, count);

  p = buf;
  ct = count;
  while (ct--)
  {				/* loop over the buffer */
    c = *p++;			/* pick up a character */
    if (rp->prevchar == '\r')
    {				/* if prev char was cr */
      if (c == '\n')		/* if have cr,lf then just */
	fseek (file, -1, 1);	/* smash lf on top of the cr */
      else if (c == '\0')	/* if have cr,nul then */
	goto skipit;		/* just skip over the putc */
      /* else just fall through and allow it */
    }
    putc (c, file);
  skipit:
    rp->prevchar = c;
  }
  return (count);
}
#endif
