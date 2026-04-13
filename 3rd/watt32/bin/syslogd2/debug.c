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

/* $Id: debug.c,v 1.7 1998/11/16 00:07:01 rand Exp $ */

static char *RCSid __attribute__ ((unused)) = "$Id: debug.c,v 1.7 1998/11/16 00:07:01 rand Exp $";

#include "config.h"
#include "debug.h"

#include <stdio.h>
#if defined(HAVE_SYS_SIGEVENT_H)
#include <sys/sigevent.h>
#endif
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <limits.h>

int  debugMax = 0;				/* the number of facilities */
int *debugFacilities = NULL;			/* array of facilities */

#if defined(DEBUG)

void DebugInit(int count)
{
	int  i;
	
	debugMax = count;
	debugFacilities = (int *) calloc(count + 1,sizeof(int));
	if(debugFacilities != NULL)
		for(i = 0; i <= count; i++)
			debugFacilities[i] = -1;
}

/*
** DebugParse sets the level of different debugging facilities. The
** syntax that it recognizes is: "facility[-facility][.level]"
** Multiple settings can be seperated (with out white space!) by
** commas. For example the following are all valid: "4", "2.4",
** "2-5.30", "3-9", "1-10.10,9.100,4". If a level is not specified, it 
** is set to the highest possible level. (a.k.a. INT_MAX).
** Also, if facility 0 is set then all unset facilities are set to
** that level after parsing the rest of the
** string. (e.g. "0.10,4-5.100,8.1000" sets all facilities to level 10 
** with the exceptions of 4 and 5 are set to 100 and 8 is set to 1000.
*/
void DebugParse(char *s)
{
	int first;
	int last;
	int level;
	int i;
	
	if(debugFacilities == NULL) return;
	while(*s != '\0') {
		while(!isdigit(*s)) s++;
		first = atoi(s);
		last = first;
		while(isdigit(*s)) s++;
		if (*s == '-') {
			s++;
			last = atoi(s);
			while(isdigit(*s)) s++;
		}
		level = INT_MAX;
		if (*s == '.') {
			s++;
			level = atoi(s);
			while(isdigit(*s)) s++;
		}
		if(first <= debugMax) {
			if(last >= debugMax) last = debugMax;
			for(i = first; i <= last; i++)
				debugFacilities[i] = level;
		}
	}
	/*
	** Set all the unset debugging facilities to what facility 0
	** was set. Used as a shorthand to set all facilities to the
	** same level.
	*/
	for(i = 1; i <= debugMax; i++)
		if(debugFacilities[i] == -1) debugFacilities[i] = debugFacilities[0];
	/* Always set, so that even if no facilities were set, we know we were called */
	debugFacilities[0] = 0;
}

void DebugPrintf(int facility, int level, char *fmt,...)
{
	va_list		  args;

	va_start(args, fmt);
	if(DebugTest(facility, level))
	    vfprintf(stderr, fmt,args);
	va_end(args);
}
#endif
