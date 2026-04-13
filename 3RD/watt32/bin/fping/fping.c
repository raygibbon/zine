/*
 * fping: fast-ping, file-ping for Waterloo tcp/ip
 *
 * Used to send out ping requests to a list of hosts in a round robin
 * fashion.
 *
 *
 *
 * Standard RCS Header information (see co(1))
 *
 * $Author: schemers $
 *
 * Revision 1.30  1998/07/23  11:16:38  giva
 *
 * Rewritten for Waterloo tcp/ip under DOS
 *
 * $Date: 1993/02/23 00:16:38 $
 *
 * $Revision: 1.20 $
 *
 * $Locker: schemers $
 *
 * $Source: /networking/src/fping/RCS/fping.c,v $
 *
 * $State: Exp $
 *
 * $Log: fping.c,v $
 * Revision 1.20  1993/02/23  00:16:38  schemers
 * fixed syntax error (should have compiled before checking in...)
 *
 * Revision 1.19  1993/02/23  00:15:15  schemers
 * turned off printing of "is alive" when -a is specified.
 *
 * Revision 1.18  1992/07/28  15:16:44  schemers
 * added a fflush(stdout) call before the summary is sent to stderr, so
 * everything shows up in the right order.
 *
 * Revision 1.17  1992/07/23  03:29:42  schemers
 * fixed declaration of timeval_diff.
 *
 * Revision 1.16  1992/07/22  19:24:37  schemers
 * Modified file reading so it would skip blanks lines or lines starting
 * with a '#'. Now you can do something like:
 *
 * fping -ad < /etc/hosts
 *
 * Revision 1.15  1992/07/21  17:07:18  schemers
 * Put in sanity checks so only root can specify "dangerous" options.
 * Changed usage to show switchs in alphabetical order.
 *
 * Revision 1.14  1992/07/21  16:40:52  schemers
 * Now when sendto returns an error, the host is considered unreachable and
 * and the error message (from errno) is displayed.
 *
 * Revision 1.13  1992/07/17  21:02:17  schemers
 * changed default timeout to 2500 msec (for WANs), and default try
 * to 3. This gives 10 second overall timeout.
 *
 * Added -e option for showing elapsed (round-trip) time on packets
 *
 * Modified -s option to inlude to round-trip stats
 *
 * Added #ifndef DEFAULT_* stuff its easier to change the defaults
 *
 * Reorganized main loop.
 *
 * cleaned up timeval stuff. removed set_timeval and timeval_expired
 * since they aren't needed anymore. Just use timeval_diff.
 *
 * Revision 1.12  1992/07/17  16:38:54  schemers
 * move socket create call so I could do a setuid(getuid()) before the
 * fopen call is made. Once the socket is created root privs aren't needed
 * to send stuff out on it.
 *
 * Revision 1.11  1992/07/17  16:28:38  schemers
 * moved num_timeout counter. It really was for debug purposes and didn't
 * make sense to the general public :-) Now it is the number of timeouts
 * (pings that didn't get received with the time limit).
 *
 * Revision 1.10  1992/07/16  16:24:38  schemers
 * changed usage() to use fprintf(stderr,"...");
 *
 * Revision 1.9  1992/07/16  16:00:04  schemers
 * Added _NO_PROTO stuff for older compilers, and _POSIX_SOURCE
 * for unistd.h, and _POSIX_SOURCE for stdlib.h. Also added
 * check for __cplusplus.
 *
 * Revision 1.8  1992/07/16  05:44:41  schemers
 * changed -a and -u to only show hostname in results. This is
 * for easier parsing. Also added -v flag
 *
 * Revision 1.7  1992/07/14  18:45:23  schemers
 * initialized last_time in add_host function
 *
 * Revision 1.6  1992/07/14  18:32:40  schemers
 * changed select to use FD_ macros
 *
 * Revision 1.5  1992/07/14  17:21:22  schemers
 * standardized exit status codes
 *
 * Revision 1.4  1992/06/26  15:25:35  schemers
 * changed name from rrping to fping
 *
 * Revision 1.3  1992/06/24  15:39:32  schemers
 * added -d option for unreachable systems
 *
 * Revision 1.2  1992/06/23  03:01:23  schemers
 * misc fixes from R.L. "Bob" Morgan
 *
 * Revision 1.1  1992/06/19  18:23:52  schemers
 * Initial revision
 *
 *--------------------------------------------------
 * Copyright (c) 1992 Board of Trustees
 *            Leland Stanford Jr. University
 ***************************************************
 */

