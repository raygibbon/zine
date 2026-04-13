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

/* $Id: local.c,v 1.25 1998/12/03 15:07:22 rand Exp $ */

static char *RCSid __attribute__ ((unused)) = "$Id: local.c,v 1.25 1998/12/03 15:07:22 rand Exp $";

#include "syslogd.h"

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <strings.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#if !defined(WATT32)
#include <sys/un.h>
#endif
#if defined(HAVE_SYS_STRLOG_H)
#include <sys/strlog.h>
#endif
#if defined(HAVE_SYS_STROPTS_H)
#include <stropts.h>
#include <sys/stropts.h>
#endif
#if defined(HAVE_SYS_LOG_H)
#include <sys/log.h>
#endif

static localFD = -1;
static klogFD = -1;
static char **partialLines = NULL;

int InitLocal(const char *path)
{
#if defined(STREAMS_LOGGING)
    struct strioctl ioc;

    DebugPrintf(1, 10, "Opening streams %s\n", path);
    localFD = open(path, O_RDONLY);
    if(localFD < 0) {
	ErrorMessage("could not open streams device %s, local messages will be lost:", path);
	return(localFD);
    }
    ioc.ic_cmd = I_CONSLOG;
    ioc.ic_timout = 0;
    ioc.ic_len = 0;
    ioc.ic_dp = NULL;
    if(ioctl(localFD, I_STR, &ioc) < 0) {
	ErrorMessage("could register %s as a console logger:", path);
	localFD = -1;
	return(localFD);
    }
    if(ioctl(localFD, I_SRDOPT, RMSGD) < 0) {
	ErrorMessage("could not set read mode of %s to message-discard mode:", path);
	localFD = -1;
	return(localFD);
    }
    return(localFD);
#endif
#if defined(SOCKET_DGRAM_LOGGING)
    struct sockaddr_un sock;

    DebugPrintf(1, 10, "Opening datagram socket %s\n", path);
    unlink(path);
    memset(&sock, 0, sizeof(sock));
    strncpy(sock.sun_path, path, sizeof(sock.sun_path));
    sock.sun_path[sizeof(sock.sun_path) - 1] = '\0';
    sock.sun_family = AF_UNIX;
    localFD = socket(AF_UNIX, SOCK_DGRAM, 0);
    if(localFD < 0) {
	ErrorMessage("could not create socket: %s, local messages will be lost:", path);
	return(localFD);
    }
    if(bind(localFD, (struct sockaddr *) &sock, SUN_LEN(&sock)) < 0) {
	ErrorMessage("could not bind socket: %s, local messages will be lost:", path);
	localFD = -1;
	return(localFD);
    }
    if(chmod(path, logDeviceMode) < 0) {
	ErrorMessage("could not chmod: %s, local messages may be lost:", path);
    }
    DebugPrintf(1, 20, "socket file descriptor: %d\n", localFD);
    return(localFD);
#endif
#if defined(SOCKET_STREAM_LOGGING)
    struct sockaddr_un sock;

    DebugPrintf(1, 10, "Opening stream socket %s\n", path);
    unlink(path);
    memset(&sock, 0, sizeof(sock));
    strncpy(sock.sun_path, path, sizeof(sock.sun_path));
    sock.sun_path[sizeof(sock.sun_path) - 1] = '\0';
    sock.sun_family = AF_UNIX;
    localFD = socket(AF_UNIX, SOCK_STREAM, 0);
    if(localFD < 0) {
	ErrorMessage("could not create socket: %s, local messages will be lost:", path);
	return(localFD);
    }
    if(bind(localFD, (struct sockaddr *) &sock, sizeof(sock.sun_family) + strlen(sock.sun_path)) < 0) {
	ErrorMessage("could not bind socket: %s, local messages will be lost:", path);
	localFD = -1;
	return(localFD);
    }
    if(chmod(path, logDeviceMode) < 0) {
	ErrorMessage("could not chmod: %s, local messages may be lost:", path);
    }
    if(listen(localFD, socketBacklog) < 0) {
	ErrorMessage("could not listen on %s, local messages will be lost:", path);
	localFD = -1;
	return(localFD);
    }
    DebugPrintf(1, 20, "socket file descriptor: %d\n", localFD);
    return(localFD);
#endif
#if defined(FIFO_LOGGING)
    DebugPrintf(1, 10, "Opening FIFO %s\n", path);
    unlink(path);
    if(mkfifo(path, 0666) < 0) {
	ErrorMessage("could not create FIFO: %s, local messages will be lost:", path);
	return(localFD);
    }
    if(chmod(path, 0666) < 0)
	ErrorMessage("could not chmod(%s,0666), local messages may be lost:", path);
    /*
    ** Got to open the FIFO for read & write even though all we ever
    ** do is read from it because if its opened for readonly after a
    ** single message the select will always report that there is data
    ** to read.
    */
    localFD = open(path, O_RDWR);
    if(localFD < 0) {
	ErrorMessage("Could not open FIFO %s, local messages will be lost:", path);
	return(localFD);
    }
    return(localFD);
#endif
}

