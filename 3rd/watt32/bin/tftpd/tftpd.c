/* 
 * Copyright (c) 1994, 1995, 1997
 *      The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Trivial file transfer protocol server.
 *
 * This version includes many modifications by Jim Guyton
 * <guyton@rand-unix>.
 *
 * Rewritten to handle multiple concurrent clients in a single process
 * by Craig Leres, leres@ee.lbl.gov.
 */

#include "tftp.h"

/* Everything we need to keep track of a client
 */
struct client {
  struct sockaddr_in sin;	/* address and port of client */
  time_t wakeup;		/* time to wakeup */
  int    retries;               /* retries left until we give up */
  struct tftphdr *tp;		/* pointer to received packet */
  struct tftphdr *dp;		/* pointer to output packet */
  int    tpcc;                  /* size of data in tp */
  int    dpcc;                  /* size of data in dp */
  int    proc;                  /* client proc */
  int    state;                 /* client state */
  FILE  *f;                     /* open file pointer */
  int    block;                 /* block in file */
  int    convert;               /* convert to ascii if true */
  struct ra ra;			/* read ahead buffer stuff */
};

/* Client states
 */
#define CP_FREE		0
#define CP_TFTP		1	/* ready to start a session */
#define CP_SENDFILE	2	/* ready to send a block */
#define CP_RECVFILE	3	/* ready to recv a block */
#define CP_DONE		4	/* all done */

#define ST_INIT		1	/* initialize */
#define ST_SEND		2	/* send a block */
#define ST_WAITACK	3	/* waiting for a block to be ack'ed */
#define ST_LASTACK	4	/* sent last block, wait for last ack */

/* Internal statistics
 */
struct stats {
  u_int32_t retrans;		/* number of retransmissions */
  u_int32_t timeout;		/* number of timed out clients */
  u_int32_t badop;		/* number of was expecting ACK */
  u_int32_t errorop;		/* received ERROR from client */
  u_int32_t badfilename;	/* filename wasn't parsable */
  u_int32_t badfilemode;	/* file mode wasn't parsable */
  u_int32_t wrongblock;		/* client ACKed wrong block */
  u_int32_t fileioerror;	/* errors reading the file */
  u_int32_t sent;		/* number of packets sent */
  u_int32_t maxclients;		/* maximum concurrent clients */
};

/* 
 * Null-terminated directory prefix list for absolute pathname requests and
 * search list for relative pathname requests.
 */
struct dirlist {
       char *name;
       int   len;
     };

#define WAIT_TIMEOUT 5		/* seconds between retransmits */
#define WAIT_RETRIES 5		/* retransmits before giving up */

#define MORECLIENTS 10		/* add this many when we need more */

/*
 * Forwards
 */
static char *errtomsg (int);
static struct client *findclient (struct sockaddr_in *);
static void freeclient (struct client *);
static void nak (struct client *, int);
static struct client *newclient (void);
static void process (struct sockaddr_in *, struct tftphdr *, int);
static void recvfile (struct client *);
static void runclient (struct client *);
static void runtimer (void);
static void sendfile (struct client *);
static void tftp (struct client *);
static int validate_access (struct client *, char *, int);
static __dead void finish (int) __attribute__ ((volatile));

void die (int);
void logstats (int);

/* Globals */
char *errstr;			/* detail for DENIED syslog */
int   debug;

/* Locals */
static int suppress_naks;
static int s;			/* tftp socket */
static char *bootptab = "/etc/bootptab";
static time_t now;		/* updated after packet reception */

static int dostats;		/* need to syslog statistics */
static int candostats;		/* ok to syslog statistics */
static struct stats stats;	/* tftpd statistics */

#define MAXDIRS	20
static struct dirlist dirs[MAXDIRS + 1];

static struct client **clientlist;
static int clientlistsize;	/* size of clientlist array */
static int clientlistcnt;	/* number in use */

