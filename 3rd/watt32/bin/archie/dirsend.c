/*
 * Copyright (c) 1989, 1990, 1991 by the University of Washington
 *
 * For copying and distribution information, please see the file
 * <copyright.h>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <tcp.h>
#include <netdb.h>
#include <conio.h>

#include "pfs.h"
#include "perrno.h"
#include "archie.h"

/*
 * These were locals in dirsend(). Note that the initializations here
 * are really meaningless since we have to redo them for each call to
 * dirsend() since they were formerly automatically initialized.
 */
static struct ptext *first = NULL;   /* First returned packet         */
static struct ptext *next;           /* The one we are waiting for    */
static struct ptext *vtmp;           /* For reorganizing linked list  */
static struct ptext *comp_thru;      /* We have all packets though    */
static struct ptext *pkt;
static struct ptext *dirsendReturn;
static tcp_Socket    sock;           /* Opened UDP/TCP socket         */

static int     dirsendDone;
static int     udp_port              = PROSPERO_PORT;
static WORD    next_conn_id          = 0;
static int     client_dirsrv_timeout = CLIENT_DIRSRV_TIMEOUT;
static int     client_dirsrv_retry   = CLIENT_DIRSRV_RETRY;
static int     tmp;

static int     hdr_len;            /* Header Length                 */
static int     nd_pkts;            /* Number of packets we want     */
static int     no_pkts;            /* Number of packets we have     */
static int     pkt_cid;            /* Packet connection identifier  */
static WORD    this_conn_id;       /* Connection ID (host order)    */
static char   *seqtxt  = NULL;     /* Pointer to text w/ sequence # */
static DWORD   host_ip = 0;        /* IP address of host            */
static char   *ctlptr;             /* Pointer to control field      */
static short   stmp;               /* Temp short for conversions    */
static int     backoff;            /* Server requested backoff      */
static BYTE    rdflag11;           /* First byte of flags (bit vect)*/
static BYTE    rdflag12;           /* Second byte of flags (int)    */
static int     scpflag = 0;        /* Set if any sequencd cont pkts */
static int     ackpend = 0;        /* Acknowledgement pending       */
static int     gaps = 0;           /* Gaps present in recvd pkts    */
static time_t  timeout;            /* Time to wait for response     */
static time_t  ackwait;            /* Time to wait before acking    */
static time_t  gapwait;            /* Time to wait b4 filling gaps  */
static time_t *selwait;            /* Time to wait for select       */
static int     retries;            /* retry counter                 */
static char    hostname[128];      /* lmjm: saves _inet_ntoa() str  */

/* Values for dirsendDone
 */
#define DSRET_DONE           1
#define DSRET_SEND_ERROR    -1
#define DSRET_RECV_ERROR    -2
#define DSRET_SELECT_ERROR  -3
#define DSRET_TIMEOUT       -4
#define DSRET_ABORTED       -5

static int  InitHostData   (char *host);
static int  InitDirsend    (void);
static void RetryDirsend   (void);
static void WaitingDirsend (void);
static void TimeoutProc    (void);
static void ProcessEvent   (void);

/*
 * dirsend - send packet and receive response
 *
 *   DIRSEND takes a pointer to a structure of type PTEXT and a host,
 *   It then sends the supplied packet off to the directory server on the
 *   specified host. If host is a valid numeric address, that address is used.
 *   Otherwise, the host is resolved up to obtain the address.
 *
 *   DIRSEND will wait for a response and retry an appropriate
 *   number of times as defined by timeout and retries (both static
 *   variables).  It will collect however many packets form the reply, and
 *   return them in a structure (or structures) of type PTEXT.
 *
 *   DIRSEND will free the packet that it is presented as an argument.
 *   The packet is freed even if dirsend fails.
 */

struct ptext *dirsend (struct ptext *pkt_p, char *host)
{
  pkt     = pkt_p;
  first   = NULL;
  scpflag = 0;
  ackpend = 0;
  gaps    = 0;
  retries = client_dirsrv_retry;

  if (!host_ip && !InitHostData(host))
     return (NULL);

