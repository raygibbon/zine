/*
** syslogd -- system logging daemon
** Copyright (c) 1998 Douglas K. Rand   <rand@aero.und.edu>
** All Rights Reserved.
*/

/*
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License as
** published by the Free Software Foundation; either version 2 of the
** License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful, but
** WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
** General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; see the file COPYING.  If not, write to
** the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
** Boston, MA 02111-1307, USA.
*/

/* $Id: syslogd.c,v 1.71 1998/12/03 17:14:52 rand Exp $ */

/*
** Eric Allman originally wrote the first implementation of syslogd.
** I have borrowed many features and ideas from the FreeBSD and Linux
** (Dr. Greg Wettstein <greg@wind.rmcc.com> and Martin Schulze
** <joey@linux.de>) implementations.
**
** I first saw actions based on program names in the FreeBSD syslogd,
** those extenions were done by Peter da Silva.
*/

#include "syslogd.h"

char *Copyright = "Copyright (c) 1998 Douglas K. Rand <rand@aero.und.edu> All Rights Reserved.";
char *Version = "syslogd " SYSLOGD_VERSION;
static char *RCSid __attribute__ ((unused)) = "$Id: syslogd.c,v 1.71 1998/12/03 17:14:52 rand Exp $";

#include <stdio.h>
#if defined(HAVE_SYS_SIGEVENT_H)
#include <sys/sigevent.h>
#endif
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <limits.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <netinet/in.h>

#if defined(WATT32)
#include <tcp.h>   /* struct iovec, sock_ini() etc */
#else
#include <sys/uio.h>
#include <sys/un.h>
#endif

struct code facilityNames[] = {
    {"kern",	LOG_KERN,	BIT_KERN},	/* 0 */
    {"user",	LOG_USER,	BIT_USER},	/* 1 */
    {"mail",	LOG_MAIL,	BIT_MAIL},	/* 2 */
    {"daemon",	LOG_DAEMON,	BIT_DAEMON},	/* 3 */
    {"auth",	LOG_AUTH,	BIT_AUTH},	/* 4 */
    {"syslog",	LOG_SYSLOG,	BIT_SYSLOG},	/* 5 */
    {"lpr",	LOG_LPR,	BIT_LPR},	/* 6 */
    {"news",	LOG_NEWS,	BIT_NEWS},	/* 7 */
    {"uucp",	LOG_UUCP,	BIT_UUCP},	/* 8 */
    {"bsdcron",	LOG_BSDCRON,	BIT_BSDCRON},	/* 9 */
    {"authpriv",LOG_AUTHPRIV,	BIT_AUTHPRIV},	/* 10 */
    {"ftp",	LOG_FTP,	BIT_FTP},	/* 11 */
    {"ntp",	LOG_NTP,	BIT_NTP},	/* 12 */
    {"unused",	INTERNAL_NOPRI,	0},		/* 13 */
    {"unused",	INTERNAL_NOPRI,	0},		/* 14 */
    {"sysvcron",LOG_SYSVCRON,	BIT_SYSVCRON},	/* 15 */
    {"local0",	LOG_LOCAL0,	BIT_LOCAL0},	/* 16 */
    {"local1",	LOG_LOCAL1,	BIT_LOCAL1},	/* 17 */
    {"local2",	LOG_LOCAL2,	BIT_LOCAL2},	/* 18 */
    {"local3",	LOG_LOCAL3,	BIT_LOCAL3},	/* 19 */
    {"local4",	LOG_LOCAL4,	BIT_LOCAL4},	/* 20 */
    {"local5",	LOG_LOCAL5,	BIT_LOCAL5},	/* 21 */
    {"local6",	LOG_LOCAL6,	BIT_LOCAL6},	/* 22 */
    {"local7",	LOG_LOCAL7,	BIT_LOCAL7},	/* 23 */
    {NULL,	-1,		0}
};

