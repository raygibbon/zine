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

/* $Id: misc.c,v 1.20 1998/11/30 22:03:41 rand Exp $ */

static char *RCSid __attribute__ ((unused)) = "$Id: misc.c,v 1.20 1998/11/30 22:03:41 rand Exp $";

#include "syslogd.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>

static void vErrorMessage(const char *message, const va_list args, int priority)
{
    static char buff[MAXMESSAGE + 1];
    struct message m;
    int length;
    int error;

    error = errno;
    snprintf(buff, sizeof(buff), "%s[%d]: ", progName, pid);
    length = strlen(buff);
    vsnprintf(buff + length, sizeof(buff) - length, message, args);
    length = strlen(buff);
    /* If the message ends in ':' append strerror(errno), easier than implementing %m */
    if(error != 0 && message[strlen(message) - 1] == ':')
	snprintf(buff + length, sizeof(buff) - length, " %s (%d)", strerror(error), error);
    if(upAndRunning) {
	m.message = buff;
	m.length = strlen(buff);
	m.host = hostName;
	m.flags = FLAG_SET_FACPRI|FLAG_ADD_DATE;
	m.facility = LOG_FAC(LOG_SYSLOG);
	m.priority = LOG_PRI(priority);
	m.forwardOK = true;
	DoMessage(&m);
    }

    if(!daemonize || !upAndRunning || DebugTest(0,0)) {
	fprintf(stderr, "%s\n", buff);
    }
}

void ErrorMessage(const char *message, ...)
{
    va_list args;

    va_start(args, message);
    vErrorMessage(message, args, LOG_ERR);
    va_end(args);
}
void WarningMessage(const char *message, ...)
{
    va_list args;

    va_start(args, message);
    vErrorMessage(message, args, LOG_WARNING);
    va_end(args);
}
void InfoMessage(const char *message, ...)
{
    va_list args;

    va_start(args, message);
    vErrorMessage(message, args, LOG_INFO);
    va_end(args);
}

void FatalError(const char *message, ...)
{
    va_list args;

    va_start(args, message);
    vErrorMessage(message, args, LOG_CRIT);
    va_end(args);
    Exit();
}

void LogStatistics(void)
{
    time(&now);
    InfoMessage("syslogd running for %d seconds", now - started);
    InfoMessage("messages/second: %0.4f   bytes/second: %0.4f",
		(float)totalMessagesReceived / (float)(now - started),
		(float)totalBytesReceived / (float)(now - started));
    InfoMessage("total messages: %lu   bytes: %lu", totalMessagesReceived, totalBytesReceived);
    InfoMessage("inet  messages: %lu   bytes: %lu", inetMessagesReceived, inetBytesReceived);
    InfoMessage("local messages: %lu   bytes: %lu", localMessagesReceived, localBytesReceived);
}

void ResetStatistics(void)
{
    time(&started);
    inetBytesReceived = 0;
    inetMessagesReceived = 0;
    localBytesReceived = 0;
    localMessagesReceived = 0;
    totalBytesReceived = 0;
    totalMessagesReceived = 0;
}


void Signal(int sig)
{
    switch(sig) {
	/*
	** Instead of doing the HUP processing here, we just set
	** gotSigHUP and let the main loop handle it since the
	** select() will be interrupted Same with ALRM.
	*/
	case SIGHUP:
	    gotHangup = true;
	    signal(SIGHUP,  Signal);
	    break;
	case SIGALRM:
	    gotAlarm = true;
	    signal(SIGALRM, Signal);
	    alarm(markInterval);
	    break;
	case SIGUSR1:
	    gotUser1 = true;
	    signal(SIGUSR1, Signal);
	    break;
	case SIGUSR2:
	    gotUser2 = true;
	    signal(SIGUSR2, Signal);
	    break;
	default:
	    ErrorMessage("exiting on signal %d", sig);
	    DebugPrintf(10, 0, "syslogd: terminating on signal %d\n", sig);
	    Exit();
    }
}

void Exit(void)
{
    FlushMessages();
    exit(0);
}

void Usage(void)
{
    fprintf(stderr,"usage: syslogd [-f configfile] [-d debuglevel]\n");
    exit(1);
}

int LookupAction(const char *action)
{
    int i = 0;

    while(actionNames[i].name != NULL && strcmp(action, actionNames[i].name) != 0)
	i++;
    return(actionNames[i].value);
}

int LookupFacility(const char *facility)
{
    int i = 0;

    while(facilityNames[i].name != NULL && strcmp(facility, facilityNames[i].name) != 0)
	i++;
    DebugPrintf(10, 500, "Looking up facility %s: i: %d value: %d\n", facility, i, facilityNames[i].value);
    if(facilityNames[i].value >= 0)
	return(facilityNames[i].value >> 3);
    else return(-1);
}

int LookupPriority(const char *priority)
{
    int i = 0;

    while(priorityNames[i].name != NULL && strcmp(priority, priorityNames[i].name) != 0)
	i++;
    return(priorityNames[i].value);
}

struct acls *LookupACL(const char *name)
{
    struct acls *p = acls;

    /* Shouldn't be any list entries with NULL names, but you never know. */
    if(name == NULL) return(NULL);
    else while(p != NULL && !streq(p->name, name)) p = p->next;
    return(p);
}

/*
** Check for a valid time stamp. It doesn't look up the month name or
** check that for valid ranges (such as day 84 of the month), but it
** looks for whitespace in the correct places, digits where they
** belong, and punctuation.
*/
bool ValidTimestamp(const char *ts)
{
    if(strlen(ts) >= 16)
	if(isalpha(ts[0]) && isalpha(ts[1]) && isalpha(ts[2]) &&
	   ts[3] == ' ' &&
	   (ts[4] == ' ' || isdigit(ts[4])) && isdigit(ts[5]) &&
	   ts[6] == ' ' &&
	   isdigit(ts[7]) && isdigit(ts[8]) &&
	   ts[9] == ':' &&
	   isdigit(ts[10]) && isdigit(ts[11]) &&
	   ts[12] == ':' &&
	   isdigit(ts[13]) && isdigit(ts[14]) &&
	   ts[15] == ' ')
	    return(true);
    return(false);
}

struct hashTable *InitCache(void)
{
    struct hashTable *new;

    if((new = malloc(sizeof(struct hashTable))) == NULL)
	FatalError("Could not alloc %d bytes for hash table", sizeof(struct hashTable));
    if((new->cache = calloc(hashTableSize, sizeof(struct hash))) == NULL)
	FatalError("Could not alloc a hash table of %d elements", hashTableSize);
    new->size = 0;
    new->acl = NULL;
    return(new);
}

void CheckCacheSize(struct hashTable *table)
{
    struct hash *p;
    struct hash *next;
    int i;

    if(table == NULL || table->cache == NULL) return;
    DebugPrintf(10, 100, "Checking hash table size: %d  (maximum: %d)\n", table->size, hashTableMaxSize);
    if(table->size <= hashTableMaxSize) return;
    for(i = 0; i < hashTableSize; i++) {
	for(p = table->cache[i]; p!= NULL; p = next) {
	    next = p->next;
	    free(p);
	}
	table->cache[i] = NULL;
    }
    table->size = 0;
}