  if (!InitDirsend())
     return (NULL);

  RetryDirsend();    /* set the first timeout */

  dirsendReturn = NULL;
  dirsendDone   = 0;

  /* Until one of the callbacks says to return, keep processing events
   */
  while (!dirsendDone)
        ProcessEvent();

  /* Return whatever we're supposed to
   */
  return (dirsendReturn);
}


/*
 * This function does all the initialization that used to be done at the
 * start of dirsend(), including opening the socket "&sock". It returns (1)
 * if successful, otherwise (0) to indicate that dirsend() should return
 * NULL immediately.
 */
static int InitDirsend (void)
{ 
  timeout   = client_dirsrv_timeout;
  gapwait   = client_dirsrv_timeout < 5 ? client_dirsrv_timeout : 5;
  ackwait   = 0;
  comp_thru = NULL;
  perrno    = 0;
  nd_pkts   = 0;
  no_pkts   = 0;
  pkt_cid   = 0;

  /* Find first connection ID
   */
  if (next_conn_id == 0)
  {
    srand (time(NULL));
    next_conn_id = rand();
  }

  /* Must open a new port each time. we do not want to see old
   * responses to messages we are done with
   */
  if (!udp_open(&sock,DIRSRV_PORT,host_ip,udp_port,NULL))
  {
    Adebug (-1, "Can't open udp socket\n");
    perrno = DIRSEND_UDP_CANT;
    ptlfree (pkt);
    return (0);
  }

  /* Add ARDP protocol header
   */
  if (rdgram_prio)
  {
    pkt->start  -= 15;
    pkt->length += 15;
    *(char *)(pkt->start)    = 15;
    *(DWORD*)(pkt->start+9)  = 0L;
    *(char *)(pkt->start+11) = 2;
    *(short*)(pkt->start+13) = htons (rdgram_prio);
  }
  else
  {
    pkt->start  -= 9;
    pkt->length += 9;
    *pkt->start  = 9;
  }

  this_conn_id = next_conn_id++;
  if (next_conn_id == 0)
      next_conn_id++;

  *(WORD*)(pkt->start+1) = htons (this_conn_id);
  *(WORD*)(pkt->start+3) = 0x100;
  *(WORD*)(pkt->start+5) = 0x100;
  *(WORD*)(pkt->start+7) = 0;

  first = ptalloc();
  next  = first;

  return (1);
}

/*
 * First time initialisation of host-ip and port number:
 *   If the "archie" protocol is listed in the service file, use that data.
 *   If "host(x)" is given, extract the port number "x".
 *   If host is a name (not an IP), resolve it into an IP-address
 */
static int InitHostData (char *host)
{
  char           *paren;
  struct servent *serv;

  /* I we have a null host name, return an error
   */
  if (host == NULL || !*host)
  {
    Adebug (0, "dirsrv: NULL hostname specified\n");
    perrno = DIRSEND_BAD_HOSTNAME;
    ptlfree (pkt);
    return (0);
  }

  serv = getservbyname ("prospero-np", "udp");
  if (serv)
     udp_port = ntohs (serv->s_port);

  /* If a port is included, save it away
   */
  paren = strchr (host, '(');
  if (paren)
  {
    *paren = 0;
    udp_port = atoi (paren+1);
    Adebug (2, "Using port %u\n", udp_port);
  }

  if (!isaddr(host))    /* if non-numeric address */
  {
    char buf[20];
    Adebug (0, "resolving `%s'...", host);
    fflush (stdout);

    host_ip = resolve (host);
    if (!host_ip)
    {
      Adebug (-1, "Can't resolve host `%s'\n", host);
      perrno = DIRSEND_BAD_HOSTNAME;
      ptlfree (pkt);
      return (0);
    }
    Adebug (0, "[%s]\n", _inet_ntoa(buf,host_ip));
  }
  else
    host_ip = aton (host);

  strncpy (hostname, host, sizeof(hostname)-1);
  return (1);
}


/*
 * Retry the last packet. Now we resend the packet and call
 * WaitingDirsend() to wait for a reply.
 */
