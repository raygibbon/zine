/*
 * This file is extracted from GNU tar 1.12 file extract.c.
 * Original copyright notice:
 *
 * Extract files from a tar archive.
 * Copyright (C) 1988, 92, 93, 94, 96, 97 Free Software Foundation, Inc.
 * Written by John Gilmore, on 1985-11-19.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
 * Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "ftp.h"

#ifndef S_ISCHR
#define S_ISCHR(m)  ((m) & S_IFCHR)
#endif


/*
 * Attempt to repair file names that are illegal on MS-DOS and
 * MS-Windows by changing illegal characters.
 */

char *msdosify (char *file_name)
{
  static char  dos_name [_MAX_PATH];
  static char  illegal_chars_dos[] = ".+, ;=[]|<>\\\":?*";
  static char *illegal_chars_w95   = illegal_chars_dos + 8;
  char  *s = file_name;
  char  *d = dos_name;
  char  *illegal_aliens = illegal_chars_dos;
  size_t len = sizeof (illegal_chars_dos) - 1;
  int    lfn = 0;
  int    save_errno = errno;    /* in case `_use_lfn' below sets errno */
  int    idx, dot_idx;

  /* Support for Windows 9X VFAT systems, when available. */
#ifdef __DJGPP__
  if (_use_lfn(file_name))
     lfn = 1;
#endif
  if (lfn)
  {
    illegal_aliens = illegal_chars_w95;
    len -= (illegal_chars_w95 - illegal_chars_dos);
  }

  /* Get past the drive letter, if any. */
  if (s[0] >= 'A' && s[0] <= 'z' && s[1] == ':')
  {
    *d++ = *s++;
    *d++ = *s++;
  }

  for (idx = 0, dot_idx = -1; *s; s++, d++)
  {
    if (memchr (illegal_aliens, *s, len))
    {
      /* Dots are special: DOS doesn't allow them as the leading
       * character, and a file name cannot have more than a single dot.
       * We leave the first non-leading dot alone, unless it comes too
       * close to the beginning of the name: we want sh.lex.c to become
       * sh_lex.c, not sh.lex-c.
       */
      if (*s == '.')
      {
        if (idx == 0 && (s[1] == '/' || s[1] == '.') && s[2] == '/')
        {
          /* Copy "./" and "../" verbatim.  */
          *d++ = *s++;
          if (*s == '.')
            *d++ = *s++;
          *d = *s;
        }
        else if (idx == 0)
                *d = '_';
        else if (dot_idx >= 0)
        {
          if (dot_idx < 5) /* 5 is a heuristic ad-hoc'ery */
          {
            d[dot_idx - idx] = '_'; /* replace previous dot */
            *d = '.';
          }
          else
            *d = '-';
        }
        else
          *d = '.';

        if (*s == '.')
           dot_idx = idx;
      }
      else if (*s == '+' && s[1] == '+')
      {
        if (idx - 2 == dot_idx) /* .c++, .h++ etc. */
        {
          *d++ = 'x';
          *d   = 'x';
        }
        else
        {
          /* libg++ etc.  */
          memcpy (d, "plus", 4);
          d += 3;
        }
        s++;
        idx++;
      }
      else
        *d = '_';
    }
    else
      *d = *s;
    if (*s == '/')
    {
      idx = 0;
      dot_idx = -1;
    }
    else
      idx++;
  }

  *d = '\0';
  errno = save_errno;       /* FIXME: errno should be read-only */
  return (dos_name);
}

static char *BaseName (const char *path)
{
  char *s1 = strrchr (path,'\\');
  char *s2 = strrchr (path,'/');

  if (!s1 && !s2)
     return (char*) path;

  if (s1 && *(s1+1))
     return (s1+1);

  if (s2 && *(s2+1))
     return (s2+1);

  return (NULL);
}

char *rename_if_dos_device_name (char *file_name)
{
  /* We could have a file in an archive whose name is a device
   * on MS-DOS.  Trying to extract such a file would fail at
   * best and wedge us at worst.  We need to rename such files.
   *
   * FIXME: I don't know how this can be tested in non-DJGPP
   * environment.  I will assume `stat' works for them also.
   */
  char  *base;
  struct stat st_buf;
  static char fname [_MAX_PATH];
  int    i = 0;

  strcpy (fname, file_name);
  base = BaseName (fname);
  while (stat(base, &st_buf) == 0 && S_ISCHR(st_buf.st_mode))
  {
    size_t blen = strlen (base);

    /* I don't believe any DOS character device names begin with a
     * `_'.  But in case they invent such a device, let us try twice.
     */
    if (++i > 2)
    {
      WARN1 (_LANG("Could not create file `%s'\r\n"), file_name);
      return (file_name);
    }
    /* Prepend a '_'.  */
    memmove (base + 1, base, blen + 1);
    base[0] = '_';
  }
  return (i ? fname : file_name);
}

