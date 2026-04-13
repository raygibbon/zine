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

/* $Id: syslogd.h,v 1.45 1998/12/03 17:32:31 rand Exp $ */

#include "config.h"
#include "debug.h"
#include "version.h"

/* Need to get the rest of the syslog definitions */
#ifdef WATT32
#include <sys/syslog.h>
#else
#include <syslog.h>
#endif

/* Include files needed for the type definitions below */
#if defined(HAVE_SYS_SIGEVENT_H)
#include <sys/sigevent.h>
#endif
#include <time.h>
#include <netdb.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <netinet/in.h>

/* Believe it or now Solaris 2.5 has __snprintf() and __vsnprintf() */
#if defined(HAVE___SNPRINTF)
#define snprintf __snprintf
#define vsnprintf __vsnprintf
#endif

#if defined(__DJGPP__)
  #include <stdarg.h>
  int snprintf (char *str, size_t count, const char *fmt, ...);
  int vsnprintf(char *str, size_t count, const char *fmt, va_list args);
#endif

#define INTERNAL_NOPRI 12	/* No priority, CANNOT FIT IN 3 BITS! */
#define MAXMESSAGE 1024		/* size of the largest message */
#define MAXPROGNAME 64		/* size of the longest program name */
#define HASHSIZE 997		/* size of the hash tables (should be prime) */
#define MAX_HASHSIZE 2048	/* how large can a hash table grow */

#define LOG_EMERG	0	/* system is unusable */
#define LOG_ALERT	1	/* action must be taken immediately */
#define LOG_CRIT	2	/* critical conditions */
#define LOG_ERR		3	/* error conditions */
#define LOG_WARNING	4	/* warning conditions */
#define LOG_NOTICE	5	/* normal but significant condition */
#define LOG_INFO	6	/* informational */
#define LOG_DEBUG	7	/* debug-level messages */

#define LOG_KERN	(0<<3)	/* kernel messages */
#define LOG_USER	(1<<3)	/* random user-level messages */
#define LOG_MAIL	(2<<3)	/* mail system */
#define LOG_DAEMON	(3<<3)	/* system daemons */
#define LOG_AUTH	(4<<3)	/* security/authorization messages */
#define LOG_SYSLOG	(5<<3)	/* messages generated internally by syslogd */
#define LOG_LPR		(6<<3)	/* line printer subsystem */
#define LOG_NEWS	(7<<3)	/* network news subsystem */
#define LOG_UUCP	(8<<3)	/* UUCP subsystem */
#define LOG_BSDCRON	(9<<3)	/* clock daemon for BSD systems  Sigh. */
#define LOG_AUTHPRIV	(10<<3)	/* security/authorization messages (private) */
#define LOG_FTP		(11<<3)	/* file transfer protocol */
#define LOG_NTP		(12<<3)	/* network time protocol */
#define LOG_SYSVCRON	(15<<3)	/* clock daemon for System V systems  Sigh. */
				/* other codes through 15 reserved for system use */
#define LOG_LOCAL0	(16<<3)	/* reserved for local use */
#define LOG_LOCAL1	(17<<3)	/* reserved for local use */
#define LOG_LOCAL2	(18<<3)	/* reserved for local use */
#define LOG_LOCAL3	(19<<3)	/* reserved for local use */
#define LOG_LOCAL4	(20<<3)	/* reserved for local use */
#define LOG_LOCAL5	(21<<3)	/* reserved for local use */
#define LOG_LOCAL6	(22<<3)	/* reserved for local use */
#define LOG_LOCAL7	(23<<3)	/* reserved for local use */

#define LOG_FAC(p)		(((p) & LOG_FACMASK) >> 3)
#define LOG_PRI(p)		((p) & LOG_PRIMASK)
/* Recent BSD distributions are not shifting the facility in LOG_MAKEPRI */
#if defined(LOG_MAKEPRI)
#undef LOG_MAKEPRI
#endif
#define LOG_MAKEPRI(fac, pri)	(((fac) << 3) | (pri))

/* Bitmaps of all the facilities for fast lookups */
#define BIT_KERN	0x00000001
#define BIT_USER	0x00000002
#define BIT_MAIL	0x00000004
#define BIT_DAEMON	0x00000008
#define BIT_AUTH	0x00000010
#define BIT_SYSLOG	0x00000020
#define BIT_LPR		0x00000040
#define BIT_NEWS	0x00000080
#define BIT_UUCP	0x00000100
#define BIT_BSDCRON	0x00000200
#define BIT_AUTHPRIV	0x00000400
#define BIT_FTP		0x00000800
#define BIT_NTP		0x00001000
#define UNUSED13	0x00002000
#define UNUSED14	0x00004000
#define BIT_SYSVCRON	0x00008000
#define BIT_LOCAL0	0x00010000
#define BIT_LOCAL1	0x00020000
#define BIT_LOCAL2	0x00040000
#define BIT_LOCAL3	0x00080000
#define BIT_LOCAL4	0x00100000
#define BIT_LOCAL5	0x00200000
#define BIT_LOCAL6	0x00400000
#define BIT_LOCAL7	0x00800000

