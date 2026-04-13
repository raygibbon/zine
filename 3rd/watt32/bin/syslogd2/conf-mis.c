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

/* $Id: conf-misc.c,v 1.36 1998/11/30 22:04:09 rand Exp $ */

static char *RCSid __attribute__ ((unused)) = "$Id: conf-misc.c,v 1.36 1998/11/30 22:04:09 rand Exp $";

#include "syslogd.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <netdb.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

void SetDirectory(const char *argument)
{
    struct stat status;
    
    directory = strdup(argument);
    DebugPrintf(3, 100, "Changing directory to %s\n", directory);
    if(directory == NULL || *directory == '\0') {
	ErrorMessage("directory option NULL or empty");
	return;
    }
    if(chdir(directory) < 0) {
	ErrorMessage("Could not change directory to %s:", directory);
	return;
    }
    if (stat(directory, &status) < 0) {
	ErrorMessage("could not stat directory %s:", directory);
	return;
    }
    if(status.st_mode & S_IWOTH)
	ErrorMessage("directory %s is world writeable", directory);
}

void SetSyslogPort(char *name)
{
    int port;
    
    if((port = GetPortByName(name)) != 0) {
	syslogPortName = strdup(name);
	syslogPort = port;
    }
    else ErrorMessage("Could not find service named '%s'\n", name);
}

void SetMarkInterval(char *argument)
{
    char *p = argument;
    
    if(p != NULL) {
	markInterval = atoi(p);
	while(*p != '\0' && isdigit(*p)) p++;
	switch(*p) {
	    /* Notice that all cases fall through */
	    case 'w': markInterval *= 7;
	    case 'd': markInterval *= 24;
	    case 'h': markInterval *= 60;
	    case 'm': markInterval *= 60;
	    case 's': /* NULL */;
	    case '\0': break;
	    default: ErrorMessage("Invalid units on mark-interval: '%c'\n", *p);
	}
	DebugPrintf(3, 100, "markInterval set to %d (%s)\n", markInterval, argument);
    }
}

void SetRepeatInterval(char *argument)
{
    char *p = argument;
    
    if(p != NULL) {
	repeatInterval = atoi(p);
	while(*p != '\0' && isdigit(*p)) p++;
	switch(*p) {
	    /* Notice that all cases fall through */
	    case 'w': repeatInterval *= 7;
	    case 'd': repeatInterval *= 24;
	    case 'h': repeatInterval *= 60;
	    case 'm': repeatInterval *= 60;
	    case 's': /* NULL */;
	    case '\0': break;
	    default: ErrorMessage("Invalid units on repeat-interval: '%c'\n", *p);
	}
	DebugPrintf(3, 100, "repeatInterval set to %d (%s)\n", repeatInterval, argument);
    }
}

void SetInetForwardFrom(const char *name)
{
    if(name == NULL) inetForwardFrom->acl = NULL;
    else {
	struct acls *p = LookupACL(name);
	if(p == NULL) ErrorMessage("inet-forward-from: acl '%s' not defined", name);
	else inetForwardFrom->acl = p->list;
    }
}

void SetInetReceiveFrom(const char *name)
{
    if(name == NULL) inetReceiveFrom->acl = NULL;
    else {
	struct acls *p = LookupACL(name);
	if(p == NULL) ErrorMessage("inet-receive-from: acl '%s' not defined", name);
	else inetReceiveFrom->acl = p->list;
    }
}

void SetSimpleHostnames(const char *name)
{
    if(name == NULL) simpleHostnames->acl = NULL;
    else {
	struct acls *p = LookupACL(name);
	if(p == NULL) ErrorMessage("simple-hostnames: acl '%s' not defined", name);
	else simpleHostnames->acl = p->list;
    }
}

void SetStripDomains(const char *name)
{
    if(name == NULL) stripDomains->acl = NULL;
    else {
	struct acls *p = LookupACL(name);
	if(p == NULL) ErrorMessage("strip-domains: acl '%s' not defined", name);
	else stripDomains->acl = p->list;
    }
}

struct aclEntry *CreateHostACL(const char *hostName)
{
    struct aclEntry *acl;

    DebugPrintf(3, 100, "Creating host acl for '%s'\n", hostName);
    if((acl = malloc(sizeof(struct aclEntry))) == NULL)
	FatalError("could not malloc %d bytes for host ACL named %s", sizeof(struct aclEntry), hostName);
    acl->type = host;
    acl->acl.host = strdup(hostName);
    return(acl);
}