/*
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by Stanford University.  The name of the University may not be used
 * to endorse or promote products derived from this software without
 * specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <process.h>

#include <sys/socket.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <tcp.h>

#ifdef __DJGPP__
#include <unistd.h>
#endif

/* constants */

#ifndef DEFAULT_INTERVAL
#define DEFAULT_INTERVAL 25        /* default time between packets (msec) */
#endif

#ifndef DEFAULT_TIMEOUT
#define DEFAULT_TIMEOUT 2500       /* individual host timeouts (ms) */
#endif

#ifndef DEFAULT_RETRY
#define DEFAULT_RETRY 3            /* number of times to retry a host */
#endif


/* entry used to keep track of each host we are pinging */

typedef struct host_entry {
        char              *host;             /* text description of host */
        struct sockaddr_in saddr;            /* internet address */
        int                i;                /* index into array */
        int                num_packets_sent; /* number of ping packets sent */
        struct host_entry *prev,*next;       /* doubly linked list */
        struct timeval     last_time;        /* time of last packet sent */
      } HOST_ENTRY;


HOST_ENTRY  *rrlist = NULL;   /* linked list of hosts be pinged */
HOST_ENTRY **table  = NULL;   /* array of pointers to items in the list */
HOST_ENTRY  *cursor;

char  *filename;              /* file containing hosts to ping */
char  *prog;
int    s;                     /* socket */

int    retry         = DEFAULT_RETRY;
int    timeout       = DEFAULT_TIMEOUT;
int    interval      = DEFAULT_INTERVAL;
long   max_reply     = 0;
long   min_reply     = 10000;
int    total_replies = 0;
double sum_replies   = 0;

int    ident;                      /* packet identifier */
int    num_waiting;                /* number of hosts we are pinging */
int    num_hosts;                  /* total number of hosts */

int    num_alive        = 0;       /* total number alive */
int    num_unreachable  = 0;       /* total number unreachable */
int    num_noaddress    = 0;       /* total number of addresses not found */

int    num_timeout      = 0;       /* number of times select timed out */
int    num_pingsent     = 0;       /* total pings sent */
int    num_pingreceived = 0;       /* total pings received */

int    verbose_flag, dns_flag, debug_flag;
int    stats_flag, unreachable_flag;
int    alive_flag, elapsed_flag, version_flag;

struct timeval  timeout_timeval;
struct timezone tz;

struct timeval current_time;       /* current time (pseudo) */
struct timeval start_time;
struct timeval end_time;


char *get_host_by_address (struct in_addr in)
{
  struct hostent *h = gethostbyaddr ((char*)&in,sizeof(in),AF_INET);

  if (!h || !h->h_name)
     return inet_ntoa(in);
  return (h->h_name);
}


void remove_job (HOST_ENTRY *h)
{
  table[h->i]=NULL;
  --num_waiting;

  if (num_waiting)         /* remove us from list of jobs */
  {
    h->prev->next = h->next;
    h->next->prev = h->prev;
    if (h == cursor)
       cursor = h->next;
  }
  else
  {
    cursor = NULL;
    rrlist = NULL;
  }
}


void crash_and_burn (char *message)
{
  if (verbose_flag)
     fprintf (stderr,"%s: %s\n",prog,message);
  exit (4);
}

void errno_crash_and_burn (char *message)
{
  if (verbose_flag)
     fprintf (stderr,"%s: %s : %s\n",prog,message,strerror(errno));
  exit (4);
}

double timeval_diff (const struct timeval *a, const struct timeval *b)
{
  double temp = (((a->tv_sec*1000000) + a->tv_usec) -
                 ((b->tv_sec*1000000) + b->tv_usec))/1000;
  return (temp);

}

/*
 * Compose and transmit an ICMP_ECHO REQUEST packet.  The IP packet
 * will be added on by the kernel.  The ID field is our UNIX process ID,
 * and the sequence number is an index into an array of outstanding
 * ping requests. The sequence number will later be used to quickly
 * figure out who the ping reply came from.
 *
 */
