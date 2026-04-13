    /*
     *  Developers - please don't assume everything here has meaning.  There
     *               are some things which are carryovers from early versions
     *               of WATTCP.  For example, this application need not
     *               switch between text and ascii mode, but it does it due
     *               to an ancient (and long fixed) bug in WATTCP.
     *
     *  Changes & cleanup by G.Vanem Feb 2000
     */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <conio.h>
#include <tcp.h>

#if defined(__WATCOMC__) || defined(__HIGHC__) || defined(__DJGPP__)
#include <unistd.h>
#endif

    /* this is a stupid limit, DOS file handles should be increased using
     * the newer DOS calls.  But we should also test to ensure BC handles
     * extra files correctly.  A good project for another day.
     *
     * SMTPSERV gets the number of concurrent sessions from DEFAULTSMTPSES,
     * but accepts an override value in WATTCP.CFG.
     */
#define DEFAULTSMTPSES 8

#define SEQFILE        "MAILFILE.SEQ"
#define SMTPPORT       25

    /* server developers - I used to design WATTCP based servers around a
     * four state machine.  The machine can go like this:
     *
     *     S_INIT ---> S_WAIT ---> S_CONN
     *       /|\          |           |
     *        |          \|/          |
     *        +------  S_CLOS  <------+
     *
     * I have a newer philosophy which is a little less confusing and I hope
     * to soon push, but I haven't yet had a chance to test it as extensively.
     * The system presented here is a little blocky, but it is used
     * successfully 24 h a day to receive an average of 1000 to 1500 messages
     * per day from everywhere on the internet.
     */

#define S_INIT  0
#define S_WAIT  1
#define S_CONN  2   /* connected and talking */
#define S_CLOS  3

    /* GOT_* - used to determine what we have and so what we should
     *         say when we get absurd input
     */

#define GOT_SENDER   1
#define GOT_RECEIVER 2
#define GOT_DATA     3

    /*
     * SMTP files, the text constants are used when parsing the config file
     */

#define SMTPFILELEN 80          /* length of possible filename */
#define SMTPDIR  "smtp.subdir"
#define SMTPSES  "smtp.sessions"

    /*
     * SMTP commands - some of these contain spaces
     */

#define HELO     "HELO"
#define HELP     "HELP"
#define NOOP     "NOOP"
#define MAILFROM "MAIL FROM:"
#define RCPTTO   "RCPT TO:"
#define DATA     "DATA"
#define RSET     "RSET"
#define QUIT     "QUIT"
#define STAT     "STAT"

    /*
     * SMTP replies - the numeric values are important
     */

#define SMTP_OK         "250 OK\r\n"
#define SMTP_RESET      "250 Reset state\r\n"
#define SMTP_INT_OK     "354 Start mail input; end with <CRLF>.<CRLF>\r\n"
#define SMTP_SENT       "250 Mail accepted\r\n"
#define SMTP_NOT_SENT   "421 Something got baked, mail discarded\r\n"

#define SMTP_OVERLOAD   "421 System overloaded, try again later\r\n"
#define SMTP_MISS_NAME  "501 Missing <name>\r\n"
#define SMTP_GOT_SENDER "501 Already have a sender\r\n"
#define SMTP_NEED_WHO   "501 Needs a sender first\r\n"
#define SMTP_CLOSING    "251 Closing\r\n"
#define SMTP_DUNNO      "500 Command not recognized\r\n"

    /*
     * smtpmsg - the entire data for each incoming session is stored
     *           in this structure, including the tcp_Socket stucture.
     *           Mail received during the session uses this structure for
     *           temporary data such as filenames and file handles.
     */

typedef struct {
    int         status;
    int         got;
    int         datamode;       /* 1 if true */
    char        hisname[ SMTPFILELEN ];
    tcp_Socket  socket;
    FILE        *cmdfile;
    FILE        *datafile;
    longword    messageid;      /* must be non-zero to be valid */
} smtpmsg;

int smtpsessions = DEFAULTSMTPSES;
smtpmsg **smtp;


/* buffers used for filenames */
char buffer1[ 128 ], buffer2[ 1024 ];
char subdir[128];
char messagebuf[ 1024 ];


    /*
     * read config file
     */

void (*oldinit)( const char *directive, const char *value );