struct code priorityNames[] = {
    {"emerg",	LOG_EMERG,	0},	/* 0 */
    {"alert",	LOG_ALERT,	0},	/* 1 */
    {"crit",	LOG_CRIT,	0},	/* 2 */
    {"err",	LOG_ERR,	0},	/* 3 */
    {"warning",	LOG_WARNING,	0},	/* 4 */
    {"notice",	LOG_NOTICE,	0},	/* 5 */
    {"info",	LOG_INFO,	0},	/* 6 */
    {"debug",	LOG_DEBUG,	0},	/* 7 */
    {"*none*",	INTERNAL_NOPRI,	0},	/* 8 */
    {NULL,	-1,		0}
};

struct code actionNames[] = {
    {"*none*",	actionNone,	0},	/* 0 */
    {"file",	actionFile,	0},	/* 1 */
    {"fifo",	actionFIFO,	0},	/* 2 */
    {"forward",	actionForward,	0},	/* 3 */
    {"break",	actionBreak,	0},	/* 4 */
    {NULL,	-1,		0}	/* 5 */
};

bool daemonize = true;				/* should we fork and dissociate from the tty */
bool gotAlarm = false;				/* We got an ALRM signal, the timer expired */
bool gotHangup = false;				/* We got a HUP signal */
bool gotUser1 = false;				/* We got a USR1 signal */
bool gotUser2 = false;				/* We got a USR2 signal */
bool inetForwarding = false;			/* should we forward inet messages we receive */
bool inetReceive = true;			/* should we accept messages on the UDP port */
bool logUnwantedHosts = false;			/* should we log which hosts are trying to log to us */
bool syncEachMessage = true;			/* should we sync() after writing a message */
bool upAndRunning = false;			/* are we ready to do work yet? */
char *configFile = PATH_SYSLOG_CONF;		/* path to configuration file */
char *directory = NULL;				/* where should we do a chdir(2) to */
char *hostName = NULL;				/* name of the system we are on */
char *klogDevice = PATH_KLOG;			/* name of the device for kernel messages */
char *logDevice = PATH_LOG;			/* path to local logging stream */
char *pidFile = PATH_PID;			/* path to file recording our PID */
char *progName;					/* name of our program, from argv[0] */
char *syslogPortName = "syslog";		/* name of the service to send/recieve messages on */
enum logWithPriorities logWithPriority = none;	/* should we log messages with the facility/priority */
int hashTableSize = HASHSIZE;			/* how large should the hash tables be */
int hashTableMaxSize = MAX_HASHSIZE;		/* The maximun number of entries in the hash table */
int lineNumber = 1;				/* the current line number in the config file */
int logDeviceMode = 0666;			/* what mode should the log devcie have after we create it */
int logFD;					/* file descriptor to log device */
int logFileMode = 0644;				/* what mode should a log file have if we create it */
int markInterval = 60;				/* how many seconds between logging marks */
int maxDuplicates = 0;				/* how many duplicate messages before marking as duplicate */
int numberActions = 0;				/* how many actions in actionNames */
int numberFacilities = 0;			/* how many facilities in facilityNames */
int numberPriorities = 0;			/* how many priorities in priorityNames */
int pid;					/* Our PID */
int repeatInterval = 60;			/* how many seconds to hold duplicate messages before flushing */
int socketBacklog = 10;				/* how big of a backlog does listen() need to maintain */
short unsigned syslogPort = 0;			/* which UDP port does syslogd use */
struct acls *acls = NULL;			/* list of all ACLs defined in syslogd.conf */
struct actionOptions actionOptions;		/* options for this indvidual action */
struct facility *facilities = NULL;		/* list of facilities to search for message actions */
struct hashTable *inetForwardFrom = NULL;	/* which hosts should we forward messages for */
struct hashTable *inetReceiveFrom = NULL;	/* which hosts should we acccept messages from */
struct hashTable *simpleHostnames = NULL;	/* which hosts should be logged with only their hostnames */
struct hashTable *stripDomains = NULL;		/* which hosts should we strip domains from */
time_t now;					/* what time is it "now" */
time_t started;					/* what time did syslogd start (or restart) */
unsigned long int inetBytesReceived = 0;	/* how many bytes of inet messages we've gotten */
unsigned long int inetMessagesReceived = 0;	/* how many inet messages we've gotten */
unsigned long int localBytesReceived = 0;	/* how many bytes of local messages we've gotten */
unsigned long int localMessagesReceived = 0;	/* how many local messages we've gotten */
unsigned long int totalBytesReceived = 0;	/* how many bytes of messages we've gotten */
unsigned long int totalMessagesReceived = 0;	/* how many messages we've gotten */