static void RetryDirsend (void)
{
  gaps = ackpend = 0;

  sock_fastwrite (&sock, pkt->start, pkt->length);
  Adebug (2, "Sending %d byte message to `%s'...\n", pkt->length, hostname);
  WaitingDirsend();
}


/*
 * We come back here if the packet received is only part of
 * the response, or if the response came from the wrong host.
 *
 * We set selwait to the appropriate values, then return to
 * allow event processing.
 */
static void WaitingDirsend (void)
{
  Adebug (2, "Waiting for reply...");
  fflush (stderr);

       if (ackpend) selwait = &ackwait;
  else if (gaps)    selwait = &gapwait;
  else              selwait = &timeout;
}


/*
 * This routine is called when a timeout occurs. It includes the code that
 * was formerly used when select() returned 0 (indicating a timeout).
 */
static void TimeoutProc (void)
{
  if (gaps || ackpend)  /* Send acknowledgment */
  {
    /* Acks are piggybacked on retries - If we have received
     * an ack from the server, then the packet sent is only
     * an ack and the rest of the message will be empty
     */
    Adebug (2, "Acknowledging (%s).\n", ackpend ? "requested" : "gaps");

    if (gaps && verbose)
       Adebug (0, "Searching...\n");
    RetryDirsend();
    return;
  }

  if (retries-- > 0)
  {
    timeout = CLIENT_DIRSRV_BACKOFF (timeout);
    Adebug (2, "Timed out. Setting timeout to %d seconds.\n", timeout);
    RetryDirsend();
    return;
  }

  Adebug (0, "dirsend: Retries exhausted");
  sock_close (&sock);
  perrno = DIRSEND_SELECT_FAILED;
  ptlfree (first);
  ptlfree (pkt);
  dirsendReturn = NULL;
  dirsendDone   = DSRET_TIMEOUT;
}

/*
 * This function is called whenever there's something to read on the
 * connection. It includes the code that was run when select() returned
 * greater than 0 (indicating read ready).
 */