int InitKlog(const char *path)
{
#if defined(KLOG_LOGGING)
    DebugPrintf(1, 10, "Opening kernel device %s\n", path);
    klogFD = open(path, O_RDONLY);
    if(klogFD < 0)
	ErrorMessage("Could not open kernel device %s, kernel messages will be lost:", path);
#endif
    return(klogFD);
}

#if defined(STREAMS_LOGGING)
void GetStreamsMessage(void)
{
    char buffer[MAXMESSAGE + 1];
    struct strbuf data, ctl;
    struct log_ctl control;
    struct message m;
    int status;
    int flags = 0;
    char *p;

    data.buf = buffer;
    data.maxlen = sizeof(buffer) - 1;
    ctl.buf = (char *) &control;
    ctl.maxlen = sizeof(control);

    status = getmsg(localFD, &ctl, &data, &flags);
    while(buffer[data.len - 1] == '\0' || isspace(buffer[data.len - 1]))
	data.len--;
    buffer[data.len] = '\0';
    DebugPrintf(11, 1, "facility: %s   priority: %s\n",
		facilityNames[LOG_FAC(control.pri)].name, priorityNames[LOG_PRI(control.pri)].name);
    DebugPrintf(11, 1, "priority: %d   time: %d\n", control.pri, control.ttime);
    m.facility = LOG_FAC(control.pri);
    m.priority = LOG_PRI(control.pri);
    m.host = hostName;
    for(p = strtok(buffer, "\n\r"); p != NULL; p = strtok(NULL, "\n\r")) {
	DebugPrintf(11, 1, "[%s]\n", p);
	localBytesReceived += strlen(p);
	localMessagesReceived++;
	m.message = p;
	m.length = strlen(p);
	m.flags = FLAG_SET_FACPRI;
	if(!ValidTimestamp(p))
	    m.flags |= FLAG_ADD_DATE;
	m.forwardOK = true;
	DoMessage(&m);
    }
}
#endif

#if defined(FIFO_LOGGING)
void GetFIFOMessage(void)
{
    int chars;
    static char buffer[MAXMESSAGE + 1];
    struct message m;
    
    chars = read(localFD, buffer, sizeof(buffer) - 1);
    DebugPrintf(11, 100, "Got a %d byte message from FIFO\n", chars);
    if(chars < 0) {
	DebugPrintf(11, 10, "difficulities with read(FIFO): %s\n", strerror(errno));
	if(errno != EINTR) ErrorMessage("read(FIFO):");
	return;
    }
    if(chars == 0)
	DebugPrintf(11, 100, "Got a 0 byte message!\n");
    buffer[chars] = '\0';
    /* For some reason we end up with lots of trailing \0's, so "truncate" them */
    chars = strlen(buffer);
    DebugPrintf(11, 1000, "Message: '%s'\n", buffer);
    localBytesReceived += chars;
    localMessagesReceived++;
    m.message = buffer;
    m.length = chars;
    m.host = hostName;
    m.forwardOK = true;
    m.flags = 0;
    DoMessage(&m);
}
#endif

#if defined(SOCKET_DGRAM_LOGGING)
void GetDgramMessage(void)
{
    int chars;
    static char buffer[MAXMESSAGE + 1];
    struct sockaddr_un from;
    int size = sizeof(from);
    struct message m;

    chars = recvfrom(localFD, buffer, sizeof(buffer) - 1, 0, (struct sockaddr *) &from, &size);
    DebugPrintf(11, 100, "Got a %d byte message from socket\n", chars);
    if(chars < 0) {
	DebugPrintf(11, 10, "difficulities with recvfrom(local): %s\n", strerror(errno));
	if(errno != EINTR) ErrorMessage("recvfrom(local):");
	return;
    }
    buffer[chars] = '\0';
    DebugPrintf(11, 1000, "Message: '%s'\n", buffer);
    localBytesReceived += chars;
    localMessagesReceived++;
    m.message = buffer;
    m.length = chars;
    m.host = hostName;
    m.forwardOK = true;
    m.flags = 0;
    DoMessage(&m);
}
#endif

#if defined(SOCKET_STREAM_LOGGING)
void OpenNewSocket(fd_set *vector)
{
    struct sockaddr_un sock;
    int length;
    int fd;
    
    DebugPrintf(11, 100, "Opening a new socket connection\n");
    length = sizeof(sock);
    fd = accept(localFD, (struct sockaddr *) &sock, &length);
    if(fd >= 0) {
	DebugPrintf(11, 100, "Accepted new socket connection, file descriptor %d\n", fd);
	FD_SET(fd, vector);
    }
    else ErrorMessage("accept(localFD):");
}

