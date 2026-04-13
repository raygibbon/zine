

/*
 *        ring_stat.c  --  Gather and Display Ping Statistics
 * 
 *       Get_Parm()
 *       Initialize()
 *       Sent_ping()
 *       Do_ping_reply(triptime, seqno)
 *       Final_summary()
 *
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <ctype.h>
#include <sys/param.h>

#include "histogra.h"

#define LOSS_LIM  5    /* Number of successive lost packets => black hole*/
#define MSPT 20        /* Clock resolution (milliseconds per tick) */
#define PRINT_spasm  5 /* Max spasm of consec pings to print with -r flag */
#define MAXBURST 20    /* Max value of -b parameter */

extern int errno;
extern struct timezone tz;      /* time zone */
extern struct timeval startod, sendtod ; /* start time, last-send time */ 
extern int fin_delay;      /* Time to delay before final statistics */

extern int  totallen;
extern int  ntransmitted;  /* Sequence # for next outbound packet = #sent */
extern char hostaddr[16];
extern char canhost[64] ;  /* canonical host name */
extern int  burst_size;    /* Number to send in each burst (default = 1) */
extern int  interval;      /* Pinging interval in ms. */
extern int  send_limit;    /* Limit on number to send */

int RTT_thresh = 0;        /* Only report if RTT exceeds this number */
int hist_depth = 35;       /* Number of lines to use in histogram display */
int Report_freq = 0;       /* Reporting frequency (-f value) */
int RTT_is_usec = 0;       /* parm -u =>report  RTT in micro secs, not ms. */

int loss_limit;            /* Number of lost packets => black hole */
int black_hole = -1;       /* Start time (secs) of black hole, or zero. */
int blackholed = 0;        /* Count of packet sent into black hole */
int bholetime = 0;         /* Total time in seconds for black holes */
int nreceived = 0;         /* Total # of packets we got back */
int nmangled = 0;          /* Count mangled packets */
int HWM_rcv_seq  = 65535;  /* High Water Mark of received sequence #s*/
int Sample_freq = 0;       /* Sampling frequency  */

   /* Following variables need to be re-initialized after a black hole...
    */
int last_Cum_loss = 0 ;
int last_secs = -1 ;       /* Elapsed seconds at last display */
int spasm_tally = 0 ;      /* Used to control max printing freq. */
int print_ellide = 0 ;      /* Boolean: need ... */

char   time_buf[10] ; 
struct timeval  SIGINTime;  
HCB *sumhcbp;   /* Histogram pointer */

    /* Burst statistics */ 
/***
int btmin[MAXBURST], btmax[MAXBURST], btsum[BURST], bnrecvd[MAXBURST];
***/
    
int tmin ;                   /* Min RTT */
int tmax ;                   /* Max RTT */
int tsum ;          /* sum of all RTTs, for average */

          
        /* Compute  A -= B for timeval structs A, B
         */
#define tvsub(A, B)   (A)->tv_sec -= (B)->tv_sec ;\
         if (((A)->tv_usec -= (B)->tv_usec) < 0) {\
                  (A)->tv_sec-- ;\
                  (A)->tv_usec += 1000000 ; }

/*
 *   Parse those invocation parameters that control the statistics
 *      functions in this module.  Return 1 if parm recognized, 0
 *      otherwise.
 */
int
Get_Parm(int ch, char *arg)
    {
    switch (ch) {
            
        case 'u':
            RTT_is_usec = 1;
            break;
            
        case 'f':
            Report_freq = atoi(arg);
            break ;
            
        case 'r':
            RTT_thresh = atoi(arg);
            break ;
            
        case 'h':
            hist_depth = atoi(arg);
            break ;

        default:
            return(0);
        }
    return(1);
} /* Get_Parm() */

        
Initialize()
    {
    HCB *init_histogram();
    int i; 

#ifdef WATT32
    extern int tell_all;
    if (tell_all)
       dbug_init();
    sock_init();
#endif
        /*
         *  Validate parameters...
         */
    if (interval < MSPT && !RTT_is_usec)  {
        printf("Interval is %d ms\n", MSPT) ;
        interval = MSPT ;
    }
    if (Report_freq && Report_freq*interval < 1000) {
        printf("Specified report frequency too short\n") ;
        Report_freq = 1000/interval ;
    }
    loss_limit = (interval < 2000)? (2000*LOSS_LIM)/interval : LOSS_LIM ;
    Sample_freq = burst_size *(1000/interval) ; 
      /* how many per second  ?? */

         /* Define a histogram area for 300 entries. Each bin is 100 ms,
          * so RTTs beyond  30 seconds are lumped into highest bin.
          */
    if ((sumhcbp = init_histogram(100, 0, 300, "PINGER")) == NULL) {
        printf("No space for histogram\n");
    }

/***
    for (i = 0;i < MAXBURST; i++) {
        btmax[i] = btsum[i] = bnrecvd[i] =  0;
        btmin[i] = 99999999 ;
        }
***/
    tmin = 99999999 ;
    tmax = tsum = 0 ;
    init_globals() ;
    setlinebuf( stdout );      
} /* Initialize() */