static void ReadProc (void)
{
  next->start  = next->dat;
  next->length = sock_fastread (&sock, next->start, sizeof(next->dat));
  next->start [next->length] = 0;

  Adebug (2, "\nGot %d bytes from %s\n", next->length, hostname);

  /* For the current format, if the first byte is less than
   * 20, then the first two bits are a version number and the next six
   * are the header length (including the first byte).
   */
  hdr_len = *(BYTE*)next->start;
  if (hdr_len < 20)
  {
    ctlptr = next->start + 1;
    next->seq = 0;

    if (hdr_len >= 3)       /* Connection ID */
    {
      stmp = *(short*)ctlptr;
      if (stmp)
         pkt_cid = ntohs (stmp);
      ctlptr += 2;
    }

    if (pkt_cid && this_conn_id && pkt_cid != this_conn_id)
    {
      /* The packet is not for us */
      WaitingDirsend();
      return;
    }

    if (hdr_len >= 5)       /* Packet number */
    {
      stmp = *(short*)ctlptr;
      next->seq = ntohs (stmp);
      ctlptr += 2;
    }
    else  /* No packet number specified, so this is the only one */
    {
      next->seq = 1;
      nd_pkts = 1;
    }

    if (hdr_len >= 7)            /* Total number of packets */
    {
      stmp = *(short*)ctlptr;    /* 0 means don't know      */
      if (stmp)
         nd_pkts = ntohs (stmp);
      ctlptr += 2;
    }

    if (hdr_len >= 9)            /* Received through */
    {
      stmp = *(short*)ctlptr;    /* 1 means received request */
      if (stmp && ntohs(stmp) == 1)
      {
        /* Future retries will be acks only */
        pkt->length = 9;
        *(short*)(pkt->start+3) = 0;
        Adebug (2, "Server acked request - retries will be acks only\n");
      }
      ctlptr += 2;
    }

    if (hdr_len >= 11)           /* Backoff */
    {
      stmp = *(short*)ctlptr;
      if (stmp)
      {
        backoff = ntohs (stmp);
        Adebug (2, "Backing off to %d seconds\n", backoff);

        timeout = backoff;
        if (backoff > 60 && first == next && no_pkts == 0)
        {
          /* Probably a long queue on the server - don't give up */
          retries = client_dirsrv_retry;
        }
      }
      ctlptr += 2;
    }

    if (hdr_len >= 12)      /* Flags (1st byte) */
    {
      rdflag11 = *(BYTE*)ctlptr;
      if (rdflag11 & 0x80)
      {
        Adebug (2, "Ack requested\n");
        ackpend++;
      }
      if (rdflag11 & 0x40)
      {
        Adebug (2, "Sequenced control packet\n");
        next->length = -1;
        scpflag++;
      }
      ctlptr++;
    }

    if (hdr_len >= 13)      /* Flags (2nd byte) */
    {
      /* Reserved for future use */
      rdflag12 = *(BYTE*)ctlptr;
      ctlptr++;
    }

    if (next->seq == 0)
    {
      WaitingDirsend();
      return;
    }

    if (next->length >= 0)
        next->length -= hdr_len;
    next->start += hdr_len;
    goto done_old;
  }

  pkt_cid = 0;

  /* if intermediate format (between old and new), then process
   * and go to done_old
   */
  ctlptr = next->start + max (0,next->length-20);
  while (*ctlptr)
        ctlptr++;

  /* Control fields start after the terminating null
   */
  ctlptr++;

  /* Until old version are gone, must be 4 extra bytes minimum
   * When no version 3 servers, can remove the -4
   */
  if (ctlptr < next->start + next->length - 4)
  {
    /* Connection ID */
    stmp = *(short*)ctlptr;
    if (stmp)
       pkt_cid = ntohs (stmp);
    ctlptr += 2;

    if (pkt_cid && this_conn_id && pkt_cid != this_conn_id)
    {
      /* The packet is not for us */
      WaitingDirsend();
      return;
    }

    /* Packet number */
    if (ctlptr < (next->start + next->length))
    {
      stmp = *(short*)ctlptr;
      next->seq = ntohs (stmp);
      ctlptr += 2;
    }

    /* Total number of packets */
    if (ctlptr < (next->start + next->length))
    {
      stmp = *(short*)ctlptr;
      if (stmp)
         nd_pkts = ntohs (stmp);
      ctlptr += 2;
    }

    /* Received through */
    if (ctlptr < (next->start + next->length))
    {
      /* Not supported by clients */
      ctlptr += 2;
    }

    /* Backoff */
    if (ctlptr < (next->start + next->length))
    {
      stmp = *(short*)ctlptr;
      backoff = ntohs (stmp);
      Adebug (2, "Backing off to %d seconds\n", backoff);

      if (verbose && backoff)
         Adebug (0, "Searching...\n");
      if (backoff)
         timeout = backoff;
      ctlptr += 2;
    }
    if (next->seq == 0)
    {
      WaitingDirsend();
      return;
    }
    goto done_old;
  }

  /* Note that we have to start searching 11 bytes before the
   * expected start of the MULTI-PACKET line because the message
   * might include up to 10 bytes of data after the trailing null
   * The order of those bytes is two bytes each for Connection ID
   * Packet-no, of, Received-through, Backoff
   */

  seqtxt = nlsindex (next->start + max(0,next->length - 40),"MULTI-PACKET");
  if (seqtxt)
      seqtxt += 13;

  if (nd_pkts == 0 && no_pkts == 0 && seqtxt == NULL)
     goto all_done;

  tmp = sscanf (seqtxt,"%d OF %d", &next->seq, &nd_pkts);
  if (tmp < 2)
     Adebug (0, "Can't read packet sequence number: %s", seqtxt);

done_old:

  Adebug (2, "Packet %d of %d\n", next->seq, nd_pkts);

  if (first == next && no_pkts == 0)
  {
    if (first->seq == 1)
    {
      comp_thru = first;
      /* If only one packet, then return it */
      if (nd_pkts == 1)
         goto all_done;
    }
    else
      gaps++;
    no_pkts = 1;
    next = ptalloc();
    WaitingDirsend();
    return;
  }

  if (comp_thru && (next->seq <= comp_thru->seq))
     ptfree(next);
  else if (next->seq < first->seq)
  {
    vtmp  = first;
    first = next;
    first->next     = vtmp;
    first->previous = NULL;
    vtmp->previous  = first;
    if (first->seq == 1)
       comp_thru = first;
    no_pkts++;
  }
  else
  {
    vtmp = (comp_thru ? comp_thru : first);
    while (vtmp->seq < next->seq)
    {
      if (vtmp->next == NULL)
      {
        vtmp->next     = next;
        next->previous = vtmp;
        next->next     = NULL;
        no_pkts++;
        goto ins_done;
      }
      vtmp = vtmp->next;
    }
    if (vtmp->seq == next->seq)
       ptfree(next);
    else
    {
      vtmp->previous->next = next;
      next->previous = vtmp->previous;
      next->next     = vtmp;
      vtmp->previous = next;
      no_pkts++;
    }
  }

ins_done:

  while (comp_thru && comp_thru->next &&
         comp_thru->next->seq == comp_thru->seq + 1)
  {
    comp_thru = comp_thru->next;

    /* We've made progress, so reset retry count
     */
    retries = client_dirsrv_retry;

    /* Also, next retry will be only an acknowledgement
     * but for now, we can't fill in the ack field
     */
    Adebug (2, "Packets now received through %d\n", comp_thru->seq);
  }

  /* See if there are any gaps
   */
  if (!comp_thru || comp_thru->next)
       gaps++;
  else gaps = 0;

  if (nd_pkts == 0 || no_pkts < nd_pkts)
  {
    next = ptalloc();
    WaitingDirsend();
    return;
  }

all_done:
  if (ackpend)  /* Send acknowledgement if requested */
  {
    Adebug (2, "Acknowledging final packet\n");
    sock_fastwrite (&sock, pkt->start, pkt->length);
  }
  sock_close (&sock);
  ptlfree (pkt);

  /* Get rid of any sequenced control packets
   */
  if (scpflag)
  {
    while (first && first->length < 0)
    {
      vtmp  = first;
      first = first->next;
      if (first)
          first->previous = NULL;
      ptfree (vtmp);
    }
    vtmp = first;
    while (vtmp && vtmp->next)
    {
      if (vtmp->next->length < 0)
      {
        if (vtmp->next->next)
        {
          vtmp->next = vtmp->next->next;
          ptfree (vtmp->next->previous);
          vtmp->next->previous = vtmp;
        }
        else
        {
          ptfree (vtmp->next);
          vtmp->next = NULL;
        }
      }
      vtmp = vtmp->next;
    }
  }
  dirsendReturn = first;
  dirsendDone   = DSRET_DONE;
}