#if defined(__DJGPP__)
int writev (int f, struct iovec *iov, int len)
{
  int i, status, nwritten;

  for (nwritten = 0, i = 0; i < len; ++i, ++iov)
  {
    if ((status = write(f, iov->iov_base, iov->iov_len)) < 0)
       break;
    nwritten += status;
  }
  return (nwritten);
}
#endif

int main(int argc, char *argv[])
{
    int status;
    int localFD;
    int inetFD;
    int klogFD;
    fd_set *readVector;
#if defined(SOCKET_STREAM_LOGGING)
    fd_set *socketVector;
    int fd;
#endif
    struct timeval start, finish;
    float interval;

    /* Do these before the fork() in Init() */
    if((readVector = malloc(sizeof(fd_set))) == NULL)
	FatalError("could not malloc %d bytes for readVector", sizeof(fd_set));
#if defined(SOCKET_STREAM_LOGGING)
    if((socketVector = malloc(sizeof(fd_set))) == NULL)
	FatalError("could not malloc %d bytes for socketVector", sizeof(fd_set));
#endif
    
    Init(argc, argv);

    localFD = InitLocal(logDevice);
    inetFD = InitInet();
    klogFD = InitKlog(klogDevice);
#if defined(SOCKET_STREAM_LOGGING)
    FD_ZERO(socketVector);
#endif

    DebugPrintf(1, 0, "up and running...\n");

    while(1) {
	FD_ZERO(readVector);
#if defined(SOCKET_STREAM_LOGGING)
	/* There should be a better way than iterating through FD_SETSIZE */
	for(fd = 0; fd < FD_SETSIZE; fd++)
	    if(FD_ISSET(fd, socketVector))
		FD_SET(fd, readVector);
#endif
       	if(localFD >= 0) FD_SET(localFD, readVector);
       	if(inetFD >= 0) FD_SET(inetFD, readVector);
	if(klogFD >= 0) FD_SET(klogFD, readVector);

	gettimeofday(&start, NULL);
	interval = TIMEVAL2FLOAT(start) - TIMEVAL2FLOAT(finish);
	DebugPrintf(2, 10, "Processing took %0.4f seconds\n", interval);
	status = select(FD_SETSIZE, readVector, (fd_set *) NULL, (fd_set *) NULL, (struct timeval *) NULL);
	gettimeofday(&finish, NULL);
	interval = TIMEVAL2FLOAT(finish) - TIMEVAL2FLOAT(start);
	DebugPrintf(2, 10, "Select slept for %0.2f seconds, return status: %d\n", interval, status);

	if(gotHangup) {
	    gotHangup = false;
	    DebugPrintf(2, 500, "gotHangup is set, flushing messages and reopening files\n");
	    FlushMessages();
	    ReopenFiles();
	}
	if(gotAlarm) {
	    gotAlarm = false;
	    DebugPrintf(2, 500, "gotAlarm is set, flushing messages and syncing files\n");
	    FlushMessages();
	    SyncFiles();
	}
	if(gotUser1) {
	    gotUser1 = false;
	    DebugPrintf(2, 500, "gotUser1 is set, logging statistics\n");
	    LogStatistics();
	}
	if(gotUser2) {
	    gotUser2 = false;
	    DebugPrintf(2, 500, "gotUser1 is set, resetting statistics\n");
	    ResetStatistics();
	}
	
	if(status == 0) {
	    DebugPrintf(2, 500, "select() returned with out activity\n");
	    continue;
	}
	if(status < 0) {
	    if(errno != EINTR) ErrorMessage("select():");
	    DebugPrintf(2, 500, "select interrupted\n");
	    continue;
	}
	
	if(localFD > 0) {
	    if(FD_ISSET(localFD, readVector)) {
#if defined(STREAMS_LOGGING)
		GetStreamsMessage();
#endif
#if defined(FIFO_LOGGING)
		GetFIFOMessage();
#endif
#if defined(SOCKET_DGRAM_LOGGING)
		GetDgramMessage();
#endif
#if defined(SOCKET_STREAM_LOGGING)
		DebugPrintf(2, 10000, "Opening a new socket\n");
		OpenNewSocket(socketVector);
	    }
	    else {
		for(fd = 0; fd < FD_SETSIZE; fd++)
		    if(FD_ISSET(fd, socketVector) && FD_ISSET(fd, readVector)) {
			DebugPrintf(2, 1000, "Getting a socket message from fd: %d\n", fd);
			GetSocketMessage(fd, socketVector);
		    }
#endif
	    }
	}

	if(klogFD > 0 && FD_ISSET(klogFD, readVector)) {
	    DebugPrintf(2, 1000, "Getting a kernel message...\n");
	    GetKlogMessage();
	}
	if(inetFD > 0 && FD_ISSET(inetFD, readVector)) {
	    DebugPrintf(2, 1000, "Getting an inet message...\n");
	    GetInetMessage();
	}
    }
}