int main (int argc, char **argv)
{
  struct servent    *sp;
  struct timeval    *tvp;
  struct sockaddr_in sin, from;
  struct timeval     tv;
  int    op, cc, fromlen;
  char  *cp;
  time_t last;
  fd_set fds, rfds;
  char   buf[PKTSIZE];

  openlog ("tftpd",0,LOG_AUTH);

  while ((op = getopt(argc,argv,"dnb:")) != EOF)
  {
    switch (op)
    {
      case 'd':
	   ++debug;
	   break;

      case 'n':
	   ++suppress_naks;
	   break;

      case 'b':
	   bootptab = optarg;
	   break;

      default:
	   log (LOG_WARNING, "ignoring unknown option -%c", op);
    }
  }

#ifdef __MSDOS__
  if (debug)
     dbug_init();
#endif

  if (optind < argc)
  {
    struct dirlist *dirp;

    /* Get list of directory prefixes. Skip relative pathnames.
     */
    for (dirp = dirs; optind < argc && dirp < &dirs[MAXDIRS]; optind++)
    {
      if (argv[optind][0] == '/')
      {
	dirp->name = argv[optind];
	dirp->len = strlen (dirp->name);
	dirp++;
      }
    }
  }
  else
  {
    /* Default directory list */
    dirs[0].name = "/tftpboot";
    dirs[0].len  = strlen (dirs[0].name);
  }

  /* Read the bootptab */
  cp = setbootptab (bootptab);
  if (cp)
     log (LOG_ERR, "setbootptab: %s", cp);

  /* Setup our socket */
  s = socket (AF_INET, SOCK_DGRAM, 0);
  if (s < 0)
  {
    log (LOG_ERR, "socket: %m");
    finish (1);
  }
  sp = getservbyname ("tftp", "udp");
  if (sp == NULL)
  {
    log (LOG_ERR, "tftp/udp: unknown service");
    finish (1);
  }
  memset (&sin, 0, sizeof (sin));
  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = INADDR_ANY;
  sin.sin_port = sp->s_port;
  if (bind (s, (struct sockaddr *) &sin, sizeof (sin)) < 0)
  {
    log (LOG_ERR, "bind: %m");
    finish (1);
  }

  /* Setup signal handler */
  signal (SIGINT, die);

  FD_ZERO (&fds);
  FD_SET (s, &fds);
  tv.tv_sec = WAIT_TIMEOUT;
  tv.tv_usec = 0;
  last = 0;

  for (;;)
  {
    rfds = fds;
    if (clientlistcnt > 0)
         tvp = &tv;
    else tvp = NULL;
    ++candostats;
    if (select_s (s + 1, &rfds, NULL, NULL, tvp) < 0)
    {
      /* Don't choke when we get ptraced */
      if (errno == EINTR)
         continue;
      log (LOG_ERR, "select: %m");
      finish (1);
    }
    candostats = 0;
    if (dostats)
      logstats (0);

    if (FD_ISSET (s, &rfds))
    {
      /* Process a packet */
      fromlen = sizeof (from);
      cc = recvfrom (s, buf, sizeof(buf), 0, (struct sockaddr*)&from, &fromlen);
      if (cc < 0)
      {
	log (LOG_ERR, "recvfrom: %m");
	continue;
      }

      /* Update now */
      now = time (NULL);

      /* Process this packet */
      process (&from, (struct tftphdr*)buf, cc);
    }
    else
      now = time (NULL);

    /* Run the timer list, no more than once a second */
    if (clientlistcnt > 0 && last != now)
    {
      runtimer();
      last = now;
    }
  }
  return (1);
}

/*
 * Process an incoming tftp packet
 */