/*
 *  Sent_ping() :  called after ping has been sent.
 *
 */
Sent_ping()
    {
    int i, outstand;

       /* Test for a black hole...  This test is delicate.  We must 
        *   consider the number of packets which may be in the pipeline,
        *   which depends upon the RTT and the interval.  To bound the
        *   RTT, we could use the largest RTT we have seen so far...
        *   but a single spuriously large RTT could upset that completely.
        *   So we use min(2*avg-RTT, max-RTT).
        */ 
    if (nreceived-nmangled > 0) { 
        i = 2*tsum/(nreceived - nmangled) ;
        outstand = ((i > tmax) ? tmax : i)/interval ;
    }
    else 
        outstand = tmax/interval ;
        
    i = (ntransmitted & 0xFFFF)   /*  modulo 2^16 seq field    */
            -  HWM_rcv_seq  - 2;  /* = Gap since last reply... */
    if (i < -2)  i += 65536 ;     /*  Adjust for rollover.    */
    
    if ((black_hole < 0) &&  (i > (loss_limit + outstand)) )
        {  /* We have a black hole; compute: 
            *      black_hole = secs since beginning; max err is 
            *                 +/- one packet.
            */
        tvsub(&sendtod, &startod ) ;
        black_hole = sendtod.tv_sec - (i*interval)/(1000*burst_size);
        if (sendtod.tv_usec > 500000) black_hole++;
        format_time(black_hole);             
        printf("[+%s]  Started BLACK HOLE of %d+ packets...\n", time_buf, i) ;
        }
} /* Sent_ping() */

/*
 *  do_ping_reply(triptime, seqno)
 *     Trip time in usec.
 *
 */
Do_ping_reply(triptime, seqno)
    int triptime, seqno;
    { 
    int p, mangled = 0 ;
    int now_secs, Cum_loss;
    struct timeval elapse_tv;

    if (!RTT_is_usec)
        triptime = (triptime+999)/1000;
    histogram(sumhcbp, triptime) ;
      
    nreceived++;
    Cum_loss = seqno  - nreceived ;

    gettimeofday( &elapse_tv, &tz );
    tvsub(&elapse_tv, &startod) ;   /* Elapsed since start of run */
    now_secs = elapse_tv.tv_sec ;
    if (elapse_tv.tv_usec > 500000 ) now_secs++ ;

    if (black_hole >= 0) {
           /* End of a black hole.... */
           
        blackholed +=  (0 < (p = seqno - HWM_rcv_seq)) ? p : p + 65536 ;
        format_time(now_secs) ;         
        printf("[+%s]  ECHOING AGAIN after %d seconds..\n", time_buf, 
                        now_secs - black_hole) ;
        bholetime += now_secs - black_hole ;
        black_hole = -1 ;
        init_globals() ;  /* re-initialize control variables */
        
    } 
           
    tsum += triptime;
    if( triptime < tmin )  tmin = triptime;
    if( triptime > tmax )  tmax = triptime;
    
/***
    if (burst_size != 1) {
        p = (seqno - 1) %  burst_size ;
        btsum[p] += triptime ;
        if (triptime < btmin[p]) btmin[p] = triptime ;
        if (triptime > btmax[p]) btmax[p] = triptime ;
        ++bnrecvd[p] ;
    }
***/
            /* Get high-water-mark of sequence number, modulo 2^16 */
    if (seqno > HWM_rcv_seq|| seqno < (HWM_rcv_seq - 32767)) 
        HWM_rcv_seq = seqno ;
                
    if (Report_freq > 0)   {
        if  ((seqno %Report_freq) == 0) 
            {  /* Time for a report (summary) */
            last_secs = now_secs ;
            format_time(now_secs) ;
            printf("[+%s]", time_buf) ;
            print_summary() ;
        }
    }           
    else if  ( RTT_thresh == 0 &&
          ( Sample_freq == 0 || ((seqno-1) %Sample_freq) == 0 ) ) {
          
            /* Normal case... 
             * print no more than approx one sample per sec...
             */ 
         last_Cum_loss = Cum_loss ;    
         format_time(last_secs = now_secs ) ;      
         format_echo_line(seqno, triptime, Cum_loss) ;
    }     
     
    else if  ( RTT_thresh > 0 &&
        ( triptime >= RTT_thresh || Cum_loss != last_Cum_loss || mangled)) {
        
             /* Threshold (-r) parameter given.  Suppress printout unless
              *     something bad happens...  Even then, print at most
              *     PRINT_burst times in one second, and an average of
              *     1 per second over each PRINT_burst seconds.
              */
        spasm_tally -= (now_secs - last_secs) ;
        if (spasm_tally < 0) spasm_tally = 0 ;
        if (spasm_tally < PRINT_spasm) {
            spasm_tally++ ;
            if (print_ellide) {  /* Begin new line with "..." to
                                     show we omitted some previously */
                print_ellide = 0 ;
                printf(" ...\n") ;
            }
            last_Cum_loss = Cum_loss ;
            format_time(last_secs = now_secs ) ;       
            format_echo_line(seqno, triptime, Cum_loss) ;
        }
        else print_ellide ++ ;
    } ;    
            
    fflush(stdout);
} /* Do_ping_reply() */