void Init(int argc, char *argv[])
{
    int opt;
    int debug = 0;
    char *p;
    FILE *file;
    
    progName = argv[0];
    DebugInit(11);
    time(&started);

    while(facilityNames[numberFacilities].value != -1) numberFacilities++;
    while(priorityNames[numberPriorities].value != -1) numberPriorities++;
    while(actionNames[numberActions].value != -1) numberActions++;

    inetReceiveFrom = InitCache();
    inetForwardFrom = InitCache();
    simpleHostnames = InitCache();
    stripDomains = InitCache();

    while((opt = getopt(argc, argv, "d:f:")) != EOF)
	switch(opt) {
	    case 'd':		/* set debugging level */
#if defined(DEBUG)
		DebugParse(optarg);
                debug++;
#else
		ErrorMessage("syslogd not compiled with debugging enabled");
#endif
		break;
	    case 'f':		/* configuration file */
		configFile = optarg;
		break;
	    case '?':
	    default:
		Usage();
		break;
	}
    if((argc -= optind) != 0)
	Usage();

    if(DebugTest(0,0)) {
	DebugPrintf(0,0,"Debugging facilities:\n");
	DebugPrintf(0,0,"   1: initialization and termination: %d\n", DebugLevel(1));
	DebugPrintf(0,0,"   2: main message loop:              %d\n", DebugLevel(2));
	DebugPrintf(0,0,"   3: parsing config file:            %d\n", DebugLevel(3));
	DebugPrintf(0,0,"   4: partial messages:               %d\n", DebugLevel(4));
	DebugPrintf(0,0,"   5: inet udp messages:              %d\n", DebugLevel(5));
	DebugPrintf(0,0,"   6: ACL processing:                 %d\n", DebugLevel(6));
	DebugPrintf(0,0,"   7: message processing              %d\n", DebugLevel(7));
	DebugPrintf(0,0,"   8: action processing               %d\n", DebugLevel(8));
	DebugPrintf(0,0,"   9: finding actions                 %d\n", DebugLevel(9));
	DebugPrintf(0,0,"  10: miscellaneous                   %d\n", DebugLevel(10));
	DebugPrintf(0,0,"  11: UNIX FIFO messages:             %d\n", DebugLevel(11));
    }

#if defined(WATT32)
#if defined(DEBUG)
    if (debug)
       dbug_init();
#endif
    sock_init();
#endif

    syslogPort = GetPortByName(syslogPortName);
    
    ReadConfigFile();

    /* done early so we can still log the error via stderr */
    file = fopen(pidFile, "w");
    if(file == NULL) ErrorMessage("could not open %s:", pidFile);

    if(hostName == NULL) {
	char tmp[MAXHOSTNAMELEN];
	
	gethostname(tmp, MAXHOSTNAMELEN);
	if((p = strchr(tmp, '.')) != NULL) *p = '\0';
	hostName = strdup(tmp);
    }

    if(daemonize && !DebugTest(0,0)) {
	int fd;
	
	if(fork()) exit(0);
	freopen("/dev/null", "r", stdin);
	freopen("/dev/null", "w", stdout);
	freopen("/dev/null", "w", stderr);
	fd = open("/dev/tty", O_RDWR|O_NOCTTY);
	if(fd >= 0) {
#if defined(HAVE_TIOCNOTTY)
	    ioctl(fd, TIOCNOTTY, NULL);
#endif
	    close(fd);
	}
    }

    upAndRunning = true;				/* Everything is ready to go */

    /* Save our PID */
    pid = getpid();
    if(file != NULL) {
	DebugPrintf(1, 10, "recording our pid %d in %s\n", pid, pidFile);
	fprintf(file, "%d\n", pid);
	fclose(file);
    }

    signal(SIGHUP,  Signal);
    signal(SIGALRM, Signal);
    signal(SIGTERM, Signal);
    signal(SIGUSR1, Signal);
    signal(SIGUSR2, Signal);
#if defined(WATT32)
    signal(SIGINT, Signal);
#endif
    alarm(markInterval);
    
    /* Not really an error, unless you really dislike this syslogd :) */
    InfoMessage(Copyright);
    InfoMessage("%s up and running", Version);
}

