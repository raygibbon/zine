/*
 * Copyright 1990 by the Massachusetts Institute of Technology.
 * All Rights Reserved.
 *
 * Export of this software from the United States of America may
 *   require a specific license from the United States Government.
 *   It is the responsibility of any person or organization contemplating
 *   export to obtain such a license before exporting.
 *
 * WITHIN THAT CONSTRAINT, permission to use, copy, modify, and
 * distribute this software and its documentation for any purpose and
 * without fee is hereby granted, provided that the above copyright
 * notice appear in all copies and that both that copyright notice and
 * this permission notice appear in supporting documentation, and that
 * the name of M.I.T. not be used in advertising or publicity pertaining
 * to distribution of the software without specific, written prior
 * permission.  M.I.T. makes no representations about the suitability of
 * this software for any purpose.  It is provided "as is" without express
 * or implied warranty.
 *
 */

/*
 * Shared routines for client and server for
 * secure read(), write(), getc(), and putc().
 * Only one security context, thus only work on one fd at a time!
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/ftp.h>

#ifdef USE_SRP

#include "ftp.h"

static unsigned nout;        /* number of chars in ucbuf */

#define ERR          -2      /* Error code */
#define FUDGE_FACTOR 100     /* buffer slack */

static int looping_write (int fd, const char *buf, int len)
{
  int cc, wrlen = len;

  do
  {
    cc = write (fd, buf, wrlen);
    if (cc < 0)
    {
      if (errno == EINTR)
         continue;
      return (cc);
    }
    buf   += cc;
    wrlen -= cc;
  }
  while (wrlen > 0);
  return (len);
}

static int looping_read (int fd, char *buf, int len)
{
  int cc, len2 = 0;

  do
  {
    cc = read (fd, buf, len);
    if (cc < 0)
    {
      if (errno == EINTR)
         continue;
      return (cc);		/* errno is already set */
    }
    if (cc == 0)
       return (len2);

    buf  += cc;
    len2 += cc;
    len  -= cc;
  }
  while (len > 0);
  return (len2);
}

static int secure_putbyte (int fd, BYTE c)
{
  ucbuf[nout++] = c;
  if (nout == maxbuf - FUDGE_FACTOR)
  {
    int ret  = secure_putbuf (fd, ucbuf, nout);
    nout = 0;
    return (ret ? ret : c);
  }
  return (c);
}

/* returns:
 *       0  on success
 *      -1  on error (errno set)
 *      -2  on security error
 */
int secure_flush (int fd)
{
  int ret;

  if (prot_level == PROT_C)
     return (0);

  if (nout && (ret = secure_putbuf(fd, ucbuf, nout)) != 0)
     return (ret);
  return secure_putbuf (fd, "", nout = 0);
}

/* returns:
 *      c>=0  on success
 *      -1    on error
 *      -2    on security error
 */
int secure_putc (char c, FILE *stream)
{
  if (prot_level == PROT_C)
     return putc (c, stream);
  return secure_putbyte (fileno(stream), (BYTE)c);
}

/* returns:
 *      nbyte on success
 *      -1  on error (errno set)
 *      -2  on security error
 */
int secure_write (int fd, BYTE *buf, int nbyte)
{
  int i, c;

  if (prot_level == PROT_C)
     return write (fd, buf, nbyte);

  for (i = 0; nbyte > 0; i++, nbyte--)
  {
     c = secure_putbyte (fd, buf[i]);
     if (c < 0)
        return (c);
  }
  return (i);
}

/* returns:
 *       0  on success
 *      -1  on error (errno set)
 *      -2  on security error
 */
int secure_putbuf (int fd, BYTE *buf, int nbyte)
{
  static char *outbuf  = NULL;  /* output ciphertext */
  static int   bufsize = 0;     /* size of outbuf */

  /* Other auth types go here ...
   */
  if (bufsize < nbyte + FUDGE_FACTOR)
  {
    int size = nbyte + FUDGE_FACTOR;

    if (outbuf)
         outbuf = realloc(outbuf, size);
    else outbuf = malloc (size);

    if (outbuf)
       bufsize = size;
    else
    {
      bufsize = 0;
      WARN1 (_LANG("%s (in malloc of PROT buffer).\r\n"), sys_errlist[errno]);
      return (ERR);
    }
  }

  if (!strcmp(cfg.auth_type,"SRP"))
  {
    long net_len;
    int  length = srp_encode (prot_level == PROT_P,
                              (BYTE*)buf, (BYTE*)outbuf, nbyte);
    if (length < 0)
    {
      WARN (_LANG("srp_encode failed.\r\n"));
      return (ERR);
    }

    net_len = htonl ((u_long)length);
    if (looping_write (fd, (const char*)&net_len, sizeof(net_len)) == -1)
       return (-1);

    if (looping_write (fd, outbuf, length) != length)
       return (-1);
  }
  else
  {
    /* normal unencrypted write */
  }
  return (0);
}

static int secure_getbyte (int fd)
{
  /* number of chars in ucbuf, pointer into ucbuf
   */
  static unsigned nin, bufp;
  int    kerror;
  DWORD  length;

  if (nin == 0)
  {
    kerror = looping_read (fd, (char*)&length, sizeof(length));
    if (kerror != sizeof(length))
    {
      WARN2 (_LANG("Couldn't read PROT buffer length: %d/%s.\r\n"),
             kerror, kerror == -1 ? sys_errlist[errno]
                                  : _LANG("premature EOF"));
      return (ERR);
    }
    length = ntohl (length);
    if (length > maxbuf)
    {
      WARN2 (_LANG("Length (%lu) of PROT buffer > PBSZ=%u.\r\n"),
             length, maxbuf);
      return (ERR);
    }
    kerror = looping_read (fd, ucbuf, length);
    if (kerror != length)
    {
      WARN2 (_LANG("Couldn't read %u byte PROT buffer: %s.\r\n"),
             length, kerror == -1 ? sys_errlist[errno]
                                  : _LANG("premature EOF"));
      return (ERR);
    }
    if (!strcmp(cfg.auth_type, "SRP"))
    {
      nin = bufp = srp_decode (prot_level == PROT_P, (BYTE*)ucbuf,
                               ucbuf, length);
      if (bufp == -1)
      {
        WARN (_LANG("srp_encode failed.\r\n"));
        return (ERR);
      }
    }
  }
  if (nin == 0)
     return (EOF);
  return (ucbuf[bufp - nin--]);
}

/* returns:
 *      c>=0 on success
 *      -1   on EOF
 *      -2   on security error
 */
int secure_getc (FILE *stream)
{
  if (prot_level == PROT_C)
     return getc (stream);
  return secure_getbyte (fileno(stream));
}

/* returns:
 *      n>0 on success (n == # of bytes read)
 *      0   on EOF
 *      -1  on error (errno set), only for PROT_C
 *      -2  on security error
 */
int secure_read (int fd, char *buf, int nbyte)
{
  static int c;
  int    i;

  if (prot_level == PROT_C)
     return read (fd, buf, nbyte);

  if (c == EOF)
  {
    c = 0;
    return (0);
  }

  for (i = 0; nbyte > 0; nbyte--)
  {
    c = secure_getbyte (fd);
    switch (c)
    {
      case ERR:
           return (c);
      case EOF:
           if (!i)
              c = 0;
           return (i);
      default:
           buf[i++] = c;
    }
  }
  return (i);
}

#endif	   /* USE_SRP */
