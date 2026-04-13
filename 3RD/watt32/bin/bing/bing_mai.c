
/*
 *          B I N G . C   (Braden's Ping)
 *
 * This is yet another "pinger" program for Internet testing and Internet
 *    fault isolation using ICMP echoes.    It was (re-)written at the
 *    Information Sciences Institute of USC by Bob Braden, starting
 *    from a basic pinger program written by:
 *
 *              Mike Muuss
 *              U. S. Army Ballistic Research Laboratory
 *              December, 1983
 *
 * Status -
 *      Public Domain.  Distribution Unlimited.
 *
 * Significant extnesions: 01July87  Bob Braden ISI
 *        See man page for details.
 * Convert to Sun/OS4  12May89 Bob Braden ISI
 * Add -u parm, fix Sparcstation problem with inet_ntoa() call. 28Aug90 ISI
 * Fix bug in Black Hole reporting. 15Nov90 ISI
 * Split into two modules: bing_stat.c and bing_net.c  April 1992 ISI
 *
 * use getopt() for command-line parsing. Aug 1999 <gvanem@yahoo.no>
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <ctype.h>
#include <sys/param.h>
#include <sys/file.h>

#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <errno.h>

#define MAXBURST 20  /* Max value of -b parameter */
#define MAXPACKET   2048
u_char outpack[MAXPACKET];

extern int errno;
struct timezone tz;      /* leftover */
struct timeval  startod, sendtod ; /* start time, last-send time */
struct timeval  SIGINTime  ;

struct sockaddr_in target_sin;
u_long target_host;
int raw_sock;
int datalen = 64-ICMP_MINLEN;
int totallen = 64;
int is_slave = 0;

char usage[] =
  "[-dData size] [-tInterval(ms)]  [-bBurst size] "                 \
  "[-rRTT threshold (ms)] [-fReport frequency] [-nNumber to send] " \
  "[-hHistogram lines] [-e[raw]]  [-x] [-u]  Host \n";

char hostaddr[16];
char canhost[64] ;   /* canonical host name */

int burst_size = 1 ;        /* Number to send in each burst (default = 1) */
int interval = 1000 ;       /* Pinging interval in ms. */
int send_limit = 99999999 ; /* Limit on number to send */
int tell_all = 0;           /* parm -x => dump all unexpected ICMP packets */
int echo_host = 0 ;         /* For echo host: send ECHO REPLY */
int RAW_echo = 0 ;          /* For echo host: send protocol 255 */

int ntransmitted = 1;       /* Sequence # for outbound packets = #sent */
int ident ;                 /* ICMP identifier */
int is_fin = 0 ;            /* Termination-mode flag */
int fin_delay ;             /* Time to delay before final statistics */

int finish();
void terminate();
void catcher() ;
u_long resolve_name() ;
char *inet_ntoa();
u_short in_cksum();
char *pr_type() ;

        /* Compute  A -= B for timeval structs A, B
         */
#define tvsub(A, B)   (A)->tv_sec -= (B)->tv_sec ;\
         if (((A)->tv_usec -= (B)->tv_usec) < 0) {\
                  (A)->tv_sec-- ;\
                  (A)->tv_usec += 1000000 ; }


/*
 *          M A I N
 */
main(argc, argv)
int argc;
char *argv[];
{
    register int i, ch ;
    u_long options;
    char   tod_buf[26] ;

    while ((ch = getopt(argc, argv, "xd:t:b:n:e:uf:r:h:?")) != EOF)
      switch (ch)
      {
        case 'x':
            options |= SO_DEBUG;
            tell_all = 1 ;
            break ;

        case 'd':
            datalen = atoi(optarg);
            if( datalen > MAXPACKET)  {
                printf("ping:  packet size too big\n");
                exit(1);
            }
            totallen = datalen + ICMP_MINLEN;
            break ;

        case 't':
            interval = atoi(optarg) ;
            break ;

        case 'b':
            burst_size = atoi(optarg) ;
            break ;

        case 'n':
            send_limit = atoi(optarg) ;
            break ;

        case 'e':
            echo_host = 1 ;
            if (0 == strcmp(2+*argv, "raw")) RAW_echo = 1 ;
            break ;

        case 'u':
        case 'f':
        case 'r':
        case 'h':
             Get_Parm (ch,optarg);
             break;

        case '?':
        default :
             printf ("Usage: %s %s", argv[0], usage);
             exit (0);
      }

    argc -= optind;
    argv += optind;

    if (argc <= 0) {
        printf("Missing Target Host\n") ;
        exit(1) ;
    }

    if (burst_size > MAXBURST) {
        printf("Burst size max is %d\n", MAXBURST) ;
        return(1);
    }
    if (datalen < sizeof(struct timeval)) {
        printf("NOTE:  Data length reset to min = 8 bytes\n") ;
        datalen = sizeof(struct timeval) ;
    }

    Initialize(); /* Perform stat-specific initialization */

        /*
         *  Convert host name/number to IP address
         */
    target_host =  resolve_name( *argv, canhost, 64);
    if (target_host == 0)  {
        printf("ping: unknown host %s\n", *argv);
        return;
    }
    strcpy(hostaddr, inet_ntoa(target_host));
    target_sin.sin_addr.s_addr = target_host;
    target_sin.sin_family = AF_INET;

    gettimeofday(&startod, &tz) ;
    strcpy(tod_buf, ctime(&startod.tv_sec) ) ;

    printf("---PING Host= [%s]: %s echo %d data bytes\n %.24s %s\n",
         hostaddr, (RAW_echo)? "Raw": "ICMP", datalen,
         tod_buf, canhost);

    do_ping() ;

} /* main() */