void ReadConfigFile(void)
{
    int yyparse(void);
    extern FILE *yyin;

    DebugPrintf(3, 10, "Starting to parse %s\n", configFile);
    if((yyin = fopen(configFile, "r")) == NULL)
	FatalError("cannot open %s:", configFile);
    if(yyparse() != 0)
	FatalError("difficulties in parsing %s", configFile);
    fclose(yyin);
    DebugPrintf(2, 10, "Done parsing %s\n", configFile);
}

void DoMessage(struct message *m)
{
    int facpri = 0;
    char *p = m->message;
    char program[MAXPROGNAME + 1];
    int i = 0;
    int j = 0;
    char c;
    char *timestamp = NULL;
    static char buffer[MAXMESSAGE + 1];

    time(&now);
    totalBytesReceived += m->length;
    totalMessagesReceived++;
    if(m->host == NULL || *m->host == '\0')
	m->host = hostName;
    /* Trim off any trailing white space and nulls */
    m->message[m->length] = '\0';
    while(m->length >= 0 && (m->message[m->length] == '\0' || isspace(m->message[m->length]))) m->length--;
    if(m->length <= 0) return;							/* Empty message, ignore */
    m->length++;
    m->message[m->length] = '\0';
    DebugPrintf(7, 100, "DoMessage called\n   Message: %s\n   Length: %d\n   Host: %s\n   Flags: %d\n",
		m->message, m->length, m->host, m->flags);

    /* get the facility & priority off the head of the message */
    if(*p == '<') {
	while(isdigit(*++p))
	    facpri = facpri * 10 + (*p - '0');
	if(*p == '>') p++;
	else DebugPrintf(7,0, "Priority not terminated with a '>'\n");
    }
    else facpri = LOG_USER|LOG_NOTICE;
    if(m->flags & FLAG_SET_FACPRI) facpri = LOG_MAKEPRI(m->facility,m->priority);
    m->length -= (p - m->message);
    m->facility = LOG_FAC(facpri);
    m->priority = LOG_PRI(facpri);
    
    if(ValidTimestamp(p)) {
	timestamp = p;
	timestamp[15] = '\0';
	p += 16;
	m->length -= 16;
    }
    else m->flags |= FLAG_ADD_DATE;
	    
    if(m->flags & FLAG_ADD_DATE) {
	timestamp = ctime(&now) + 4;		/* skip over the day */
	timestamp[15] = '\0';			/* We don't want the timezone or year */
    }
    m->timestamp = timestamp;

    /*
    ** Copy the message into a temporary buffer, converting all
    ** whitespace to a plain space, and any non-printable characters
    ** to ^c strings
    */
    while((c = *p++ & 0x7f) != '\0' && i < sizeof(buffer)) {
	if(isspace(c)) buffer[i] = ' ';
	else if(iscntrl(c)) {
	    buffer[i++] = '^';
	    buffer[i] = c ^ 0100;
	}
	else buffer[i] = c;
	i++;
    }
    buffer[i] = '\0';
    m->message = buffer;

    /* Copy the program name, either program[pid]: or program: */
    for(j = 0; j < i && j < sizeof(program) && buffer[j] != ':' && buffer[j] != '[' && !isspace(buffer[j]); j++)
	program[j] = buffer[j];
    program[j] = '\0';
    m->program = program;

    DebugPrintf(7, 10, "Message: %s\n", m->message);
    DebugPrintf(7, 10, "  Timestamp: %s\n", m->timestamp);
    DebugPrintf(7, 10, "  Facility/Priority: %s.%s\n", facilityNames[m->facility].name, priorityNames[m->priority].name);
    DebugPrintf(7, 10, "  Program: %s\n", m->program);
    DebugPrintf(7, 10, "  Hostname: %s\n", m->host);

    FindActions(m);
}