static void process (struct sockaddr_in *fromp, struct tftphdr *tp, int cc)
{
  struct client *cl = findclient (fromp);   /* Look for old session */

  /* See if we need to reset him */
  if (cl && (tp->th_opcode == RRQ || tp->th_opcode == WRQ) &&
      cl->proc == CP_SENDFILE && cl->state == ST_WAITACK)
  {
    ++stats.badop;
    if (debug)
      log (LOG_DEBUG, "%s.%d resetting",
	   inet_ntoa (cl->sin.sin_addr), cl->sin.sin_port);
    freeclient (cl);
    cl = NULL;
    /* Fall through and handle like a new client */
  }

  if (cl == NULL)
  {
    cl = newclient();
    if (cl == NULL)
       return;
    cl->sin = *fromp;
    cl->proc = CP_TFTP;
    if (debug)
      log (LOG_DEBUG, "%s.%d starting",
	   inet_ntoa (cl->sin.sin_addr), cl->sin.sin_port);
  }
  cl->tp = tp;
  cl->tpcc = cc;

  /* Start up this client */
  runclient (cl);
}

/* 
 * Scan client list for guys that need to retransmit. Do it backwards
 * in case anybody gets freed.
 */
static void runtimer (void)
{
  struct client *cl, **clp;

  if (debug > 1)
     log (LOG_DEBUG, "runtimer (clientlistcnt %d)", clientlistcnt);

  for (clp = clientlist + clientlistcnt - 1; clp >= clientlist; --clp)
  {
    cl = *clp;
    cl->tp = NULL;
    cl->tpcc = 0;
    if (cl->wakeup != 0 && cl->wakeup <= now)
    {
      if (debug)
	log (LOG_DEBUG, "%s.%d wakeup",
	     inet_ntoa (cl->sin.sin_addr),
	     cl->sin.sin_port);
      runclient (cl);
    }
  }
}

/*
 * Execute a client proc
 */
static void runclient (struct client *cl)
{
  switch (cl->proc)
  {
    case CP_TFTP:
	 tftp (cl);
	 break;

    case CP_SENDFILE:
	 sendfile (cl);
	 break;

    case CP_RECVFILE:
	 recvfile (cl);
	 break;

    default:
	 log (LOG_ERR, "bad client proc %d", cl->proc);
	 finish (1);
  }
  if (cl->proc == CP_DONE)
    freeclient (cl);
}

/*
 * Handle initial connection protocol
 */
static void tftp (struct client *cl)
{
  struct tftphdr *tp;
  char  *mode, *cp, *cp2, *ep;
  char   file[PATH_MAX];
  int    ecode, isclient;

  tp = cl->tp;
  tp->th_opcode = ntohs ((u_short) tp->th_opcode);
  if (tp->th_opcode != RRQ && tp->th_opcode != WRQ)
  {
    /* XXX this probably can't happen */
    log (LOG_DEBUG, "%s.%d expected RRQ/WRQ",
	 inet_ntoa (cl->sin.sin_addr), cl->sin.sin_port);
    cl->proc = CP_DONE;
    return;
  }

  errstr = NULL;
  cp = (char *) &tp->th_block;
  ep = (char *) cl->tp + cl->tpcc;

  /* Extract filename */
  cp2 = file;
  while (cp < ep && *cp != '\0')
    *cp2++ = *cp++;
  if (cp >= ep)
  {
    ++stats.badfilename;
    nak (cl, EBADOP);
    return;
  }
  *cp2 = '\0';
  ++cp;

  /* Make sure mode is in packet */
  mode = cp;
  while (cp < ep && *cp != '\0')
    ++cp;
  if (cp >= ep)
  {
    ++stats.badfilemode;
    nak (cl, EBADOP);
    return;
  }

  if (!stricmp (mode,"netascii"))
    cl->convert = 1;
  else if (!stricmp(mode,"octet"))
    cl->convert = 0;
  else
  {
    ++stats.badfilemode;
    nak (cl, EBADOP);
    return;
  }
  isclient = goodclient (cl->sin.sin_addr.s_addr);
  if (!isclient)
  {
    errstr = "not tftp client";
    ecode = EACCESS;
  }
  else
    ecode = validate_access (cl, file, tp->th_opcode);
  if (ecode)
  {
    log (LOG_INFO, "%s %s \"%s\" DENIED (%s)",
	 inet_ntoa (cl->sin.sin_addr),
	 tp->th_opcode == WRQ ? "write" : "read",
         file, errstr ? errstr : errtomsg (ecode));

    /* Only nak if a valid client (and we support naks) */
    if (isclient && !suppress_naks)
      nak (cl, ecode);
    cl->proc = CP_DONE;
    return;
  }

  log (LOG_DEBUG, "%s-%s req for \"%s\" from %s",
       tp->th_opcode == WRQ ? "write" : "read",
       mode, file, inet_ntoa (cl->sin.sin_addr));

  cl->state = ST_INIT;
  cl->tp    = NULL;
  cl->tpcc  = 0;
  if (tp->th_opcode == WRQ)
  {
    cl->proc = CP_RECVFILE;
    recvfile (cl);
  }
  else
  {
    cl->proc = CP_SENDFILE;
    sendfile (cl);
  }
}

