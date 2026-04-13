/*
** syslogd -- system logging daemon
** Copyright (c) 1998 Douglas K. Rand   <rand@aero.und.edu>
** All Rights Reserved.
*/

/*
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; see the file COPYING.  If not, write to
** the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
** Boston, MA 02111-1307, USA.
*/

/* $Id: debug.h,v 1.4 1998/11/18 21:22:13 rand Exp $ */

#if defined(DEBUG)
#define DebugTest(facility,level) ((debugFacilities != NULL) && (facility <= debugMax) && (level <= debugFacilities[facility]))
#define DebugLevel(facility) (((debugFacilities != NULL) && (facility <= debugMax)) ? debugFacilities[facility] : -1)
#else
#define DebugTest(facility,level) 0
#define DebugLevel(facility) 0
#define DebugInit(count)
#define DebugParse(string)
/*
** Since DebugPrintf() has a variable number of arguments, you can't
** use a #define to get rid of DebugPrintf. I could do the double
** parens trick DebugPrintf((facility, level, format, args...)) but
** I can never remember to do the double parens.
*/
inline static void DebugPrintf() __attribute__ ((unused));
inline static void DebugPrintf() { }
#endif

extern int debugMax;					/* How many debugging facilities */
extern int *debugFacilities;				/* Array of facilities, each with a level */