void GetSocketMessage(int fd, fd_set *vector)
{
    int chars;
    static char buffer[MAXMESSAGE + 1];
    
    chars = read(fd, buffer, sizeof(buffer) - 1);
    DebugPrintf(11, 100, "Got a %d byte message from socket, file descriptor %d\n", chars, fd);
    if(chars == 0) {
	DebugPrintf(11, 100, "Closing socket file descriptor %d\n", fd);
	FD_CLR(fd, vector);
	close(fd);
    }
    else if(chars > 0) {
	buffer[chars] = '\0';
	DebugPrintf(11, 1000, "Message: '%s'\n", buffer);
	AccumulateMessage(fd, buffer, chars);
    }
    else {
	DebugPrintf(11, 10, "difficulities with read(socket fd #%d): %s\n", fd, strerror(errno));
	if(errno != EINTR) ErrorMessage("read(socket: %d):", fd);
    }
}
#endif

void GetKlogMessage(void)
{
#if defined(KLOG_LOGGING)
    int chars;
    char *p;
    static char buffer[MAXMESSAGE + 1];
    struct message m;
    
    chars = read(klogFD, buffer, sizeof(buffer) - 1);
    DebugPrintf(11, 100, "Got a %d byte message from the kernel\n", chars);
    if(chars < 0) {
	DebugPrintf(11, 10, "difficulities with read(klogFD): %s\n", strerror(errno));
	if(errno != EINTR) ErrorMessage("read(klogFD):");
	return;
    }
    buffer[chars] = '\0';
    /* For some reason we end up with lots of trailing \0's, so "truncate" them */
    chars = strlen(buffer);
    DebugPrintf(11, 100, "Message: '%s'\n", buffer);
    m.host = hostName;
    m.forwardOK = true;
    for(p = strtok(buffer, "\n\r"); p != NULL; p = strtok(NULL, "\n\r")) {
	DebugPrintf(11, 500, "Message: [%s]\n", p);
	localBytesReceived += strlen(p);
	localMessagesReceived++;
	m.message = p;
	m.length = strlen(p);
	m.flags = FLAG_ADD_DATE;
	/* Does the message have a facility and priority? */
	if(buffer[0] != '<') {
	    m.flags |= FLAG_SET_FACPRI;
	    m.facility = LOG_FAC(LOG_KERN);
	    m.priority = LOG_PRI(LOG_NOTICE);
	}
	DoMessage(&m);
    }
#endif
}

void AccumulateMessage(int fd, char *message, int size)
{
    int length;
    int i;
    int numberFDs;
    struct message m;

    DebugPrintf(4, 1, "AccumulateMessage(%d, '%s', %d)\n", fd, message, size);
    if(partialLines == NULL) {
	/* Make a table of partial lines for each possible file descriptor. */
	numberFDs = getdtablesize();
	partialLines = malloc(numberFDs * sizeof(char *));
	if(partialLines == NULL)
	    FatalError("could not allocate %d bytes for partialLines, terminating", numberFDs * sizeof(char *));
	for(i = 0; i < numberFDs; i++)
	    partialLines[i] = NULL;
    }
    
    if(partialLines[fd] == NULL) {
	/*
	** Allocate space for a line only when we need it, with reused file descriptors
	** the table shouldn't grow to be too large. We don't attempt to shrink the table.
	*/
	partialLines[fd] = malloc(MAXMESSAGE + 1);
	if(partialLines[fd] == NULL) {
	    ErrorMessage("could not allocate %d bytes for partialLines[%d]", MAXMESSAGE + 1, fd);
	    return;
	}
	bzero(partialLines[fd], MAXMESSAGE + 1);
    }
    
    length = strlen(partialLines[fd]);
    if(length + size >= MAXMESSAGE + 1) {
	ErrorMessage("message too long (%d >= %d) truncating", length + size, MAXMESSAGE + 1);
	return;
    }
    
    strncat(partialLines[fd], message, size);
    partialLines[fd][length + size] = '\0';
    /*
    ** Step through the message, stopping *before* we reach the
    ** terminating NUL we added on the end.  Any other NULs delimit
    ** messages, and we may have to accumulate a message between
    ** calls.
    */
    m.host = hostName;
    m.flags = 0;
    m.forwardOK = true;
    for(i = 0; i < size; i++) {
	if(message[i] == '\0') {
	    /* OK, got the trailing NUL, the message is complete */
	    DebugPrintf(4, 10, "got a complete message fd %d: '%s'\n", fd, partialLines[fd]);
	    localBytesReceived += length + size - 1;
	    localMessagesReceived++;
	    m.message = partialLines[fd];
	    m.length = length + size - 1;
	    DoMessage(&m);
	    partialLines[fd][0] = '\0';
	    strncpy(partialLines[fd], message + i + 1, size - (i + 1));
	    partialLines[fd][size - (i + 1)] = '\0';
	    DebugPrintf(4, 100, "partialLines[%d] after: '%s'\n", fd, partialLines[fd]);
	}
    }
}