void FindActions(struct message *m)
{
    struct facility *fac;
    
    DebugPrintf(9, 10, "Message facility: %s   program: %s   priority: %s\n",
		facilityNames[m->facility].name, m->program, priorityNames[m->priority].name);
    DebugPrintf(9, 10, "%s %s %s\n", m->timestamp, m->host, m->message);
    DebugPrintf(9, 20, "Looking for facilities matching %s\n", facilityNames[m->facility].name);
    for(fac = facilities; fac != NULL; fac = fac->next) {
	if(fac->bitmap & facilityNames[m->facility].bitValue) {
	    struct program *prog;
	    DebugPrintf(9, 25, "  Matched bitmap: %x & %x\n", fac->bitmap, facilityNames[m->facility].bitValue);
	    DebugPrintf(9, 30, "  Looking for programs matching '%s'\n", m->program);
	    for(prog = fac->programs; prog != NULL; prog = prog->next) {
		struct programNames *name;
		for(name = prog->programNames; name != NULL; name = name->next) {
		    if(name->name == NULL || streq(name->name, m->program)) {
			struct priority *pri;
			DebugPrintf(9, 35, "    Matched program '%s'\n", name->name == NULL ? "<NULL>" : name->name);
			DebugPrintf(9, 40, "    Looking for priorities matching %s\n", priorityNames[m->priority].name);
			for(pri = prog->priorities; pri != NULL; pri = pri->next) {
			    switch(pri->comparison) {
				/*
				** In the UNIX tradition, more important is numbered lower.
				** (e.g. info == 6  warning == 4) so the comparisions are switched
				** so them make "logical" sense (ge is <= and le is >=) so "warning"
				** will be "greater" than "info".
				*/
				case le:
				    if(m->priority >= pri->priority) {
					DebugPrintf(9, 45, "      Matched priority: le %s\n",
						    priorityNames[pri->priority]);
					DoActions(pri->actions, m);
					if(pri->actions->hasBreak) goto sawBreak;
				    }
				    break;
				case ge:
				    if(m->priority <= pri->priority) {
					DebugPrintf(9, 45, "      Matched priority: ge %s\n",
						    priorityNames[pri->priority]);
					DoActions(pri->actions, m);
					if(pri->actions->hasBreak) goto sawBreak;
				    }
				    break;
				case eq:
				    if(m->priority == pri->priority) {
					DebugPrintf(9, 45, "      Matched priority: eq %s\n",
						    priorityNames[pri->priority]);
					DoActions(pri->actions, m);
					if(pri->actions->hasBreak) goto sawBreak;
				    }
				    break;
				case ne:
				    if(m->priority != pri->priority) {
					DebugPrintf(9, 45, "      Matched priority: ne %s\n",
						    priorityNames[pri->priority]);
					DoActions(pri->actions, m);
					if(pri->actions->hasBreak) goto sawBreak;
				    }
				    break;
			    }
			}
		    }
		}
	    }
	}
    sawBreak:
    }
}

