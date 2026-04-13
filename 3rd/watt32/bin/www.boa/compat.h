/*
 *  Boa, an http server
 *  Copyright (C) 1995 Paul Phillips <psp@well.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 1, or (at your option)
 *  any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#ifndef _COMPAT_H
#define _COMPAT_H

#if defined (__GNUC__)
#  include <process.h>
#  include <unistd.h>
#  include <pc.h>         /* kbhit() */
#  define _MAX_PATH   255
#endif

#if defined(__TURBOC__) || defined(__WATCOMC__)
#  include <dir.h>
#endif

#if defined(__HIGHC__)
#  include <direct.h>
#  include <unistd.h>
#  define  min(a,b)  _min(a,b)
#  define  max(a,b)  _max(a,b)
#endif

#if defined(__DMC__)
#  include <direct.h>
#  include <unistd.h>
#  include <process.h>
#endif

#ifdef unix
#  define SLASH  '/'
#else
#  define SLASH  '\\'
#endif

#ifndef S_ISDIR
#define S_ISDIR(m)  ((m) & S_IFDIR)
#endif

#ifndef S_ISREG
#define S_ISREG(m)  ((m) & S_IFREG)
#endif

#endif