/* 
 * Validate file access. Since we have no uid or gid, for now require
 * file to exist and be publicly readable/writable.
 * If we were invoked with arguments from inetd then the file must also
 * be in one of the given directory prefixes. Note also, full path name
 * must be given as we have no login directory.
 */
static int validate_access (struct client *cl, char *file, int mode)
{
  struct dirlist *dirp;
  struct stat     stbuf;
  char   tfile[PATH_MAX];
  int    fd, err;

  /*
   * Prevent tricksters from getting around the directory restrictions
   */
  if (strstr(file, "/../"))
  {
    errstr = "\"/../\" in path";
    return (EACCESS);
  }
  if (dirs[0].name == NULL)
  {
    errstr = "empty dirlist";
    return (EACCESS);
  }
#ifndef ALLOW_WRITE
  if (mode != RRQ)
  {
    errstr = "write not allowed";
    return (EACCESS);
  }
#endif

  if (*file == '/')
  {
    /*
     * Allow the request if it's in one of the approved locations.
     */
    for (dirp = dirs; dirp->name; dirp++)
    {
      if ((!strncmp (file, dirp->name, dirp->len) &&
          file[dirp->len] == '/'))
	break;
    }
    /* No default access! */
    if (dirp->name == NULL)
    {
      errstr = "not on list";
      return (EACCESS);
    }
    if (stat (file, &stbuf) < 0)
    {
      errstr = strerror (errno);
      return (errno == ENOENT ? ENOTFOUND : EACCESS);
    }
    if ((stbuf.st_mode & S_IFMT) != S_IFREG)
    {
      errstr = "not a regular file";
      return (ENOTFOUND);
    }
    if (mode == RRQ)
    {
      if ((stbuf.st_mode & S_IROTH) == 0)
      {
	errstr = "not readable";
	return (EACCESS);
      }
    }
    else if ((stbuf.st_mode & S_IWOTH) == 0)
    {
      errstr = "not writable";
      return (EACCESS);
    }
  }
  else
  {
    /*
     * Relative file name: search the approved locations for it.
     * Don't allow write requests or ones that avoid directory
     * restrictions.
     */

    if (!strncmp (file, "../", 3))
    {
      errstr = "\"../\" in path";
      return (EACCESS);
    }

    /*
     * If the file exists in one of the directories and isn't
     * readable, continue looking. However, change the error code
     * to give an indication that the file exists.
     */
    err = ENOTFOUND;
    for (dirp = dirs; dirp->name; dirp++)
    {
      sprintf (tfile, "%s/%s", dirp->name, file);
      if (stat (tfile, &stbuf) == 0 &&
	  (stbuf.st_mode & S_IFMT) == S_IFREG)
      {
	if ((stbuf.st_mode & S_IROTH) != 0)
	{
	  break;
	}
	errstr = "not readable";
	err = EACCESS;
      }
    }
    if (dirp->name == NULL)
      return (err);
    strcpy (file, tfile);
  }

#ifdef ALLOW_WRITE
  fd = open (file, mode == RRQ ? O_RDONLY : O_WRONLY);
#else
  fd = open (file, O_RDONLY);
#endif
  if (fd < 0)
  {
    errstr = strerror (errno);
    return (errno + 100);
  }

#ifdef ALLOW_WRITE
  cl->f = fdopen (fd, mode == RRQ ? "r" : "w");
#else
  cl->f = fdopen (fd, "r");
#endif

  if (cl->f == NULL)
  {
    errstr = strerror (errno);
    return (errno + 100);
  }
  return (0);
}