#define FLAG_ADD_DATE		0x01
#define FLAG_RE_DATE		0x02
#define FLAG_SET_FACPRI		0x04

#if !defined(PATH_LOG)
#define PATH_LOG		"/dev/log"
#endif
#if !defined(PATH_KLOG)
#define PATH_KLOG		"/dev/klog"
#endif
#if !defined(PATH_PID)
#define PATH_PID		"@PIDDIR@/syslogd.pid"
#endif
#if !defined(PATH_SYSLOG_CONF)
#define PATH_SYSLOG_CONF	"/etc/syslogd.conf"
#endif

#define streq(s1,s2)	(strcmp(s1,s2) == 0)
#define strneq(s1,s2,n)	(strncmp(s1,s2,n) == 0)
#define TIMEVAL2FLOAT(time)	((float) time.tv_sec + ((float) time.tv_usec / 1000000.0))

typedef enum { false = 0, true } bool;
enum actionTypes { actionNone = 0, actionFile, actionFIFO, actionForward, actionBreak };
enum comparisons { le, ge, ne, eq };
enum logWithPriorities { none, symbolic, numeric, unset };
enum aclTypes { host, domain, network };

struct hashTable {
    struct hash **cache;
    struct aclList *acl;
    int size;
};

struct hash {
    struct in_addr address;
    char *name;
    bool status;
    u_long hits;
    struct hash *next;
};

struct code {
    char *name;
    int value;
    int bitValue;
};

struct facility {
    int bitmap;
    struct program *programs;
    struct facility *next;
};

struct program {
    struct programNames *programNames;
    struct priority *priorities;
    struct program *next;
};

struct programNames {
    char *name;
    struct programNames *next;
};

struct priority {
    enum comparisons comparison;
    int priority;
    struct actions *actions;
    struct priority *next;
};

struct actionOptions {
    unsigned short port;
    bool forwardWithHostname;
    bool syncEachMessage;
    enum logWithPriorities logWithPriority;
};

struct actions {
    char previousMessage[MAXMESSAGE + 1];
    char previousHost[MAXHOSTNAMELEN + 1];
    char previousProgram[MAXPROGNAME + 1];
    int previousFacility;
    int previousPriority;
    int duplicateCount;
    int flags;
    time_t timeLogged;
    bool forwardOK;
    bool hasBreak;
    struct action *actions;
};

struct action {
    enum actionTypes type;
    char *dest;
    union {
	int file;
	struct sockaddr_in forward;
    } action;
    struct actionOptions options;
    struct action *next;
};

struct aclNetwork {
    struct in_addr network;
    int bits;
};

struct aclEntry {
    enum aclTypes type;
    union {
	char *host;
	char *domain;
	struct aclNetwork network;
    } acl;
};

struct aclList {
    struct aclEntry *acl;
    struct aclList *next;
};

struct acls {
    char *name;
    struct aclList *list;
    struct acls *next;
};

struct message {
    char *message;
    char *host;
    char *program;
    char *timestamp;
    int length;
    int flags;
    int facility;
    int priority;
    bool forwardOK;
};

#include "proto.h"

/* Global variables */

extern bool daemonize;
extern bool gotAlarm;
extern bool gotHangup;
extern bool gotUser1;
extern bool gotUser2;
extern bool inetForwarding;
extern bool inetReceive;
extern bool logUnwantedHosts;
extern bool syncEachMessage;
extern bool upAndRunning;
extern char *configFile;
extern char *directory;
extern char *klogDevice;
extern char *logDevice;
extern char *pidFile;
extern char *progName;
extern char *hostName;
extern char *syslogPortName;
extern enum logWithPriorities logWithPriority;
extern int hashTableSize;
extern int hashTableMaxSize;
extern int lineNumber;
extern int logDeviceMode;
extern int logFD;
extern int logFileMode;
extern int markInterval;
extern int maxDuplicates;
extern int numberActions;
extern int numberFacilities;
extern int numberPriorities;
extern int pid;
extern int repeatInterval;
extern int socketBacklog;
extern time_t now;
extern time_t started;
extern short unsigned syslogPort;
extern struct acls *acls;
extern struct actionOptions actionOptions;
extern struct code actionNames[];
extern struct code facilityNames[];
extern struct code priorityNames[];
extern struct facility *facilities;
extern struct hashTable *inetForwardFrom;
extern struct hashTable *inetReceiveFrom;
extern struct hashTable *simpleHostnames;
extern struct hashTable *stripDomains;
extern unsigned long int inetBytesReceived;
extern unsigned long int inetMessagesReceived;
extern unsigned long int localBytesReceived;
extern unsigned long int localMessagesReceived;
extern unsigned long int totalBytesReceived;
extern unsigned long int totalMessagesReceived;