do_ping()
{
    char packet_buff[MAXPACKET];
    int sinlen = sizeof(struct sockaddr_in);
    int i;
    u_char *datap;
    struct sockaddr_in from;
    struct itimerval itim, otim ; /* time interval structures */

        /*
         *  Use process ID as ICMP identifier
         */
    ident = getpid() & 0xFFFF;

        /*
         * Generate data in send packet buffer
         */
    datap = &outpack[ICMP_MINLEN+sizeof(struct timeval)];
    for( i= sizeof( struct timeval); i<datalen; i++)    /* skip time */
        *datap++ = i;

        /*
         * Set up interval timer structure
         */
    itim.it_interval.tv_sec = itim.it_value.tv_sec = interval/1000 ;
    itim.it_interval.tv_usec = itim.it_value.tv_usec =
                                              (interval%1000)*1000 ;

    if ((raw_sock = socket(AF_INET, SOCK_RAW,
                                     (RAW_echo)? 255 : IPPROTO_ICMP)) < 0)
    {
        perror("ping: socket");
        exit(1) ;
    }

        /*
         *  Read clock for start time and start ...
         */
    gettimeofday(&startod, &tz) ;
    sendtod = startod ;
    signal( SIGALRM, catcher );
    signal( SIGINT, terminate );
    setitimer(ITIMER_REAL, &itim, &otim) ; /* Start interval timer */

    for (;;) {
            /*
             * MAIN LOOP ... waiting to receive packets
             */
        int fromlen = sizeof(from);
        int cc;

         cc = recvfrom(raw_sock, packet_buff, sizeof(packet_buff), 0,
                       (struct sockaddr*)&from, &fromlen);
        if (cc < 0) {
            if( errno == EINTR )
                continue;
            perror("ping: recvfrom");
            continue;
        }
        else
            in_packet(packet_buff, cc, &from);
    }

} /* do_ping() */


/*
 *              C A T C H E R
 *
 *     Catch interval timer expiration.
 *
 * This routine causes another ping request (or burst) to be transmitted,
 * and schedules another SIGALRM .  However, if we are in finish-up mode,
 * just call finish() to print statistics.
 *
 */
void catcher()
{
    int i ;

    if (is_fin) {
        if (!finish() )   {
                /* It is possible (and in fact very likely, if interval is
                 * down near the minimum timer granularity, 20 ms) that the
                 * timer will go off and a SIGALRM will be queued while we
                 * are processing SIGINT.  There is
                 * no way (?) to kill the SIGALRM that immediately follows
                 * exit from SIGINT, even though the SIGINT processing
                 * did a new setitimer() call.  Therefore, finish() checks
                 * for a long-enough delay, and if not returns immediately
                 * the value 0.  In that case, we activate the timer alarm
                 * again and wait again for the real timeout.
                 */
            signal( SIGALRM, catcher );
        }
    }
    else {
        signal( SIGALRM, catcher );
        for (i=0; i< burst_size;i++) {
            if (send_limit-- > 0) {
                send_ping(raw_sock);
            }
            else {
                /* Reached limit on number to send. Pretend a ^C came */
                terminate() ;
                break ;
            }
        }
    }
} /* catcher() */


/*
 *          P I N G E R
 *
 * Compose and transmit an ICMP ECHO REQUEST packet.  The IP packet
 * will be added on by the kernel.  The ID field is our UNIX process ID,
 * and the sequence number is an ascending integer.  The first 8 bytes
 * of the data portion are used to hold a UNIX "timeval" struct in VAX
 * byte-order, to compute the round-trip time.
 *
 */