struct aclEntry *CreateDomainACL(char *domainName)
{
    struct aclEntry *acl;

    DebugPrintf(3, 100, "Creating domain acl for '%s'\n", domainName);
    if((acl = malloc(sizeof(struct aclEntry))) == NULL)
	FatalError("could not malloc %d bytes for domain ACL named %s", sizeof(struct aclEntry), domainName);
    if(domainName != NULL && *domainName == '.')
	domainName++;
    acl->type = domain;
    acl->acl.domain = strdup(domainName);
    return(acl);
}

struct aclEntry *CreateNetworkACL(int i1, int i2, int i3, int i4, int bits)
{
    struct aclEntry *acl;

    DebugPrintf(3, 100, "Creating network ACL for %d.%d.%d.%d/%d\n", i1, i2, i3, i4, bits);
    if((acl = malloc(sizeof(struct aclEntry))) == NULL)
	FatalError("could not malloc %d bytes for network ACL", sizeof(struct aclEntry));
    acl->type = network;
    acl->acl.network.network = inet_makeaddr((i1 << 24) | (i2 << 16) | (i3 << 8) | i4, 0);
    acl->acl.network.bits = bits;
    DebugPrintf(3, 250, "Made network ACL: %x/%d\n", acl->acl.network.network.s_addr, acl->acl.network.bits);
    return(acl);
}

struct aclList *AppendACL(struct aclList *list, struct aclEntry *acl)
{
    struct aclList *new;

    if(acl == NULL) return(list);
    if((new = malloc(sizeof(struct aclList))) == NULL)
	FatalError("could not malloc %d bytes for new ACL", sizeof(struct aclList));
    new->acl = acl;
    new->next = NULL;
    if(list == NULL) list = new;
    else {
	struct aclList *p = list;
	while(p->next != NULL) p = p->next;
	p->next = new;
    }
    return(list);
}

void MakeACL(char *name, struct aclList *list)
{
    struct acls *new;

    if(name == NULL || *name == '\0') return;
    if(LookupACL(name) != NULL) {
	ErrorMessage("ACL '%s' already defined", name);
	return;
    }
    if((new = malloc(sizeof(struct acls))) == NULL)
	FatalError("could not malloc %d bytes for ACL named %s", sizeof(struct acls), name);
    new->name = strdup(name);
    new->list = list;
    new->next = NULL;
    /* Append the ACL on the global list */
    if(acls == NULL) acls = new;
    else {
	struct acls *p = acls;
	while(p->next != NULL) p = p->next;
	p->next = new;
    }
}

void MakeFacility(int bitmap, struct program *programs)
{
    struct facility *new;

    if((new = malloc(sizeof(struct facility))) == NULL)
	FatalError("could not malloc %d bytes for a facility", sizeof(struct facility));
    new->bitmap = bitmap;
    new->programs = programs;
    new->next = NULL;
    /* Append the facility on the global facility list */
    if(facilities == NULL) facilities = new;
    else {
	struct facility *p = facilities;
	while(p->next != NULL) p = p->next;
	p->next = new;
    }
}

struct facility *AppendFacility(struct facility *list, struct facility *new)
{
    struct facility *p = list;

    if(new == NULL) return(list);
    if(list == NULL) return(new);
    while(p->next != NULL) p = p->next;
    p->next = new;
    return(list);
}

struct programNames *MakeProgramName(const char *name)
{
    struct programNames *new;

    if((new = malloc(sizeof(struct programNames))) == NULL)
	FatalError("could not malloc %d bytes for programNames: %s", sizeof(struct programNames), name);
    if(name == NULL) new->name = NULL;
    else new->name = strdup(name);
    new->next = NULL;
    return(new);
}

struct programNames *AppendProgramName(struct programNames *list, struct programNames *new)
{
    struct programNames *p = list;

    if(new == NULL) return(list);
    if(list == NULL) return(new);
    while(p->next != NULL) p = p->next;
    p->next = new;
    return(list);
}

struct priority *MakePriority(enum comparisons comparison, int priority, struct actions *actions)
{
    struct priority *new;

    if(actions == NULL) return(NULL);
    if((new = malloc(sizeof(struct priority))) == NULL)
	FatalError("could not malloc %d bytes for a priority", sizeof(struct priority));
    new->comparison = comparison;
    new->priority = priority;
    new->actions = actions;
    new->next = NULL;
    return(new);
}

struct priority *AppendPriority(struct priority *list, struct priority *new)
{
    struct priority *p = list;

