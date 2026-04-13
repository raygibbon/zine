/*
 * Copyright (c) 1983 Eric P. Allman
 * Copyright (c) 1988 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted provided
 * that: (1) source distributions retain this entire copyright notice and
 * comment, and (2) distributions including binaries display the following
 * acknowledgement:  ``This product includes software developed by the
 * University of California, Berkeley and its contributors'' in the
 * documentation or other materials provided with the distribution and in
 * all advertising materials mentioning features or use of this software.
 * Neither the name of the University nor the names of its contributors may
 * be used to endorse or promote products derived from this software without
 * specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "vrfy.h"

/*
** MAXSTR -- Ensure string does not exceed maximum size
** ----------------------------------------------------
**
**  Returns:
**    Pointer to the (possibly truncated) string.
**
**  If necessary, a new string is allocated, and is then
**  truncated, and the original string is left intact.
**  Otherwise the original string is truncated in place.
**
*/

char *maxstr (char *string,   /* the string to check */
              int   n,        /* the maximum allowed size */
              bool  save)     /* allocate new string, if set */
{
  if (strlen(string) > n)
  {
    if (save)
       string = newstr (string);
    string[n] = '\0';
  }
  return (string);
}

/*
** PRINTABLE -- Expand quote bits/control chars in a string
** --------------------------------------------------------
**
**  Returns:
**    Pointer to static buffer containing the expansion.
**
**  The expanded string is silently truncated if it gets too long.
*/

char *printable (char *string)   /* the string to expand */
{
  static char buf[BUFSIZ];       /* expanded string buffer */
  char  *p = buf;
  char  *s = string;
  char   c;

  while ((c = *s++) != '\0')
  {
    if (p >= buf + sizeof(buf) - 4)
       break;

    if (!isascii(c))
    {
      *p++ = '\\';
      c &= 0177;
    }
    if (iscntrl(c) && !isspace(c))
    {
      *p++ = '^';
      c ^= 0100;
    }
    *p++ = c;
  }
  *p = '\0';

  return (buf);
}

/*
** XALLOC -- Allocate or reallocate additional memory
** --------------------------------------------------
**
**  Returns:
**    Pointer to (re)allocated buffer space.
**    Aborts if the requested memory could not be obtained.
*/

void *xalloc (void *buf,    /* current start of buffer space */
              u_int size)   /* number of bytes to allocate */
{
  if (!buf)
       buf = malloc (size);
  else buf = realloc (buf, size);

  if (!buf)
  {
    error ("Out of memory");
    exit (EX_OSERR);
  }
  return (buf);
}