void newinit( const char *directive, const char *value )
{
    if (!stricmp( directive, SMTPDIR )) {
        strncpy( subdir, value, sizeof( subdir )-5 );
        subdir[ sizeof( subdir) - 5] = 0;
        printf("NOTE: using '%s' subdirectory\n", subdir );
        strcat( subdir, "\\" );
    } else if (!stricmp( directive, SMTPSES )) {
        smtpsessions = atoi( value );
    } else
        if (oldinit) (*oldinit)( directive, value );
}

    /*
     * showstats - I added a special command to help me debug the system
     */

static void showstat( smtpmsg *s )
{
    WORD days, inactive, cwindow, avg, sd;

    sock_stats( &s->socket, &days, &inactive, &cwindow, &avg, &sd );
    sock_printf( &s->socket,"250-Stats\r\n   - %u days\r\n   - %u inactive seconds\r\n", days, inactive);
    sock_printf( &s->socket,"   - %u is cwindow\r\n   - %u avg  %u sd\r\n250 Ready\r\n", cwindow, avg, sd);
}


    /*
     * getsequence - gets the current/unique mail id number
     *             - we don't allow sequence numbers of 0
     */
longword getsequence (void)
{
    static longword localseq = 0;  /* remember sequence number */
    struct stat statbuf;
    FILE  *f;

    do {
        if (!localseq) {
            if (( f = fopen( SEQFILE, "rt" )) != NULL) {
                fgets( buffer1, sizeof( buffer1 ), f );
                fclose( f );
                localseq = atol( buffer1 ) + 1;
                printf("NOTE: Initial sequence number using %lu\n", localseq );
            } else
                /* failure openning */
                printf("Unable to open SEQ file - assuming new installation\n"
                       "If not, reboot computer!!!! 7\7\7\n");
        }
        sprintf( buffer1, "%s%lu.CMD", subdir, ++localseq );
    } while ( !stat( buffer1, &statbuf ));

    if (( f = fopen( SEQFILE, "w+t" )) != NULL) {
        fprintf( f, "%lu\n", localseq );
        fclose( f );
    }
    return( localseq );
}

    /*
     * opendatafiles - called once for each message
     *               - cmd is nonzero for command, zero for data
     *               - returns 1 on success
     */

int opendatafiles( smtpmsg *s, int cmd )
{
    if ( s->cmdfile ) fclose( s->cmdfile );
    if ( s->datafile ) fclose( s->datafile );

    s->cmdfile = s->datafile = NULL;

    if ( cmd ) {
        /* get a non-zero seq number */
        s->messageid = getsequence();
        sprintf( buffer1, "%s%lu.CMD", subdir, s->messageid );
        if ((s->cmdfile = fopen( buffer1,"wt")) == NULL) {
            printf("ERR: could not open '%s'\n", buffer1 );
            return( 0 );
        }
    } else {
        sprintf( buffer1, "%s%lu.TMP", subdir, s->messageid );
        if ((s->datafile = fopen( buffer1, "wt")) == NULL) {
            printf("ERR: could not open '%s'\n", buffer1 );
            sprintf( buffer1, "%s%lu.CMD", subdir, s->messageid );
            unlink( buffer1 );
            return( 0 );
        }
    }
    return( 1 );
}

    /*
     * donemsg - returns 1 if totally successful
     *         - pass success = 1 if everything successful up 'til now
     */

int donemsg( smtpmsg *s, int success )
{
    int status;

    status = 0;


    /* generally clean up these things */
    if (s->datafile) status = fclose( s->datafile );
    if (s->cmdfile)  status |= fclose( s->cmdfile );

    if (success && status) puts("Error closing one of the files");

    s->datafile = s->cmdfile = NULL;

    sprintf(buffer1, "%s%lu.TMP", subdir, s->messageid );
    sprintf(buffer2, "%s%lu.TXT", subdir, s->messageid );
    if (success && !status ) status = rename( buffer1, buffer2 );
    else unlink( buffer1 );

    if (success && status ) {
        success = 0;
        unlink( buffer1 );
    }

    sprintf(buffer1, "%s%lu.CMD", subdir, s->messageid );
    sprintf(buffer2, "%s%lu.WRK", subdir, s->messageid );
    if (success && !status) status = rename( buffer1, buffer2 );
    else unlink( buffer1 );

    if (success && status )
        unlink( buffer1 );

    s->got = 0;


    if ( success && !status ) return( 1 );
    else return( 0 );
}

    /*
     * shutdown - given a socket and a reason, shut down the socket and files
     *            and clean up the files if necessary
     */