/*
 * Send the requested file
 */
static void sendfile (struct client *cl)
{
  struct tftphdr *tp;
  struct tftphdr *dp;
  int    oerrno, opcode, block, readblock;

  readblock = 0;
  tp = cl->tp;
  if (tp)
  {
    /* Process an input packet */
    opcode = ntohs ((u_short) tp->th_opcode);

    /* Give up if the other side wants to */
    if (opcode == ERROR)
    {
      ++stats.errorop;
      log (LOG_INFO, "%s.%d ERROR",
	   inet_ntoa (cl->sin.sin_addr), cl->sin.sin_port);
      cl->proc = CP_DONE;
      return;
    }

    /* If we didn't get an ACK, blow him away */
    if (opcode != ACK)
    {
      /* XXX this probably can't happen */
      log (LOG_DEBUG, "%s.%d expected ACK",
	   inet_ntoa (cl->sin.sin_addr), cl->sin.sin_port);
      nak (cl, ERROR);
      return;
    }

    if (cl->state == ST_WAITACK || cl->state == ST_LASTACK)
    {
      /* If he didn't ack the right block, ignore packet */
      block = ntohs ((u_short) tp->th_block);
      if (block != cl->block)
      {
	++stats.wrongblock;
	return;
      }

      /* He ack'ed the last block we sent him */
      if (debug > 1)
	log (LOG_DEBUG, "%s.%d < ack %d",
	     inet_ntoa (cl->sin.sin_addr),
	     cl->sin.sin_port,
	     cl->block);

      /* Check if we're done */
      if (cl->state == ST_LASTACK)
      {
	cl->proc = CP_DONE;
	return;
      }

      /* Setup to send the next block */
      ++cl->block;
      ++readblock;
      cl->state = ST_SEND;
      cl->wakeup = 0;
      cl->retries = WAIT_RETRIES;
    }
  }

  /* See if it's time to resend */
  if (cl->wakeup != 0 && cl->wakeup <= now &&
      (cl->state == ST_WAITACK || cl->state == ST_LASTACK))
  {
    ++stats.retrans;
    log (LOG_DEBUG, "%s.%d retrans",
	 inet_ntoa (cl->sin.sin_addr), cl->sin.sin_port);
    cl->state = ST_SEND;
  }

  /* Initialize some stuff the first time through */
  if (cl->state == ST_INIT)
  {
    cl->block = 1;
    ++readblock;
    cl->state = ST_SEND;
    cl->wakeup = 0;
    cl->retries = WAIT_RETRIES;
    cl->dp = r_init (&cl->ra);
  }

  /* If our first time through or the last block was ack'ed, read */
  if (readblock)
  {
    errno = 0;
    cl->dpcc = readit (&cl->ra, cl->f, &dp, cl->convert);
    cl->dp = dp;

    /* Bail if we run into trouble */
    if (cl->dpcc < 0)
    {
      ++stats.fileioerror;
      nak (cl, errno + 100);
      return;
    }

    cl->dpcc += 4;
    dp->th_opcode = htons ((u_short) DATA);
    dp->th_block = htons ((u_short) cl->block);
  }

  if (cl->state == ST_SEND)
  {
    /* See if it's time to give up on this turkey */
    if (cl->retries-- < 0)
    {
      ++stats.timeout;
      log (LOG_INFO, "%s.%d timeout",
	   inet_ntoa (cl->sin.sin_addr), cl->sin.sin_port);
      cl->proc = CP_DONE;
      return;
    }

    /* Send the block */
    if (sendto (s, (char *) cl->dp, cl->dpcc, 0,
		(struct sockaddr *) &cl->sin, sizeof (cl->sin)) != cl->dpcc)
    {
      oerrno = errno;
      log (LOG_ERR, "sendfile: write: %m");
      nak (cl, oerrno + 100);
      return;
    }
    ++stats.sent;

    if (debug > 1)
      log (LOG_DEBUG, "%s.%d > %d",
	   inet_ntoa (cl->sin.sin_addr),
	   cl->sin.sin_port,
	   cl->block);

    /* Read ahead next block */
    read_ahead (&cl->ra, cl->f, cl->convert);

    if (cl->dpcc == PKTSIZE)
      cl->state = ST_WAITACK;
    else
      cl->state = ST_LASTACK;
    cl->wakeup = now + WAIT_TIMEOUT;
  }
}

