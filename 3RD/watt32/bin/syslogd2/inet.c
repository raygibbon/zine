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

/* $Id: inet.c,v 1.37 1998/11/30 22:02:47 rand Exp $ */

static char *RCSid __attribute__ ((unused)) = "$Id: inet.c,v 1.37 1998/11/30 22:02:47 rand Exp $";

#include "syslogd.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <netinet/in.h>
#include <arpa/inet.h>

static int InetFD = -1;
static int ForwardFD = -1;
static int ForwardingOK = true;

int InitInet(void)
{
    struct servent *service;
    struct sockaddr_in sock;
 
    if(!inetReceive) {
	DebugPrintf(1, 10, "inetReceive false, not listening for UDP syslogd packets\n");
	InetFD = -1;
	return(InetFD);
    }
    DebugPrintf(1, 10, "Opening inet socket ...\n");
    InetFD = socket(AF_INET, SOCK_DGRAM, 0);
    if(InetFD < 0) {
	ErrorMessage("could not create inet socket, network logging disabled:");
	return(InetFD);
    }
    service = getservbyname("syslog", "udp");
    if(service == NULL) {
	ErrorMessage("could not find service syslog/udp, network logging disabled");
	InetFD = -1;
	return(InetFD);
    }
    sock.sin_port = service->s_port;
    sock.sin_addr.s_addr = 0;
    sock.sin_family = AF_INET;
    if(bind(InetFD, (struct sockaddr *) &sock, sizeof(sock)) < 0) {
	ErrorMessage("could not bind inet socket, network logging disabled:");
	InetFD = -1;
	return(InetFD);
    }
    DebugPrintf(1, 10, "listening on syslog UDP port (%d)\n", service->s_port);
    return(InetFD);
}

void GetInetMessage(void)
{
    int chars;
    static char message[MAXMESSAGE + 1];
    static struct sockaddr_in from;
    static int fromSize = sizeof(struct sockaddr_in);

    chars = recvfrom(InetFD, message, sizeof(message) - 1, 0, (struct sockaddr *)&from, &fromSize);
    DebugPrintf(5, 100, "Got a %d byte inet message from %s\n", chars, inet_ntoa(from.sin_addr));
    if(chars >= 0) {
	message[chars] = '\0';
	DebugPrintf(5, 1000, "Message: '%s'\n", message);
    }
    else {
	DebugPrintf(5, 100, "difficulities with recvfrom(InetFD): %s\n", strerror(errno));
	if(errno != EINTR) ErrorMessage("recvfrom(InetFD):");
    }
    if(CheckCache(inetReceiveFrom, from.sin_addr)->status)
	DoInetMessage(message, chars, GetHostName(from.sin_addr), from.sin_addr);
    else if(logUnwantedHosts)
	ErrorMessage("Got an unwanted syslog message from %s [%s]",
		     GetHostName(from.sin_addr), inet_ntoa(from.sin_addr));
}

void DoInetMessage(char *message, int length, char *host, struct in_addr address)
{
    struct message m;
    inetBytesReceived += length;
    inetMessagesReceived++;
    if(!SimpleHostname(host, address))
	StripDomains(host, address);
    m.message = message;
    m.length = length;
    m.host = host;
    m.forwardOK = CheckCache(inetForwardFrom, address)->status;
    DoMessage(&m);
}

bool SimpleHostname(char *host, struct in_addr address)
{
    char *p;

    if(CheckCache(simpleHostnames, address)->status) {
	p = strchr(host, '.');
	DebugPrintf(6, 500, "Simplehostname:  %s --> ", host);
	if(p != NULL) *p = '\0';
	DebugPrintf(6, 500, "%s\n", host);
	return(true);
    }
    return(false);
}

void StripDomains(char *host, struct in_addr address)
{
    struct hash *cache;

    cache = CheckCache(stripDomains, address);
    if(cache->status) {
	DebugPrintf(6, 500, "StripDomains: %s --> ", host);
	/*
	** cache->name will always be shorter than host because it
	** will either be the same as host, or it will have some
	** domains removed. OK, so if the name of an IP address
	** changes during the execution of syslogd this might fail.
	** So we only copy MAXHOSTNAMELEN characters, but I don't know
	** how big gethostbyname makes the h_name entry.  Sigh.
	*/
	if(cache->name) strncpy(host, cache->name, MAXHOSTNAMELEN);
	else {
	    /*
	    ** First time we've seen this host, now we need to see if
	    ** we should remove part of the domain.
	    */
	    struct aclList *acl = stripDomains->acl;
	    while(acl != NULL) {
		if(acl->acl->type == domain) {
		    char *p = host + strlen(host) - 1;
		    char *q = acl->acl->acl.domain + strlen(acl->acl->acl.domain) - 1;
		    /*
		    ** p and q point to the end of host and the domain
		    ** specified in the ACL, now going backwards
		    ** compare the strings.
		    */
		    while(p >= host && q >= acl->acl->acl.domain &&
			  tolower(*p) == tolower(*q)) {
			p--;
			q--;
		    }
		    /*
		    ** If p is at a dot and we exhausted the domain
		    ** acl truncate part of the domain and save it for
		    ** next time
		    */
		    if(p >= host && *p == '.' && q < acl->acl->acl.domain) {
			*p = '\0';
			cache->name = strdup(host);
			break;
		    }
		}
		/*
		** Hmm, the acl type isn't "domain", so instead of
		** complaining we just strip the entire domain. Easy,
		** and it kind of makes sense.
		*/
		else {
		    char *p = strchr(host, '.');
		    if(p != NULL) *p = '\0';
		    cache->name = strdup(host);
		    break;
		}
		acl = acl->next;
	    }
	}
	DebugPrintf(6, 500, "%s\n", host);
    }
}