void shutdown( smtpmsg *s, char *reason )
{
    printf("SHUTDOWN : %s\n", reason );
    sock_close( &s->socket );
    if (s->cmdfile || s->datafile)
        donemsg( s, 0);
    s->status = S_CLOS;
}
    /*
     * chkresource - used when someone wants to send a new message
     *             - check to see we have spare file handle and get a new
     *               sequence number for it
     *             - return 1 on bad error
     */
int chkresource( smtpmsg *s, int cmd )
{
    if ((cmd && ! s->cmdfile ) || (!cmd && !s->datafile)) {
        /* try to allocate some new resources */
        if (!opendatafiles( s , cmd )) {
            sock_puts( &s->socket, SMTP_OVERLOAD );
            shutdown( s, "OPENDATAFILES" );
            return( 1 );
        }
    }
    return( 0 );
}

    /*
     * printhelp - display available commands
     */
void printhelp( tcp_Socket *t )
{
    sock_puts( t, "250-HELP - list of command accepted by WATTCP mail daemon\r\n");
    sock_puts( t, "250-DATA    HELO    HELP    MAIL    NOOP    QUIT    RCPT    RSET\r\n");
    sock_puts( t, "250 OK\r\n");
}

    /*
     * dumpreceived - about to receive a message, so fill in the
     *                initial data indicating when and from whom
     *                we received it
     */
void dumpreceived( smtpmsg *s )
{
    struct watt_sockaddr sock;
    struct tm *tm;
    time_t now;
    int    len;
    char *months[] = { "Jan","Feb","Mar","Apr","May","Jun","Jul","Aug",
            "Sep","Oct","Nov","Dec"};
    char *days[] = { "Sun","Mon","Tue","Wed","Thr","Fri","Sat" };

    time( &now );

    tm = localtime( &now );

    len = sizeof(sock);
    _getpeername( &s->socket, &sock, &len );
    sprintf( s->hisname, "[%s:%u]",
            _inet_ntoa( buffer1, sock.s_ip ),sock.s_port);

    if (gethostname( buffer2, sizeof(buffer2)) < 0)
        sprintf( buffer2, "[%s]", _inet_ntoa( buffer1, _gethostid()));

    fprintf( s->datafile, "Received: from %s by %s with SMTP\n",
        s->hisname, buffer2 );
    fprintf( s->datafile, "        id A%lu ; %s, %u %s %u %0u:%02u:%02u %s\n",
        s->messageid, days[ tm->tm_wday ], tm->tm_mday, months[tm->tm_mon],
        tm->tm_year % 100, tm->tm_hour, tm->tm_min, tm->tm_sec,
#ifdef __TURBOC__
        tzname[ daylight ]);
#else
        tzname[ 0 ]);   /* to-do */
#endif

}
    /*
     * getvalue - find the <xxx> value in a string
     */
void *getvalue( char *p )
{
    char *ptr;

    /* kill end '>' */
    if (( ptr = strchr( p, '>')) != NULL) {
        *ptr = 0;
        /* find first '<' */
        if (( ptr = strchr( p, '<' )) != NULL)
            return( ++ptr );
    }
    return( NULL );
}

    /*
     * cmdcompare - return 1 if p matches the SMTP command
     */
int cmdcompare( char *p, char *command )
{
    return( !strnicmp( p, command, strlen( command )));
}


    /*
     * parseline - parse the command line stuff
     *           - only called if NOT in data mode
     *           - char *p is the current line
     */