send_ping(s)
    int s;
    {
    register struct icmp *icp  ;
    int i, cc;
    struct timeval *tp = (struct timeval *) &outpack[ICMP_MINLEN];

    icp = (struct icmp *) outpack ;
    icp->icmp_type = (echo_host)? ICMP_ECHOREPLY : ICMP_ECHO;
    icp->icmp_code = 0;
    icp->icmp_cksum = 0;
    icp->icmp_seq = htons(ntransmitted++);
    icp->icmp_id = htons(ident);        /* ID */

    cc = datalen + ICMP_MINLEN ;        /* total ICMP packet length */
    gettimeofday( &sendtod, &tz );
    bcopy( &sendtod, tp, sizeof(struct timeval)) ;

    /* Compute ICMP checksum here */
//  icp->icmp_cksum = htons(in_cksum( icp, cc ));
    icp->icmp_cksum = in_cksum( icp, cc );

    /* cc = sendto(s, msg, len, flags, to, tolen) */
    i = sendto( s, outpack, cc, 0,
               (struct sockaddr*)&target_sin, sizeof(struct sockaddr_in) );

    if( i < 0 || i != cc )  {
        if( i<0 )  perror("sendto");
        printf("ping: wrote %d chars, ret=%d\n",
             cc, i );
        fflush(stdout);
    }

    Sent_ping();

} /* send_ping() */



/*
 *          I N _ P A C K E T
 *
 * Process incoming ICMP packet...
 *
 */
in_packet( buf, cc, from )
    char *buf ;                  /* Pointer to packet in buffer */
    int cc;                     /*  Length of packet in buffer */
    struct sockaddr_in *from;   /* Who sent it */
    {
    register struct icmp *icp;  /* Ptr to ICMP header */
    struct timeval *tp = (struct timeval *) &outpack[ICMP_MINLEN];
    int rtt, seqno;
    struct timeval tv;
    extern send_reply();

#ifndef SUNOS4
#ifdef sun
    if  (!RAW_echo)
        icp = (struct icmp *) buf ;
    else
#endif
#endif
        {   /* Point past IP header to ICMP header */
        int hlen ;
        struct ip *ip = (struct ip *) buf;

        hlen = ip->ip_hl << 2;
        cc -= hlen ;
        icp = (struct icmp *) (buf + hlen );
    }

    if( icp->icmp_type != ICMP_ECHOREPLY )  {

        if ( icp->icmp_type == ICMP_REDIRECT )  {
            printf("!Redirect to gateway: %s\n",
                     inet_ntoa(icp->icmp_hun.ih_gwaddr )) ;
              /* should check that Redirect was for our packet */
            return ;
        }
            /* Unknown ICMP type.  Dump packet if -x was set. */
        if (tell_all) {
            printf("%d bytes from %s: ", cc,
                                     inet_ntoa( from->sin_addr.s_addr) );
            printf("icmp type=%d (%s) code=%d\n",
                icp->icmp_type, pr_type(icp->icmp_type), icp->icmp_code );
            hex_dump(icp, (cc < 48)? cc : 48) ;
        }
    }

    if( ntohs(icp->icmp_id)!= ident )
        return;         /* 'Twas not our ECHO */

    gettimeofday( &tv, &tz );

    tp = (struct timeval *) &icp->icmp_dun ;  /* pt to xmit-time in packet*/
    tvsub( &tv, tp );          /* RTT for this packet */
    rtt = tv.tv_sec*1000000 + tv.tv_usec;
    seqno = ntohs(icp->icmp_seq);

    Do_ping_reply(rtt, seqno);
} /* in_packet() */


/*
 *      T E R M I N A T E
 *
 *  User has entered ^C.  Set termination flag to inhibit further
 *  pings, and delay to await stragglers.
 *
 */
void terminate()
 {
    int delay  ;
    struct timezone tz ;
    struct itimerval itim, otim ; /* time interval structures */
    extern int tmax;  /* maximum RTT */

    gettimeofday(&SIGINTime, &tz) ;   /* time now... */

        /* Wait twice the max delay, or 20 seconds, whichever is
         * less.
         */
    delay = (2*tmax + 500)/1000 ;
    itim.it_value.tv_sec =  (delay > 20) ?  20 : (delay < 2) ? 1 : delay ;
    itim.it_value.tv_usec =  0 ;
    itim.it_interval.tv_sec = itim.it_interval.tv_usec = 0 ;
                                           /* then disable*/
    setitimer(ITIMER_REAL, &itim, &otim) ; /* Start interval timer */
    is_fin = 1 ;  /* Enter finish-up mode. */

    printf("... Delay %d seconds for stragglers...\n", itim.it_value.tv_sec) ;
    fin_delay = itim.it_value.tv_sec * 1000 ;
    fflush(stdout) ;

        /* Another control C and we will quit anyway... */
    signal( SIGINT, (void(*)(int))finish );

}