Final_summary()
    {
    int elapse, i;
    struct timeval fintime1;
    char   tod_buf[26];
    extern histdisplay(); 

    tvsub(&sendtod, &startod) ;
    elapse = sendtod.tv_sec * 1000 + (sendtod.tv_usec / 1000) ;
    gettimeofday(&fintime1, &tz);

    strcpy(tod_buf, ctime(&fintime1.tv_sec) ) ;
    format_time( (elapse+999)/1000 ) ;
    printf(
  "\n %.24s  %s\n---PING Stats--- Host= [%s]   %d byte packets  Elapsed= %s\n",
        tod_buf, canhost, 
        hostaddr, totallen, time_buf) ;
          
    print_summary() ;   
    
/***
    if (nreceived && burst_size > 1) {
        printf("     By position in burst: \n") ;
        for (i=0;i < burst_size; i++)  {
            printf("[+%d]: %d packets received,", i, bnrecvd[i]);
            if (bnrecvd[i])
                printf((RTT_is_usec)? "RTT= %dMin %dAvg %dMax us\n":
                                      "RTT= %dMin %dAvg %dMax ms\n",
                        btmin[i], btsum[i]/bnrecvd[i], btmax[i] ) ;
            else
                printf("\n") ;
        }
    }
***/
    
    printf("\n") ;
    if (hist_depth > 0 && nreceived)
        histdisplay( sumhcbp, (RTT_is_usec)?
                        "(us)| Count - HISTOGRAM : \n":
                        "(ms)| Count - HISTOGRAM : \n",
                 60, hist_depth, stdout);
} /* Final_summary() */ 



format_echo_line(seq,  rtt, loss)
int  seq, rtt, loss ;
    {                   
    printf("[+%s] Echo Reply: Seq=%-3d", time_buf, seq );
    if (RTT_is_usec)
            printf(" CumLoss=%d  RTT=%d us\n", loss, rtt) ;
    else
            printf(" CumLoss=%d  RTT=%d ms\n", loss, rtt) ;
}   
 

print_summary() 
    {
    register nt = ntransmitted - 1 - blackholed ;
    
    printf(" Sent=%4d  CumLoss= %d", nt, nt-nreceived );
    
    if (nt != nreceived)  
        printf(" (%.1f%%)", 100*  (1.0 - (float) nreceived/nt ) );
                
    if (blackholed)
        printf("+%d lost in black hole(s) over %d secs\n  ",
             blackholed, bholetime) ;
                    
    if( nreceived ) {
        printf((RTT_is_usec)? "  RTT: %dMin %dAvg %dMax us" :
                              "  RTT: %dMin %dAvg %dMax ms",
        tmin,
        tsum / (nreceived-nmangled),
        tmax );
    }
    printf("\n") ;
}


format_time(t)
int t ;  /* time in seconds */
    {

    if (t < 1000)
        sprintf(time_buf, "%d Secs", t) ;
    else
        sprintf(time_buf, "%02d:%02d:%02d", t/3600, (t/60)%60, t%60) ;
}

init_globals() {
   /* These variables need to be re-initialized after a black hole...
    */
    last_Cum_loss = 0 ;
    last_secs = -1 ;        /* Elapsed seconds at last display */
    spasm_tally = 0 ;       /* Used to control max printing freq. */
    print_ellide = 0 ;   
}
 