/*
 * Receive a file
 */
static void recvfile (struct client *cl)
{
  log (LOG_ERR, "recvfile: can't get here");
  finish (1);
}

static struct errmsg {
       int   e_code;
       char *e_msg;
     } errmsgs[] = {
       { EUNDEF,     "Undefined error code" } ,
       { ENOTFOUND,  "File not found"  } ,
       { EACCESS,    "Access violation"  } ,
       { ENOSPACE,   "Disk full or allocation exceeded" } ,
       { EBADOP,     "Illegal TFTP operation" } ,
       { EBADID,     "Unknown transfer ID" } ,
       { EEXISTS,    "File already exists" } ,
       { ENOUSER,    "No such user" } ,
       { 0,          NULL }
     };

static char *errtomsg (int error)
{
  struct errmsg *pe;
  static char  buf[32];

  if (error == 0)
     return ("success");

  for (pe = errmsgs; pe->e_msg; pe++)
      if (pe->e_code == error)
         return (pe->e_msg);

  sprintf (buf, "error %d", error);
  return (buf);
}

/* 
 * Send a nak packet (error message).
 * Error code passed in is one of the
 * standard TFTP codes, or a UNIX errno
 * offset by 100.
 */
static void nak (struct client *cl, int error)
{
  struct tftphdr *tp;
  struct errmsg  *pe;
  char   buf[PKTSIZE];
  int    size;

  memset (buf, 0, sizeof(buf));
  tp = (struct tftphdr *) buf;
  tp->th_opcode = htons ((u_short) ERROR);
  tp->th_code = htons ((u_short) error);
  for (pe = errmsgs; pe->e_code >= 0; pe++)
    if (pe->e_code == error)
      break;

  if (pe->e_code < 0)
  {
    pe->e_msg = strerror (error - 100);
    tp->th_code = EUNDEF;	/* set 'undef' errorcode */
  }
  strcpy (tp->th_msg, pe->e_msg);
  size = strlen (pe->e_msg);
  size += 4 + 1;
  if (sendto (s, buf, size, 0, (struct sockaddr *) &cl->sin,
	      sizeof (cl->sin)) != size)
    log (LOG_ERR, "nak: %m");
  cl->proc = CP_DONE;
}

/*
 * Exit with a syslog
 */
static void finish (int status)
{
  log (LOG_DEBUG, "exiting");
  exit (status);
}

/*
 * Signal handler for when something bad happens
 */
void die (int signo)
{
  finish (1);
}

/*
 * Signal handler to output statistics
 */