void send_ping (int s, HOST_ENTRY *h)
{
  static char buffer[32];
  struct icmp *icp = (struct icmp *) buffer;
  int    n,len;

  gettimeofday (&h->last_time,&tz);

  icp->icmp_type  = ICMP_ECHO;
  icp->icmp_code  = 0;
  icp->icmp_cksum = 0;
  icp->icmp_seq   = h->i;
  icp->icmp_id    = ident;
#define SIZE_ICMP_HDR 8
#define SIZE_PACK_SENT (sizeof(h->num_packets_sent))
#define SIZE_LAST_TIME (sizeof(h->last_time))

  memcpy (&buffer[SIZE_ICMP_HDR],&h->last_time, SIZE_LAST_TIME);
  memcpy (&buffer[SIZE_ICMP_HDR+SIZE_LAST_TIME], &h->num_packets_sent, SIZE_PACK_SENT);

  len = SIZE_ICMP_HDR + SIZE_LAST_TIME + SIZE_PACK_SENT;

  icp->icmp_cksum = ~inchksum ((void*)icp,len);

  n = sendto (s,buffer,len,0,(struct sockaddr*)&h->saddr,
              sizeof(struct sockaddr_in));
  if (n < 0 || n != len)
  {
    if (verbose_flag || unreachable_flag)
    {
      if (dns_flag)
           printf (get_host_by_address(h->saddr.sin_addr));
      else printf (cursor->host);
      if (verbose_flag)
          printf (" error while sending ping: %s\n", strerror(errno));
      printf ("\n");
    }
    num_unreachable++;
    remove_job (h);
  }
  else
  {
    h->num_packets_sent++;
    num_pingsent++;
  }

}

/*
 * recvfrom_wto: receive with timeout
 *      returns length of data read or -1 if timeout
 *      crash_and_burn on any other errrors
 *
 */
int recvfrom_wto (int s, char *buf, int len, struct sockaddr *saddr, int timo)
{
  int    nfound, slen, n;
  struct timeval to;
  fd_set readset, writeset;

  to.tv_sec  = timo/1000;
  to.tv_usec = (timo - (to.tv_sec*1000))*1000;

  FD_ZERO (&readset);
  FD_ZERO (&writeset);
  FD_SET (s,&readset);
  nfound = select_s (s+1,&readset,&writeset,NULL,&to);
  if (nfound < 0)
     errno_crash_and_burn ("select");
  if (nfound == 0)
     return (-1);  /* timeout */

  slen = sizeof (struct sockaddr);
  n = recvfrom (s,buf,len,0,saddr,&slen);
  if (n < 0)
     errno_crash_and_burn ("recvfrom");
  return (n);
}

int wait_for_reply (void)
{
  char   buffer [2048];
  struct sockaddr_in response_addr;
  struct timeval     sent_time;
  struct ip   *ip;
  struct icmp *icp;
  HOST_ENTRY  *h;

  int  result, n, hlen;
  long this_reply;
  int  the_index;

  result = recvfrom_wto (s,buffer,sizeof(buffer),
                         (struct sockaddr *)&response_addr,interval);
  if (result < 0)
     return (0);

  ip   = (struct ip *) buffer;
  hlen = ip->ip_hl << 2;
  if (result < hlen + ICMP_MINLEN)
  {
    if (verbose_flag)
       fprintf (stderr,"%s: too short\n", inet_ntoa(response_addr.sin_addr));
    return (1);
  }

  icp = (struct icmp *)(buffer + hlen);

  if (icp->icmp_type != ICMP_ECHOREPLY ||
      icp->icmp_id   != ident)
     return (1);  /* packet received, but not the one we are looking for! */

  num_pingreceived++;

  if (icp->icmp_seq >= num_hosts)
  {
    if (verbose_flag)
       fprintf (stderr,"%s: seq >= # hosts\n", inet_ntoa(response_addr.sin_addr));
    return (1);
  }
  if (table[icp->icmp_seq] == NULL)
  {
    if (verbose_flag)
       fprintf (stderr,"%s: no seq\n", inet_ntoa(response_addr.sin_addr));
    return (1);
  }
  if (table[icp->icmp_seq]->saddr.sin_addr.s_addr != response_addr.sin_addr.s_addr)
  {
    if (verbose_flag)
       fprintf (stderr,"%s: wrong src\n", inet_ntoa(response_addr.sin_addr));
    return (1);  /* packet received, don't know about it anymore */
  }

  n = icp->icmp_seq;
  h = table[n];

  gettimeofday (&current_time,&tz);
  memcpy (&sent_time, &icp->icmp_data[0], sizeof(sent_time));
  memcpy (&the_index, &icp->icmp_data[SIZE_LAST_TIME], sizeof(the_index));
  this_reply = (long) timeval_diff (&current_time,&sent_time);

  if (this_reply > max_reply)
     max_reply = this_reply;
  if (this_reply < min_reply)
     min_reply = this_reply;

  sum_replies += this_reply;
  total_replies++;

  if (verbose_flag || alive_flag)
  {
    if (dns_flag)
         printf (get_host_by_address(response_addr.sin_addr));
    else printf (h->host);
    if (verbose_flag)
       printf (" is alive");
    if (elapsed_flag)
       printf (" (%ld msec)",this_reply);
    printf ("\n");
  }
  num_alive++;
  remove_job (h);     /* remove job */
  return (num_waiting);
}