void DoActions(struct actions *actions, struct message *m)
{
    static char buffer[MAXMESSAGE + 1];
    struct message old;
    time_t now;
    bool same = (streq(actions->previousMessage, m->message) && streq(actions->previousHost, m->host));

    DebugPrintf(8, 10, "Doing actions for message: %s\n", m->message);
    time(&now);

    if(same) actions->duplicateCount++;
    DebugPrintf(8, 500, "same: %d   duplicateCount: %d   time since: %d\n",
		same, actions->duplicateCount, now - actions->timeLogged);

    if(actions->duplicateCount > maxDuplicates) {
	/* If this message is different or enough time has elapsed, log the duplicate message */
	if(!same || now > actions->timeLogged + repeatInterval) {
	    actions->timeLogged = now;
	    snprintf(buffer, sizeof(buffer), "[message repeated %d times] %s",
		     actions->duplicateCount, actions->previousMessage);
	    old.message = buffer;
	    old.host = actions->previousHost;
	    old.program = actions->previousProgram;
	    old.timestamp = ctime(&now) + 4;		/* skip over the day */
	    old.timestamp[15] = '\0';			/* We don't want the timezone or year */
	    old.length = strlen(buffer);
	    old.facility = actions->previousFacility;
	    old.priority = actions->previousPriority;
	    old.forwardOK = actions->forwardOK;
	    old.flags = actions->flags|FLAG_ADD_DATE;
	    SelectActions(actions->actions, &old);
	}
	else DebugPrintf(8, 500, "[message repeated %d times] %s\n",
			 actions->duplicateCount, actions->previousMessage);
    }

    if(!same || actions->duplicateCount <= maxDuplicates) {
	actions->timeLogged = now;
	SelectActions(actions->actions, m);
    }
    
    if(!same) {
	/* Messages aren't the same, save this one */
	strncpy(actions->previousMessage, m->message, sizeof(actions->previousMessage));
	strncpy(actions->previousHost, m->host, sizeof(actions->previousHost));
	strncpy(actions->previousProgram, m->program, sizeof(actions->previousProgram));
	actions->previousFacility = m->facility;
	actions->previousPriority = m->priority;
	actions->forwardOK = m->forwardOK;
	actions->flags = m->flags;
	actions->duplicateCount = 0;
    }
}

void SelectActions(struct action *actions, struct message *m)
{
    struct action *p;

    DebugPrintf(8, 10, "Selecting actions for message: %s\n", m->message);
    for(p = actions; p != NULL; p = p->next) {
	switch(p->type) {
	    case actionNone:
		DebugPrintf(8, 10, "Action type: none\n");
		break;
	    case actionFile:
		DebugPrintf(8, 10, "Action type: file %s\n", p->dest);
		DoFileAction(p, m);
		break;
	    case actionFIFO:
		DebugPrintf(8, 10, "Action type: FIFO %s\n", p->dest);
		DoFileAction(p, m);
		break;
	    case actionForward:
		DebugPrintf(8, 10, "Action type: forward %s\n", p->dest);
		DoForwardAction(p, m);
		break;
	    case actionBreak:
		DebugPrintf(8, 10, "Action type: break\n");
		return;
		break;
	}
    }
}

void DoFileAction(struct action *action, struct message *m)
{
    struct iovec iov[8];
    int i = 0;
    char buffer[20];			/* Must be bigenough to fit "[facility.priority]" */
    
    if(action->action.file < 0) return;
    if(action->options.logWithPriority == numeric ||
       (action->options.logWithPriority == unset && logWithPriority == numeric)) {
	snprintf(buffer, sizeof(buffer), "<%d> ", LOG_MAKEPRI(m->facility, m->priority));
	iov[i].iov_base = buffer;
	iov[i].iov_len = strlen(buffer);
	i++;
    }
    iov[i].iov_base = m->timestamp;
    iov[i].iov_len = strlen(m->timestamp);
    i++;
    iov[i].iov_base = " ";
    iov[i].iov_len = 1;
    i++;
    iov[i].iov_base = m->host;
    iov[i].iov_len = strlen(m->host);
    i++;
    if(action->options.logWithPriority == symbolic ||
       (action->options.logWithPriority == unset && logWithPriority == symbolic)) {
	snprintf(buffer, sizeof(buffer), "[%s.%s]",
		 facilityNames[m->facility].name, priorityNames[m->priority].name);
	iov[i].iov_base = buffer;
	iov[i].iov_len = strlen(buffer);
	i++;
    }
    iov[i].iov_base = " ";
    iov[i].iov_len = 1;
    i++;
    iov[i].iov_base = m->message;
    iov[i].iov_len = strlen(m->message);
    i++;
    iov[i].iov_base = "\n";
    iov[i].iov_len = 1;
    i++;
    if(writev(action->action.file, iov, i) < 0) {
	ErrorMessage("Problem writing message to %s:", action->dest);
	DebugPrintf(8, 0, "Problem writing message to %s", action->dest);
	action->action.file = -1;
    }
    else if(action->options.syncEachMessage)
	fsync(action->action.file);
}

