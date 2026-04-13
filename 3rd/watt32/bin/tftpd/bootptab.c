/* 
 * Copyright (c) 1994, 1995, 1997
 *      The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that: (1) source code distributions
 * retain the above copyright notice and this paragraph in its entirety, (2)
 * distributions including binary code include the above copyright notice and
 * this paragraph in its entirety in the documentation or other materials
 * provided with the distribution, and (3) all advertising materials mentioning
 * features or use of this software display the following acknowledgement:
 * ``This product includes software developed by the University of California,
 * Lawrence Berkeley Laboratory and its contributors.'' Neither the name of
 * the University nor the names of its contributors may be used to endorse
 * or promote products derived from this software without specific prior
 * written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#include "tftp.h"

static char *readbootptab (FILE *);
static char *checkbootptab (void);

#define HASHSIZE 4096
#define HASH(a) (&addr_table[(a) & (HASHSIZE - 1)])

static DWORD addr_table[HASHSIZE];

static char *bootptab;

char *setbootptab (char *file)
{
  bootptab = file;
  return (checkbootptab());
}


/* 
 * Determine if this is a "good client" or not.
 * Good clients have a bootptab entry.
 */
static char *checkbootptab (void)
{
  struct stat st;
  static char errmsg[128];
  static time_t mtime = 0, ctime = 0;
  char  *cp;
  FILE  *f;

  if (bootptab == NULL)
     return ("no bootptab set");

  /* Stat the bootptab
   */
  if (stat(bootptab, &st) < 0)
  {
    sprintf (errmsg, "stat on \"%s\": %s", bootptab, strerror(errno));
    return (errmsg);
  }

  /* Nothing to do if nothing has changed
   */
  if (mtime == st.st_mtime && ctime == st.st_ctime)
     return (NULL);

  log (LOG_INFO, "%sreading \"%s\"", mtime != 0L ? "re-" : "", bootptab);

  if ((f = fopen (bootptab, "r")) == NULL)
  {
    sprintf (errmsg, "error opening \"%s\": %s", bootptab, strerror(errno));
    return (errmsg);
  }

  /*
   * Record file modification time.
   */
  if (fstat (fileno (f), &st) < 0)
  {
    sprintf (errmsg, "fstat: %s", strerror (errno));
    fclose (f);
    return (errmsg);
  }
  mtime = st.st_mtime;
  ctime = st.st_ctime;
  cp = readbootptab (f);
  fclose (f);
  return (cp);
}

static char *readbootptab (FILE *f)
{
  char *cp, *cp2;
  char  line[BUFSIZ];
  DWORD a, *p;
  int   i;

  memset (addr_table, 0, sizeof (addr_table));

  while (fgets (line,sizeof(line),f))
  {
    cp = line;

    /* Kill trailing newline */
    cp2 = cp + strlen (cp) - 1;
    if (cp2 >= cp && *cp2 == '\n')
      *cp2++ = '\0';

    /* Scan for ip addresses */
    while (*cp != '\0')
    {
      if (*cp == '#')
	break;
      if (*cp != 'i')
      {
	++cp;
	continue;
      }
      if (*++cp != 'p' || *++cp != '=')
	continue;

      /* found one */
      ++cp;
      cp2 = strchr (cp, ':');
      if (cp2 != NULL)
	*cp2 = '\0';
      a = inet_addr (cp);
      if ((int32_t) a == -1 || a == 0)
	break;

      p = HASH (a);
      i = HASHSIZE;
      while (*p != 0 && i-- > 0)
      {
	++p;
	if (p > &addr_table[HASHSIZE - 1])
	  p = addr_table;
      }
/* XXX should check if the table is full? */
      *p = a;
      break;
    }
  }
  return (NULL);
}

/*
 * Returns true if the specified address is a client in the bootptab file
 */
int goodclient (DWORD a)
{
  int   i;
  DWORD *p;
  char  *cp = checkbootptab();

  if (cp)
     log (LOG_ERR, "goodclient: checkbootptab: %s", cp);

  p = HASH (a);
  p = &addr_table[a & (HASHSIZE - 1)];
  i = HASHSIZE;
  while (*p != 0 && i-- > 0)
  {
    if (*p == a)
      return (1);
    ++p;
    if (p > &addr_table[HASHSIZE - 1])
      p = addr_table;
  }
  return (0);
}