/*
 * Add host to linked list of hosts to be pinged
 * assume memory for *host is ours!!!
 */
void add_host (char *host)
{
  HOST_ENTRY *p;
  struct hostent *host_ent;
  struct in_addr *host_add  = NULL;
  u_long          ipaddress = inet_addr (host);

  if (ipaddress == INADDR_NONE &&
      ((host_ent = gethostbyname(host)) == NULL ||
      ((host_add = (struct in_addr*)*(host_ent->h_addr_list)) == NULL)))
  {
    if (verbose_flag)
       fprintf (stderr,"%s address not found\n",host);
    num_noaddress++;
    return;
  }

  p = malloc (sizeof(HOST_ENTRY));
  if (!p)
     crash_and_burn ("can't allocate HOST_ENTRY");

  p->host              = host;
  p->num_packets_sent  = 0;
  p->last_time.tv_sec  = 0;
  p->last_time.tv_usec = 0;

  memset (&p->saddr,0,sizeof(p->saddr));
  p->saddr.sin_family = AF_INET;

  if (ipaddress == INADDR_NONE)
       p->saddr.sin_addr        = *host_add;
  else p->saddr.sin_addr.s_addr = ipaddress;

  if (!rrlist)
  {
    rrlist  = p;
    p->next = p;
    p->prev = p;
  }
  else
  {
    p->next = rrlist;
    p->prev = rrlist->prev;
    p->prev->next = p;
    p->next->prev = p;
    rrlist = p;
  }
  num_hosts++;
}



void usage (void)
{
  fprintf (stderr,"\n"
    "Usage: %s [options] [systems...]\n"
    "   -a         show systems that are alive\n"
    "   -d         use dns to lookup address for return ping packet\n"
    "   -e         show elapsed time on return packets\n"
    "   -f file    read list of systems from a file (- means stdin)\n"
    "   -i n       interval (between ping packets) in milliseconds (default %d)\n"
    "   -q         quiet (don't show per host results)\n"
    "   -r n       retry limit (default %d)\n"
    "   -s         dump final stats\n"
    "   -t n       individual host timeout in milliseconds (default %d)\n"
    "   -u         show systems that are unreachable\n"
    "   -v         show version\n"
    "   -D         debug mode\n"
    "   systems    list of systems to check (if no -f specified)\n\n",
    prog, interval, retry, timeout);
  exit (3);
}