    if(new == NULL) return(list);
    if(list == NULL) return(new);
    while(p->next != NULL) p = p->next;
    p->next = new;
    return(list);
}

struct actions *MakeActions(struct action *actions)
{
    struct actions *new;
    struct action *p;

    if((new = malloc(sizeof(struct actions))) == NULL)
	FatalError("could not malloc %d bytes for an action", sizeof(struct action));
    new->actions = actions;
    new->previousMessage[0] = '\0';
    new->previousHost[0] = '\0';
    new->previousProgram[0] = '\0';
    new->previousFacility = 0;
    new->previousPriority = 0;
    new->duplicateCount = 0;
    new->timeLogged = 0;
    new->hasBreak = false;
    /*
    ** If the action list contains a break, record that so we don't
    ** have to search for it later
    */
    for(p = actions; p != NULL; p = p->next)
	if(p->type == actionBreak) new->hasBreak = true;
    return(new);
}

struct action *MakeAction(enum actionTypes type, const char *dest)
{
    struct action *new;
    struct hostent *host;
    
    if((new = malloc(sizeof(struct action))) == NULL)
	FatalError("could not malloc %d bytes for an action", sizeof(struct action));
    new->type = type;
    new->dest = strdup(dest);
    new->options = actionOptions;
    new->next = NULL;
    switch(type) {
	case actionFile:
	    new->action.file = open(new->dest, O_WRONLY|O_APPEND|O_CREAT|O_NOCTTY, logFileMode);
	    DebugPrintf(3, 25, "Opened file %s: %d\n", new->dest, new->action.file);
	    if(new->action.file < 0)
		ErrorMessage("Could not open file %s:", new->dest);
	    break;
	case actionFIFO:
	    new->action.file = open(new->dest, O_RDWR);
	    DebugPrintf(3, 25, "Opened FIFO %s: %d\n", new->dest, new->action.file);
	    if(new->action.file < 0)
		ErrorMessage("Could not open FIFO %s:", new->dest);
	    break;
	case actionForward:
	    DebugPrintf(3, 25, "Forwarding to %s port %d\n", dest, ntohs(new->options.port));
	    if(new->options.port == 0) {
		ErrorMessage("could not find service network forwarding to %s disabled");
		free(new);
		new = NULL;
		break;
	    }
	    
	    DebugPrintf(3, 50, "Looking up host %s\n", new->dest);
	    bzero((char *) &new->action.forward, sizeof(new->action.forward));
	    new->action.forward.sin_port = new->options.port;
	    new->action.forward.sin_family = AF_INET;
	    host = gethostbyname(new->dest);
	    if(host != NULL) {
		bcopy(host->h_addr_list[0], &new->action.forward.sin_addr, host->h_length);
		DebugPrintf(3, 50, "  Lookup succeeded: %s\n", inet_ntoa(new->action.forward.sin_addr));
	    }
	    else {
		ErrorMessage("host lookup for %s failed, forwarding to this host disabled", new->dest);
		free(new);
		new = NULL;
		break;
	    }
	    break;
	case actionNone:
	    /* Nothing needs to be done for a "none" action  :) */
	    break;
	case actionBreak:
	    /* Just setting the action type suffices for the "break" action. */
	    break;
    }
    return(new);
}

struct action *AppendAction(struct action *list, struct action *new)
{
    struct action *p = list;

    if(new == NULL) return(list);
    if(list == NULL) return(new);
    while(p->next != NULL) p = p->next;
    p->next = new;
    return(list);
}

struct program *MakeProgram(struct programNames *programNames, struct priority *priorities)
{
    struct program *new;

    if(programNames == NULL || priorities == NULL) return(NULL);
    if((new = malloc(sizeof(struct program))) == NULL)
	FatalError("could not malloc %d bytes for a program", sizeof(struct program));
    new->programNames = programNames;
    new->priorities = priorities;
    new->next = NULL;
    return(new);
}

struct program *AppendProgram(struct program *list, struct program *new)
{
    struct program *p = list;

    if(new == NULL) return(list);
    if(list == NULL) return(new);
    while(p->next != NULL) p = p->next;
    p->next = new;
    return(list);
}

void InitActionOptions(void)
{
    extern bool syncEachMessage;
    
    actionOptions.port = syslogPort;
    actionOptions.forwardWithHostname = false;
    actionOptions.syncEachMessage = syncEachMessage;
    actionOptions.logWithPriority = unset;
}