/*
** Lookup the IP address (not the name) of a host in the cache, if not 
** found update the cache with the results of processing the ACL. Note 
** that even hosts that fail to pass the ACL are stored in the
** cache. Collisions are stored in a linked list. We return the cache
** entry because StipDomains needs to update the entry.  Sigh.
*/
struct hash *CheckCache(struct hashTable *table, struct in_addr address)
{
    int index;
    struct hash *p;
    struct hash *new;

    if(table == NULL || table->cache == NULL) return(NULL);
    CheckCacheSize(table);
    index = address.s_addr % hashTableSize;
    p = table->cache[index];
    DebugPrintf(6, 100, "Looking up %s in cache  index: %d  hashTableSize: %d\n",
		inet_ntoa(address), index, hashTableSize);
    if(p != NULL) {
	DebugPrintf(6, 1000, "bucket not empty\n");
	while(p->address.s_addr != address.s_addr && p->next != NULL) p = p->next;
	if(p->address.s_addr == address.s_addr) {
	    p->hits++;
	    DebugPrintf(6, 500, "Found %s  hits: %u  status: %s\n",
			inet_ntoa(address), p->hits, p->status ? "true" : "false");
	    return(p);
	}
    }
    DebugPrintf(6, 500, "Didn't find %s, adding it\n", inet_ntoa(address));
    if((new = malloc(sizeof(struct hash))) == NULL)
	FatalError("could not malloc %d bytes for CheckCache:hash entry", sizeof(struct hash));
    table->size++;
    new->address = address;
    new->hits = 1;
    new->status = CheckACL(table->acl, address);
    new->name = NULL;
    new->next = NULL;
    if(p == NULL) table->cache[index] = new;
    else p->next = new;
    return(new);
}

char *GetHostName(struct in_addr address)
{
    struct hostent *host;
    char *p;
    
    host = gethostbyaddr((char *) &address, sizeof(address), AF_INET);
    if(host == NULL)
	return(inet_ntoa(address));
    p = host->h_name;
    return(host->h_name);
}

bool CheckACL(struct aclList *acl, struct in_addr address)
{
    struct aclList *p = acl;
    bool status;
    
    while(p != NULL) {
	switch(p->acl->type) {
	    case host:
		status = CheckHostACL(p->acl->acl.host, address);
		break;
	    case domain:
		status = CheckDomainACL(p->acl->acl.domain, address);
		break;
	    case network:
		status = CheckNetworkACL(p->acl->acl.network, address);
		break;
	    default:
		ErrorMessage("Unknown ACL type %d", p->acl->type);
		status = false;
	}
	DebugPrintf(6, 10, "ACL %s\n", status ? "matched" : "didn't match");
	if(status) return(true);
	p = p->next;
    }
    return(false);
}

bool CheckNetworkACL(struct aclNetwork acl, struct in_addr address)
{
    u_long m;
    struct in_addr mask;

    m = 0xffffffffU;
    m >>= (32 - acl.bits);
    m <<= (32 - acl.bits);
    mask = inet_makeaddr(m ,0);
    DebugPrintf(6, 100, "Checking network ACL  network: %s\n", inet_ntoa(acl.network));
    DebugPrintf(6, 100, "                      host: %s\n", inet_ntoa(address));
    DebugPrintf(6, 100, "                      mask: %s   bits: %d\n", inet_ntoa(mask), acl.bits);
    return((acl.network.s_addr & mask.s_addr) == (address.s_addr & mask.s_addr));
}

bool CheckDomainACL(char *domain, struct in_addr address)
{
    char *hostname;
    char *p, *q;

    hostname = GetHostName(address);
    DebugPrintf(6, 100, "Checking domain ACL   domain: %s   hostname: %s\n", domain, hostname);
    p = hostname + strlen(hostname) - 1;
    q = domain + strlen(domain) - 1;
    while(p >= hostname && q >= domain &&
	  tolower(*p) != tolower(*q)) {
	p--;
	q--;
    }
    if(p >= hostname && *p == '.' && q < domain)
	return(true);
    else return(false);
}