int main (int argc, char **argv)
{
  int c;

  prog  = argv[0];
  ident = getpid() & 0xFFFF;

  verbose_flag = 1;
  opterr = 0;

  if (argc < 2)
     usage();

  while ((c = getopt(argc,argv,"ed?hqusavt:i:f:r:D")) != EOF)
     switch (c)
     {
       case 't': if ((timeout = atoi(optarg)) < 0)
                    usage();
                 break;
       case 'f': filename = optarg;
                 break;
       case 'r': if ((retry = atoi(optarg)) < 0)
                    usage();
                 break;
       case 'i': if ((interval = atoi(optarg)) < 0)
                    usage();
                 break;
       case 'q': verbose_flag = 0;
                 break;
       case 'e': elapsed_flag = 1;
                 break;
       case 'd': dns_flag = 1;
                 break;
       case 's': stats_flag = 1;
                 break;
       case 'u': unreachable_flag = 1;
                 break;
       case 'a': alive_flag = 1;
                 break;
       case 'v': printf ("%s: $Revision: 1.30 $ $Date: 1998/07/23$\n", argv[0]);
                 printf ("%s: comments to gvanem@yahoo.no\n", argv[0]);
                 return (0);
       case 'D': dbug_init();
                 debug_flag = 1;
                 break;
       case '?':
       case 'h': usage();
                 break;
       default : fprintf (stderr,"Unknown flag: %s\n", argv[0]);
                 usage();
                 break;
     }

  if (unreachable_flag && alive_flag)
  {
    fprintf (stderr,"%s: specify only one of a,u\n", argv[0]);
    usage();
  }

  if (interval < 10 || retry > 20 || timeout < 250)
  {
    fprintf (stderr,"%s: these options are too risky for mere mortals.\n",prog);
    fprintf (stderr,"%s: You need i >=10, retry < 20, and t >= 250\n",prog);
    return (3);
  }

  s = socket (AF_INET,SOCK_RAW,IPPROTO_ICMP);
  if (s < 0)
     errno_crash_and_burn ("can't create raw socket");

#if 1
  setsockopt (s,SOL_SOCKET,SO_DEBUG,&debug_flag,sizeof(debug_flag));
#endif

  if (alive_flag || unreachable_flag)
     verbose_flag = 0;

  argv = &argv[optind];
  if (*argv && filename)
     usage();

  if (!*argv && !filename)
     filename = "-";

  if (*argv)
     while (*argv)
       add_host (*argv++);

  else if (filename)
  {
    FILE *ping_file;
    char line[132];
    char host[132], *p;
    if (!strcmp(filename,"-"))
         ping_file = stdin;
    else ping_file = fopen (filename,"r");

    if (!ping_file)
       crash_and_burn ("cannot open infile");

    while (fgets(line,sizeof(line),ping_file))
    {
      sscanf (line,"%s",host);
      if (!*host || host[0] == '#')  /* magic to avoid comments */
         continue;
      p = strdup (host);
      if (!p)
         crash_and_burn ("can't malloc host");
      add_host (p);
    }
    fclose (ping_file);
  }
  else
    usage();

  if (!num_hosts)
     return (2);

  /* allocate array to hold outstanding ping requests */

  table = malloc (sizeof(HOST_ENTRY*)*num_hosts);
  if (!table)
     crash_and_burn ("Can't malloc array of hosts");

  cursor = rrlist;

  for (num_waiting = 0; num_waiting < num_hosts; num_waiting++)
  {
    table[num_waiting] = cursor;
    cursor->i = num_waiting;
    cursor    = cursor->next;
  }


  gettimeofday (&start_time,&tz);
  cursor = rrlist;

  while (num_waiting)   /* while pings are outstanding */
  {
    if ((timeval_diff(&current_time,&cursor->last_time) > timeout) ||
        cursor->num_packets_sent == 0)
    {
      if (cursor->num_packets_sent > 0)
         num_timeout++;
      if (cursor->num_packets_sent == retry+1)
      {
        if (verbose_flag || unreachable_flag)
        {
          if (dns_flag)
               printf (get_host_by_address(cursor->saddr.sin_addr));
          else printf (cursor->host);
          if (verbose_flag)
             printf (" is unreachable");
          printf ("\n");
        }
        num_unreachable++;
        remove_job (cursor);
      }
      else
        send_ping (s,cursor);
    }
    while (wait_for_reply() && num_waiting)  /* call wfr until we timeout */
          ; /* wait! */

    gettimeofday (&current_time,&tz);
    if (cursor)
       cursor = cursor->next;
  }

  gettimeofday (&end_time,&tz);

  if (stats_flag)
  {
    fflush (stdout);
    fprintf (stderr,"\n");
    fprintf (stderr," %8d hosts\n",num_hosts);
    fprintf (stderr," %8d alive\n",num_alive);
    fprintf (stderr," %8d unreachable\n",num_unreachable);
    fprintf (stderr," %8d unknown addresses\n\n",num_noaddress);

    fprintf (stderr," %8d timeouts (waiting for response)\n",num_timeout);
    fprintf (stderr," %8d pings sent\n",num_pingsent);
    fprintf (stderr," %8d pings received\n\n",num_pingreceived);

    if (total_replies==0)
    {
      min_reply = 0;
      max_reply = 0;
      total_replies = 1;
      sum_replies   = 0;
    }
    fprintf (stderr," %8ld msec (min round trip time)\n",min_reply);
    fprintf (stderr," %8ld msec (avg round trip time)\n",(long)sum_replies/total_replies);
    fprintf (stderr," %8ld msec (max round trip time)\n",max_reply);
    fprintf (stderr," %8.3f sec (elapsed real time)\n\n",
             timeval_diff(&end_time,&start_time)/1000.0);
  }

  if (num_noaddress)
     return (2);
  if (num_alive != num_hosts)
     return (1);
  return (0);
}