/*
 *
 */
static void ProcessEvent (void)
{
  DWORD timeout = set_timeout (1000 * (*(DWORD*)selwait));
  int   status  = 0;

  Adebug (-1, "\r%20s\rWaiting %lu seconds", "", *(DWORD*)selwait);

  while (!chk_timeout(timeout))
  {
    sock_tick (&sock, &status);

    if (sock_dataready(&sock))
    {
      ReadProc();
      return;
    }
    if (kbhit())
    {
      int c = getch();

      if (c == 27 || c == 3)
      {
        dirsendDone = DSRET_ABORTED;
        return;
      }
    }
  }

sock_err:

  if (chk_timeout(timeout))  /* timeout */
  {
    TimeoutProc();
    return;
  }
  if (sockerr(&sock))
  {
    Adebug (0, "\n%s\n", sockerr(&sock));
    perrno = DIRSEND_BAD_RECV;
  }
  else
    perrno = DIRSEND_SELECT_FAILED;

  ptlfree (first);
  ptlfree (pkt);
  dirsendReturn = NULL;
  dirsendDone   = DSRET_SELECT_ERROR;
}


void Adebug (int lvl, char *fmt, ...)
{
  if (pfs_debug > lvl)
  {
    va_list args;
    va_start (args, fmt);
    vfprintf (stderr, fmt, args);
    va_end (args);
  }
}