void ReopenFiles(void)
{
    struct facility *facility;
    struct program *program;
    struct priority *priority;
    struct action *action;

    DebugPrintf(10, 10, "Closing and reopening files\n");
    
    for(facility = facilities; facility != NULL; facility = facility->next)
	for(program = facility->programs; program != NULL; program = program->next)
	    for(priority = program->priorities; priority != NULL; priority = priority->next)
		for(action = priority->actions->actions; action != NULL; action = action->next)
		    switch(action->type) {
			case actionNone: break;
			case actionBreak: break;
			case actionForward: break;
			case actionFile:
			    DebugPrintf(10, 100, "closing and reopening file %s\n", action->dest);
			    close(action->action.file);
			    action->action.file = open(action->dest, O_WRONLY|O_APPEND|O_CREAT|O_NOCTTY, 0644);
			    if(action->action.file < 0)
				ErrorMessage("could not reopen file %s:", action->dest);
			    break;
			case actionFIFO:
			    DebugPrintf(10, 100, "closing and reopening FIFO %s\n", action->dest);
			    close(action->action.file);
			    action->action.file = open(action->dest, O_RDWR);
			    if(action->action.file < 0)
				ErrorMessage("could not reopen FIFO %s:", action->dest);
			    break;
		    }
}

void SyncFiles(void)
{
    struct facility *facility;
    struct program *program;
    struct priority *priority;
    struct action *action;

    DebugPrintf(10, 10, "Syncing files\n");
    
    for(facility = facilities; facility != NULL; facility = facility->next)
	for(program = facility->programs; program != NULL; program = program->next)
	    for(priority = program->priorities; priority != NULL; priority = priority->next)
		for(action = priority->actions->actions; action != NULL; action = action->next)
		    switch(action->type) {
			case actionNone: break;
			case actionBreak: break;
			case actionForward: break;
			case actionFIFO:
			    fsync(action->action.file);
			    DebugPrintf(10, 100, "synced FIFO %s\n", action->dest);
			    break;
			case actionFile:
			    fsync(action->action.file);
			    DebugPrintf(10, 100, "synced file %s\n", action->dest);
			    break;
		    }
}

void FlushMessages(void)
{
    char buffer[MAXMESSAGE + 1];
    struct message m;
    struct facility *facility;
    struct program *program;
    struct priority *priority;
    struct actions *actions;
    time_t now;

    DebugPrintf(10, 10, "Flushing duplicate messages\n");
    time(&now);

    for(facility = facilities; facility != NULL; facility = facility->next)
	for(program = facility->programs; program != NULL; program = program->next)
	    for(priority = program->priorities; priority != NULL; priority = priority->next) {
		actions = priority->actions;
		if(actions->duplicateCount > maxDuplicates && now > actions->timeLogged + repeatInterval) {
		    actions->timeLogged = now;
		    snprintf(buffer, sizeof(buffer), "[message repeated %d times] %s",
			     actions->duplicateCount, actions->previousMessage);
		    DebugPrintf(10, 500, "%s\n", buffer);
		    m.message = buffer;
		    m.host = actions->previousHost;
		    m.program = actions->previousProgram;
		    m.length = strlen(m.message);
		    m.timestamp = ctime(&now) + 4;		/* skip over the day */
		    m.timestamp[15] = '\0';			/* We don't want the timezone or year */
		    m.flags = 0;
		    m.facility = actions->previousFacility;
		    m.priority = actions->previousPriority;
		    m.forwardOK = actions->forwardOK;
		    m.flags = actions->flags;
		    SelectActions(actions->actions, &m);
		}
	    }
}