int finish()
{
    int actual_delay ;
    struct timezone tz ;
    struct timeval fintime;

    signal( SIGINT, SIG_DFL );  /* Restore default action */

        /* Determine the actual delay (to let stragglers show up), and if
         *   it is too short (due to spurious clock interrupt -- see the
         *   comment in catcher()), just return to await the real timeout.
         */
    gettimeofday(&fintime, &tz) ;
    tvsub(&fintime, &SIGINTime) ;
    actual_delay = fintime.tv_sec*1000 + fintime.tv_usec/1000  ;

       /* If this was spurious timeout, return 0 for retry. */
    if (actual_delay < fin_delay - 20)
        return(0) ;

    Final_summary();

    fflush(stdout);
    exit(0);
}


hex_dump(p, len)
register char *p ;
register int len ;
    {
    register int i = 0, j ;
    u_long  temp ;

    while (len > 0) {
        printf("x%2.2x: ", i );
        for (j = 0; j<4; j++)  {
            if (len > 4) {
#ifdef vax
                temp = * (u_long *) p ;
                printf(" x%8.8x", ntohl( temp )) ;
#else
                printf(" x%8.8x", * (long *) p) ;
#endif
                p += sizeof(long) ;
                i += sizeof(long) ;
                len -= sizeof(long) ;
            }
            else {
                printf(" x%*.*x",  2*len, 2*len, * (long *) p) ;
                len = 0 ;
                break ;
            }
        }
        printf("\n") ;
    }
}


/*
 *          I N _ C K S U M
 *
 * Checksum routine for Internet Protocol family headers (C Version)
 *   (based upon a version written by Mike Muuss, BRL)
 */
u_short
in_cksum(addr, len)
u_short *addr;
int len;
    {
    register int nleft = len;
    register u_short *w = addr;
    register int sum = 0;

    /*
     *  Our algorithm is simple, using a 32 bit accumulator (sum),
     *  we add sequential 16 bit words to it, and at the end, fold
     *  back all the carry bits from the top 16 bits into the lower
     *  16 bits.
     */
    while( nleft > 1 )  {
        sum += *w++;
        nleft -= 2;
    }

    /* mop up an odd byte, if necessary */
    if( nleft == 1 )
        sum += *(u_char *)w;

    /*
     * add back carry outs from top 16 bits to low 16 bits
     */
    while (sum>>16)
        sum = (sum & 0xffff) + (sum >> 16);     /* add hi 16 to low 16 */

    return ( (~sum) & 0xFFFF);
}



/*
 *          P R _ T Y P E
 *
 * Convert an ICMP "type" field to a printable string.
 */
char *
pr_type( t )
register int t;
{
    static char *ttab[] = {
        "Echo Reply",
        "ICMP 1",
        "ICMP 2",
        "Dest Unreachable",
        "Source Quence",
        "Redirect",
        "ICMP 6",
        "ICMP 7",
        "Echo",
        "ICMP 9",
        "ICMP 10",
        "Time Exceeded",
        "Parameter Problem",
        "Timestamp",
        "Timestamp Reply",
        "Info Request",
        "Info Reply"
    };

    if( t < 0 || t > 16 )
        return("OUT-OF-RANGE");

    return(ttab[t]);
}


  /* Host name lookup...
   *
   *   Resolve host name using domain resolver or whatever, and copy
   *   canonical host name into canonp[canonl].  But if
   *   argument is dotted-decimal string (first char is digit), just
   *   convert it without lookup (we don't want to fail just because
   *   the INADDR database is not complete).
   *
   */
u_long resolve_name( namep, canonp, canonl )
 char *namep, canonp[] ;
 int  canonl ;
    {
    struct hostent *hp ;
    u_long inetaddr ;
    int n ;

    if (isdigit(*namep)) {
            /* Assume dotted-decimal */
        inetaddr = (u_long) inet_addr(namep) ;
        *canonp = '\0' ;   /* No canonical name */
        return(inetaddr) ;
      }
    else  {
        if (NULL == (hp =  gethostbyname( namep )))
            return(0) ;
        n = ((n = strlen(hp->h_name)) >= canonl) ?  canonl-1 : n ;
        bcopy(hp->h_name, canonp, n) ;
        hp->h_name[n] = '\0' ;

        return( *( u_long *) hp->h_addr) ;
    }
}
