#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <time.h>		/* timeval, time_t */
#include <setjmp.h>		/* jmp_buf et al */
#include <sys/types.h>
#include <sys/socket.h>		/* basics, SO_ and AF_ defs, sockaddr, ... */
#include <sys/stat.h>
#include <sys/time.h>		/* gettimeofday */
#include <netinet/in.h>		/* sockaddr_in, htons, in_addr */
#ifdef __OpenBSD__
#include <netinet/in_systm.h>
#endif
#include <arpa/inet.h>		/* inet_ntoa */
#include <arpa/nameser.h>       /* MAXDNAME */
#include <netinet/ip.h>         /* IPOPT_LSRR, header stuff */
#include <netdb.h>		/* hostent, gethostby*, getservby* */
#include <string.h>		/* strcpy, strchr, yadda yadda */
#include <errno.h>
#include <signal.h>
#include <fcntl.h>		/* O_WRONLY et al */
#include <resolv.h>
#include <syslog.h>
#include "secrete/incl/secrete.h"

#ifdef __MSDOS__
#include <tcp.h>
#define select select_s
#define read   read_s
#define write  write_s
#endif

#define VERSION	"0.0.8"
#define SRAND 	srand
#define RAND 	rand

#define SA  struct sockaddr	/* socket overgeneralization braindeath */
#define SAI struct sockaddr_in	/* ... whoever came up with this model */
#define IA  struct in_addr	/* ... should be taken out and shot, */
				/* ... not that TLI is any better.  sigh.. */
#define SLEAZE_PORT 31337	/* for UDP-scan RTT trick, change if ya want */
#define USHORT unsigned short	/* use these for options an' stuff */
#define BIGSIZ 8192		/* big buffers */
#define LOGBUF 1024		/* log fmt buffer */
#define E_OPTS 256		/* define max amount of params passed to '-e' */

#ifndef INADDR_NONE
#define INADDR_NONE 0xffffffff
#endif

#ifdef SYSLOG
#define log_syslog 1
#else
#define log_syslog 0
#endif

/* This implementation is unsecure. */
/* Use it with formatting arguments in the code */
#ifdef DEBUG
#define DBG(fmt, msg...)						\
	do { 								\
		if (log_syslog){					\
			syslog(LOG_INFO, fmt, ## msg);			\
		} else {						\
			printf(fmt, ## msg);				\
		}							\
	} while (0)
#else   /* NO DEBUGGING at ALL */
#define DBG(fmt, msg, ...)
#endif

struct host_poop {
	char name[MAXHOSTNAMELEN];	/* dns name */
	char addrs[8][24];		/* ascii-format IP addresses */
	struct in_addr iaddrs[8];	/* real addresses: in_addr.s_addr */
};
#define HINF struct host_poop

struct port_poop {
	char name[64];		/* name in /etc/services */
	char anum[8];		/* ascii-format number */
	USHORT num;		/* real host-order number */
};
#define PINF struct port_poop

/* stolen almost wholesale from bsd herror.c */
static char *h_errs[] = {
	"Error 0",		/* but we *don't* use this */
	"Unknown host",		/* 1 HOST_NOT_FOUND */
	"Host name lookup failure",	/* 2 TRY_AGAIN */
	"Unknown server error",	/* 3 NO_RECOVERY */
	"No address associated with name",	/* 4 NO_ADDRESS */
};

/* globals: */
int			krypton;
char			temppig[BIGSIZ];
jmp_buf              	jbuf;	/* timer crud */
int jval              	= 0;	/* timer crud */
int netfd             	= -1;
int ofd               	= 0;	/* hexdump output fd */
static char unknown[] 	= "(UNKNOWN)";
static char p_tcp[]   	= "tcp";/* for getservby* */
static char p_udp[]   	= "udp";
int gatesidx            = 0;	/* LSRR hop count */
int gatesptr            = 4;	/* initial LSRR pointer, settable */
USHORT Single           = 1;	/* zero if scanning */
unsigned int insaved    = 0;	/* stdin-buffer size for multi-mode */
unsigned int wrote_out  = 0;	/* total stdout bytes */
unsigned int wrote_net  = 0;	/* total net bytes */
static char hexnibs[20] = "0123456789abcdef  ";

/* will malloc up the following globals: */
struct timeval *timer1 = NULL;
struct timeval *timer2 = NULL;
struct timeval *then   = NULL;
SAI            *lclend = NULL;	/* sockaddr_in structs */
SAI            *remend = NULL;
HINF           **gates = NULL;	/* LSRR hop hostpoop */
char           *optbuf = NULL;	/* LSRR or sockopts */
char           *bigbuf_in;	/* data buffers */
char           *bigbuf_net;
fd_set         *ding1;		/* for select loop */
fd_set         *ding2;
PINF         *portpoop = NULL;	/* for getportpoop / getservby* */
unsigned char   *stage = NULL;	/* hexdump line buffer */

/* global cmd flags: */
USHORT 		o_alla 		= 0;
USHORT 		o_allowbroad 	= 0;
USHORT 		o_listen 	= 0;
USHORT 		o_nflag 	= 0;
USHORT 		o_wfile 	= 0;
USHORT 		o_random 	= 0;
USHORT 		o_udpmode 	= 0;
USHORT 		o_verbose 	= 0;
USHORT 		o_zero 		= 0;
USHORT 		o_tn 		= 0;
unsigned int 	o_wait 		= 0;
unsigned int 	o_interval 	= 0;

/* blowfish session */
bfs bfsession;

/* function prototyping */
void helpme(void);
static void catch(int);
static void tmtravel(int);
void shout(unsigned int, const char *fmt, ...);
char *Hmalloc(unsigned int);
HINF *gethostpoop(char *, USHORT);
USHORT getportpoop(char *, unsigned int);
USHORT nextport(char *);
void loadports(char *, USHORT, USHORT);
void doexec(int);
int doconnect(IA *, USHORT, IA *, USHORT);
int dolisten(IA *, USHORT, IA *, USHORT);
int udptest(int, IA *);
void oprint(int, char *, int);
void atelnet(unsigned char *, unsigned int);
int readwrite(int);
void set_signal_handler(int, void *);
char *iostats(void);