void logstats (int signo)
{
  if (!candostats)
  {
    ++dostats;
    return;
  }
  dostats = 0;
  log (LOG_DEBUG, "logstats: %d clientlistcnt", clientlistcnt);
  log (LOG_DEBUG, "logstats: %ld retrans", stats.retrans);
  log (LOG_DEBUG, "logstats: %ld timeout", stats.timeout);
  log (LOG_DEBUG, "logstats: %ld badop", stats.badop);
  log (LOG_DEBUG, "logstats: %ld errorop", stats.errorop);
  log (LOG_DEBUG, "logstats: %ld badfilename", stats.badfilename);
  log (LOG_DEBUG, "logstats: %ld badfilemode", stats.badfilemode);
  log (LOG_DEBUG, "logstats: %ld wrongblock", stats.badfilemode);
  log (LOG_DEBUG, "logstats: %ld fileioerror", stats.fileioerror);
  log (LOG_DEBUG, "logstats: %ld sent", stats.sent);
  log (LOG_DEBUG, "logstats: %ld maxclients", stats.maxclients);
}

/*
 * Allocate a new client struct
 */
static struct client *newclient (void)
{
  struct client *cl, **clp;
  static int allocfailure = 0;
  u_int  n, size;

  if (clientlistcnt >= clientlistsize)
  {
    /* If we've every had an allocation failure, give up now
     */
    if (allocfailure)
       return (NULL);

    n = MORECLIENTS;

    /* Allocate array of pointers
     */
    if (clientlist == NULL)
    {
      size = n * sizeof (clientlist[0]);
      clientlist = malloc (size);
      if (clientlist == NULL)
      {
	log (LOG_ERR, "newclient: list malloc: %m");
	finish (1);
      }
      clp = clientlist;
    }
    else
    {
      size = (clientlistsize + n) * sizeof (clientlist[0]);
      clientlist = realloc (clientlist, size);
      if (clientlist == NULL)
      {
	log (LOG_ERR, "newclient: list realloc: %m");
	++allocfailure;
	return (NULL);
      }
      clp = clientlist + clientlistsize;
    }

    size = n * sizeof (*cl);
    cl   = malloc (size);
    if (cl == NULL)
    {
      log (LOG_ERR, "newclient: client malloc: %m");
      ++allocfailure;
      return (NULL);
    }
    memset (cl, 0, size);
    clientlistsize += n;
    while (n-- > 0)
      *clp++ = cl++;

  }
  cl = *(clientlist + clientlistcnt);
  ++clientlistcnt;
  if (stats.maxclients < clientlistcnt)
    stats.maxclients = clientlistcnt;
  return (cl);
}

static struct client *findclient (struct sockaddr_in *sinp)
{
  struct client      **clp;
  struct sockaddr_in  *sinp2;
  int    n = clientlistcnt;

  clp = clientlist;
  while (n > 0 && (*clp)->proc != CP_FREE)
  {
    sinp2 = &(*clp)->sin;
    if (sinp->sin_port == sinp2->sin_port &&
	sinp->sin_addr.s_addr == sinp2->sin_addr.s_addr)
      return (*clp);
    --n;
    ++clp;
  }
  return (NULL);
}

static void freeclient (struct client *cl)
{
  struct client **clp;
  int    n, size;

  if (debug)
    log (LOG_DEBUG, "%s.%d ending",
	 inet_ntoa (cl->sin.sin_addr), cl->sin.sin_port);

  if (cl->f)
     fclose (cl->f);
  memset (cl, 0, sizeof(*cl));

  for (n = 0, clp = clientlist; *clp != cl; ++n, ++clp)
    if (n >= clientlistcnt)
    {
      log (LOG_ERR, "freeclient: can't find client");
      finish (1);
    }

  size = (clientlistcnt - (n + 1)) * sizeof(*clp);
  if (size > 0)
     memcpy (clientlist + n, clientlist + n + 1, size);

  --clientlistcnt;
  clp = clientlist + clientlistcnt;
  *clp = cl;
}