void parseline( smtpmsg *s, char *p )
{
    char *temp;
    tcp_Socket *sock;

    sock = &s->socket;

    if ( !*p ) {
        /* empty - do nothing */
    } else if ( cmdcompare( p , HELO )) {
        /* not too sure what to do, skip right now */
        if ((temp = strchr( p, ' ')) != NULL) {
            while ( *temp == ' ') ++temp;
            if (*temp) {
                strncpy( s->hisname, temp, SMTPFILELEN - 1 );
                s->hisname[ SMTPFILELEN - 1 ] = 0;
            }
        }
        sock_puts( sock, SMTP_OK );

    } else if ( cmdcompare( p , NOOP )) {
        sock_puts( sock, SMTP_OK);

    } else if ( cmdcompare( p , MAILFROM )) {
        if ( chkresource( s, 1 )) return;
        /* check to see if we already have a sender */
        if ( s->got & GOT_SENDER )
            sock_puts( sock, SMTP_GOT_SENDER ); /* already have one */
        else {
            if (( temp = getvalue( p )) != NULL) {
                s->got |= GOT_SENDER;
                if (fprintf( s->cmdfile, "From: %s\n", temp ) != EOF )
                    sock_puts( sock, SMTP_OK );
                else {
                    sock_puts( sock, SMTP_OVERLOAD );
                    shutdown( s, "FROMWRITEFAILED" );
                }
            } else
                sock_puts( sock, SMTP_MISS_NAME );
        }

    } else if ( cmdcompare( p, RCPTTO )) {
        if ( chkresource( s, 1 )) return;
        if ( !(s->got & GOT_SENDER ))
            sock_puts( sock, SMTP_NEED_WHO );
        else {
            if (( temp = getvalue( p )) != NULL) {
                s->got |= GOT_RECEIVER;
                if (fprintf( s->cmdfile, "To: %s\n", temp ) != EOF )
                    sock_puts( sock, SMTP_OK );
                else {
                    sock_puts( sock, SMTP_OVERLOAD );
                    shutdown( s, "TOWRITEFAILED");
                }
            } else
                sock_puts( sock, SMTP_MISS_NAME );
        }

    } else if ( cmdcompare( p, DATA)) {
        if ( chkresource( s, 0 )) return;
        if ( s->got != ( GOT_SENDER | GOT_RECEIVER ))
            sock_printf(sock, "501 Missing %s%s%s\r\n",
                (!(s->got & GOT_SENDER))?"sender name":"",
                (!(s->got & (GOT_RECEIVER || GOT_SENDER)))?" and ":"",
                (!(s->got & GOT_RECEIVER ))?"receiver name":"");
        else {
            dumpreceived( s );
            
            /* say sure, accepting data */
            sock_puts( sock, SMTP_INT_OK );
            s->datamode = 1;      /* place into data mode */
        }
    } else if ( cmdcompare( p, QUIT )) {
        sock_puts( sock, SMTP_CLOSING );
        shutdown( s, "<OK>");

    } else if ( cmdcompare( p, RSET )) {
        donemsg( s, 0 );
        s->got = 0;
        sock_puts( sock, SMTP_RESET );
    } else if ( cmdcompare( p, HELP )) {
        printhelp( sock );
    } else if ( cmdcompare( p, STAT )) {
        showstat( s );
    } else {
        sock_puts( sock, SMTP_DUNNO );
    }
}

    /*
     * parsedata - called whenever new data arrives and we are in data mode
     *           - attempt to save it to a file and handle end of message
     */

void parsedata( smtpmsg *s, char * line )
{
    /* check line for '.' */
    if ( line[0] == '.' &&
         line[1] == 0 ) {

        /* only save files if they were complete */
        if (donemsg( s, (s->datamode==1)? 1 : 0))
            sock_puts( &s->socket, SMTP_SENT );
        else
            sock_puts( &s->socket, SMTP_NOT_SENT );
        s->messageid = 0;
        s->datamode = 0;
    } else {
        /* keep writing as long as we can */
        if (s->datamode == 1 &&
            fprintf( s->datafile,"%s\n", line ) == EOF )
           s->datamode = 2;
    }
}

    /*
     * printwelcome - display the openning banner
     */
void printwelcome( tcp_Socket *t)
{
    char name[128], *domain;

    if (gethostname( name, sizeof(name))) {
        domain = strchr( name, 0 );
        if (getdomainname( domain+1, sizeof(name) - strlen(name) - 2))
            *domain = '.';
    } else
        strcpy( name, "WATTCP" );

    sock_printf( t, "220-%s SMTP service\r\n", name ? name : "WATTCP" );
    sock_puts( t, "220-bugs to erick@development.uwaterloo.ca or erick@sunee.uwaterloo.ca\r\n");
    sock_puts( t, "220-version March 29, 1992\r\n");
    sock_puts( t, "220 Ready.\r\n");
    sock_flush( t );

    sock_mode( t, TCP_MODE_ASCII );
}


    /*
     * SOME DEBUG STUFF TO HELP ME KEEP TRACK OF THE SYSTEM USAGE
     */
#ifdef DEBUG
long  lastreceived = 0;
DWORD lastdebug = 0;
#endif /* DEBUG */