/*
** Borrowed host wildcards from Linux's /etc/exports wildcards.  I'm
** not sure who the author is, but whoever it is deserves kudos for a
** simple yet powerful idea. Note that wildcards will not span a dot.
*/
bool CheckHostACL(char *pattern, struct in_addr address)
{
    char *hostname;

    hostname = GetHostName(address);
    if(pattern == NULL || hostname == NULL) return(pattern == hostname);
    DebugPrintf(6, 100, "Checking host ACL   pattern: %s   hostname: %s\n", pattern, hostname);
    while(*hostname != '\0' && *pattern != '\0') {
	switch(*pattern) {
	    case '*':
		/* The '*' wildcard only matches up to the next '.' */
		while(*hostname != '\0' && *hostname != '.') hostname++;
		pattern++;
		break;
	    case '?':
		/* The '?' wildcard can't match a '.' */
		if(*hostname == '.') return(false);
		hostname++;
		pattern++;
		break;
	    default:
		if(tolower(*hostname) != tolower(*pattern)) return(false);
		hostname++;
		pattern++;
		break;
	}
    }
    if(*hostname == '\0' && *pattern == '\0') return(true);
    else return(false);
}

void TestACLs(struct in_addr address)
{
    struct acls *acl = acls;

    while(acl != NULL) {
	DebugPrintf(6, 10, "Checking host %s against acl %s\n", GetHostName(address), acl->name);
	CheckACL(acl->list, address);
	acl = acl->next;
    }
}

void DoForwardAction(struct action *action, struct message *m)
{
    int blength;
    int sent;
    static char buffer[MAXMESSAGE + 1];

    if(ForwardFD == -1 && ForwardingOK) {
	ForwardFD = socket(AF_INET, SOCK_DGRAM, 0);
	if(ForwardFD < 0) {
	    ErrorMessage("could not create inet socket, network forwarding disabled:");
	    ForwardingOK = false;
	}
    }
    if(!ForwardingOK || !m->forwardOK) return;
    DebugPrintf(8, 500, "DoForwardAction sending a message to %s port %d\n",
		action->dest, ntohs(action->action.forward.sin_port));
    buffer[0] = '\0';
    blength = strlen(buffer);
    /* Add the numeric priority up front unless we are doing symbolic priorities. */
    if(action->options.logWithPriority == unset || action->options.logWithPriority == numeric) {
	snprintf(buffer + blength, sizeof(buffer) - blength, "<%d>", LOG_MAKEPRI(m->facility, m->priority));
	blength = strlen(buffer);
    }
    /* Add the time stamp of the message */
    snprintf(buffer + blength, sizeof(buffer) - blength, "%s", m->timestamp);
    blength = strlen(buffer);
    /* Add the host name if requested. non-standard */
    if(action->options.forwardWithHostname || action->options.logWithPriority == symbolic) {
	snprintf(buffer + blength, sizeof(buffer) - blength, " %s", m->host);
	blength = strlen(buffer);
    }
    /* Add the facility and priority in symbolic form.  non-standard */
    if(action->options.logWithPriority == symbolic) {
	snprintf(buffer + blength, sizeof(buffer) - blength, "[%s.%s] ",
		 facilityNames[m->facility].name, priorityNames[m->priority].name);
	blength = strlen(buffer);
    }
    else {
	snprintf(buffer + blength, sizeof(buffer) - blength, " ");
	blength = strlen(buffer);
    }
    /* Finally, add the message followed by a newline. */
    snprintf(buffer + blength, sizeof(buffer) - blength, "%s\n", m->message);
    blength = strlen(buffer);
    
    sent = sendto(ForwardFD, buffer, blength, 0, (struct sockaddr *) &action->action.forward,
	      sizeof(action->action.forward));
    DebugPrintf(8, 100, "sendto(%s, %d) sent %d bytes\n", action->dest, blength + m->length, sent);
    /*
    ** Interestingly, the sendto above won't immediately notice a
    ** problem, but the next recvfrom() or sendto() will. So, the only
    ** way to detect a problem forwarding a message is to forward two
    ** in a row. Otherwise the problem will be detected by the next
    ** time the system calls recvfrom() and we won't know which host
    ** had problems.
    */
    if(sent != blength + m->length) {
	DebugPrintf(8, 0, "sendto(%s, %d) returned %d  (err: %s)\n",
		    action->dest, blength + m->length, sent, strerror(errno));
    }
}

unsigned short GetPortByName(char *name)
{
    struct servent *service;
    char *p;
    unsigned short port = 0;

    if(name != NULL && *name != '\0') {
	if((service = getservbyname(name, "udp")) != NULL)
	    port = service->s_port;
	else {
	    bool number = true;
	    for(p = name; *p != '\0'; p++)
		if(!isdigit(*p)) number = false;
	    if(number) port = htons(atoi(name));
	}
    }
    DebugPrintf(10, 100, "GetPortByName(%s): %d\n", name, ntohs(port));
    return(port);
}