void smtpd_tick (void)
{
    int sess, count, i, cursessions = 0;
    smtpmsg *s;
    tcp_Socket *t;

    for ( sess = 0 ; sess < smtpsessions ; ++sess )
        if ( smtp[sess]->status != S_INIT )
            cursessions++;

#ifdef OLD
    if ( ! cursessions )
        checkdisk();
#endif

    /*
     * this displays some stats every thirty seconds,
     */

#ifdef DEBUG
    if ( chk_timeout( lastdebug )) {
        for ( i = 0; i < smtpsessions ; ++i ) {
            s = smtp[ i ];
            t = &s->socket;
            printf(" %u : %u : %u : %s\n",
                i, s->status, t->undoc[4], sockstate( t ));
        }
        lastdebug = set_timeout( 30000 );

        if ( lastreceived && chk_timeout( lastreceived ))
            exit( 3 );
    }
#endif  /* DEBUG */

    /*
     * cycle through all the sessions rememberring they are state machines
     */

    for ( sess = 0 ; sess < smtpsessions ; ++sess ) {

        tcp_tick( NULL );

        /* work with this one so get some local variable names */
        s = smtp[ sess ];
        t = &s->socket;

        switch ( s->status ) {
            case S_INIT :
                        printf("INIT : session %u\n", sess );

                        /* not necessary, but used in my test version */
                        sock_abort( t );

                        /* clean out all data from previous sessions */
                        memset( s, 0, sizeof( smtpmsg ));

                        tcp_listen( t, SMTPPORT, 0L, 0, NULL, 0 );
                        s->status = S_WAIT;
                        break;

            case S_WAIT :
                        for ( count = i = 0; i < smtpsessions ; ++i )
                            if (smtp[i]->status != S_WAIT ) count++;
                        if ( sock_established( t )) {
                            printf("OPEN : session %u -- %u capacity\n",
                                sess, (++count*100)/smtpsessions);
                            printwelcome(t);
                            s->status = S_CONN;
                        }

                        if (!tcp_tick( t ))
                            s->status = S_CLOS;

                        break;

            case S_CONN :
                        if ( sock_dataready( t )) {
                            sock_gets( t, messagebuf, sizeof( messagebuf ));
                            sock_mode( t, TCP_MODE_BINARY );
                            if ( s->datamode )
                                parsedata( s, messagebuf );
                            else
                                parseline( s, messagebuf );
                            sock_mode( t, TCP_MODE_ASCII );

                        }
                        if (!tcp_tick( t )) {
                            sock_close( t );
                            shutdown( s, "OK" );
                            s->status = S_CLOS;
                        }
                        break;

            case S_CLOS :
                        sock_mode( t, TCP_MODE_BINARY );
                        if ( sock_dataready( t ))
                            sock_fastread( t, messagebuf, sizeof( messagebuf ));
                        if ( !tcp_tick( t ) ) {
                            if (s->cmdfile || s->datafile)
                                donemsg( s, 0);
                            for ( count = i = 0; i < smtpsessions; ++i )
                                if (smtp[i]->status != S_WAIT ) count++;
                            printf("CLOS : session %u --- %u capacity\n",
                                sess, (--count*100)/smtpsessions );

#ifndef __DJGPP__
                            /* this could interfere with other things */
                            if ( !sess )
                                fcloseall();
#endif

                            s->status = S_INIT;
                        }
                        break;
        }
    }
}



int main (int argc, char **argv)
{
    int i;

    *subdir = 0;
    oldinit = usr_init;
    usr_init = newinit;

    if (argc > 1 && !strnicmp(argv[1],"-d",2))
       dbug_init();
    sock_init();

    tzset();
    puts("SMTPSERV - E-mail gateway");
    puts("By Erick Engelke");
    puts("Copyright (c) 1991, University of Waterloo");
    printf("Working from %s\n", _inet_ntoa( messagebuf, _gethostid()));

    if ( (smtp = calloc( sizeof( void *), smtpsessions )) == NULL ) {
        printf("Insufficient memory to do anything");
        exit(3);
    }
    for ( i = 0 ; i < smtpsessions ; ++i ) {
        if ((smtp[i] = calloc( sizeof( smtpmsg ), 1 )) == NULL ) {
            printf("\7\7\7Could only open %i sessions... continuing", i );
            smtpsessions = i;
        }
    }

    while ( 1 ) {
        /* do the mail stuff */
        kbhit();
        smtpd_tick();
    }
    return (0);
}

#if defined(__BORLANDC__) || defined(__TURBOC__)
int stklen = 16536;
#endif
