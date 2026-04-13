
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef WATT32
#include <netinet/tcp.h>
#include <tcp.h>
#endif

class Request;

typedef unsigned char	uint8_t;
typedef unsigned short	uint16_t;
typedef unsigned int	uint32_t;

static int              debug = 0;
static int		nServers;
static sockaddr_in     *servers;

static uint16_t		idPool;
static Request         *allRequests;
static Request         *requests[65536];
static int		pendingRequests;
static int		totalRequests;
static int		packetsOut;
static int		packetsIn;

static int		chaussette;
static int		inputDone;
static char		line[100];

static inline void fatal (const char *msg, int noPerror = 0)
{
  if (noPerror)
     fprintf(stderr,"massdns: fatal: %s\n",msg);
  else
  {
    fprintf(stderr,"massdns: fatal: ");
    perror(msg);
  }
  exit(1);
}

static inline void warning (const char *format, ...)
{
  va_list arg;
  va_start(arg,format);
  fprintf(stderr,"massdns: warning: ");
  vfprintf(stderr,format,arg);
  fputc ('\n', stderr);
  va_end(arg);
}

class Request
{
public:
    Request(int*);
   ~Request();

    void	send();
    void	getIP(char*);

    uint16_t	getId()		{ return id;		}
    int		getSent()	{ return sent;		}
    time_t	getTimeStamp()	{ return timeStamp;	}
    Request	*getNext()	{ return next;		}
    Request	*getANext()	{ return aNext;		}

private:
    uint16_t	id;
    int		sent;
    time_t	timeStamp;
    uint8_t	ip[4];

    Request	*next;
    Request	*prev;

    Request	*aNext;
    Request	*aPrev;
};

Request::Request (int *a)
{
  sent= 0;
  id= idPool++;
  timeStamp= (time_t)-1;

  ip[0]= a[0];
  ip[1]= a[1];
  ip[2]= a[2];
  ip[3]= a[3];

  prev= 0;
  next= requests[id];
  if(next) next->prev= this;
  requests[id]= this;

  aPrev= 0;
  aNext= allRequests;
  if(aNext) aNext->aPrev= this;
  allRequests= this;

  ++pendingRequests;
  ++totalRequests;

  if (idPool==65535)
     fatal("cant process more than 65536 IP's at once yet.next release "
           "will fix that.",1);
}

Request::~Request()
{
  if (next)
     next->prev= prev;
  if (prev)
       prev->next= next;
  else requests[id]= next;

  if (aNext)
     aNext->aPrev= aPrev;
  if (aPrev)
       aPrev->aNext= aNext;
  else allRequests= aNext;

  --pendingRequests;
}

void Request::getIP (char *buf)
{
  sprintf (buf, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
}

static inline uint8_t *base10 (uint8_t *p, int n)
{
    if(n>99)
    {
	p[0]= 3;
	p[1]= '0'+(n%1000)/100;
	p[2]= '0'+(n%100)/10;
	p[3]= '0'+(n%10)/1;
	p+= 4;
    }
    else if(n>9)
    {
	p[0]= 2;
	p[1]= '0'+(n%100)/10;
	p[2]= '0'+(n%10)/1;
	p+=3;
    }
    else
    {
	p[0]= 1;
	p[1]= '0'+(n%10)/1;
	p+= 2;
    }
    return p;
}

void Request::send()
{
    uint8_t packet[512];
    uint8_t *p= packet;
    p[ 0]= (id>>8)&0xFF;	// idH
    p[ 1]= (id>>0)&0xFF;	// idL
    p[ 2]= 1;			// FlagsH
    p[ 3]= 0;			// FlagsL
    p[ 4]= 0;			// QDCOUNTH
    p[ 5]= 1;			// QDCOUNTL
    p[ 6]= 0;			// ANCOUNTH
    p[ 7]= 0;			// ANCOUNTL
    p[ 8]= 0;			// NSCOUNTH
    p[ 9]= 0;			// NSCOUNTL
    p[10]= 0;			// ARCOUNTH
    p[11]= 0;			// ARCOUNTL
    p+= 12;

    p= base10(p,ip[3]);
    p= base10(p,ip[2]);
    p= base10(p,ip[1]);
    p= base10(p,ip[0]);

    p[0]=  7;
    p[1]= 'I';
    p[2]= 'N';
    p[3]= '-';
    p[4]= 'A';
    p[5]= 'D';
    p[6]= 'D';
    p[7]= 'R';
    p+= 8;

    p[0]=  4;
    p[1]= 'A';
    p[2]= 'R';
    p[3]= 'P';
    p[4]= 'A';
    p+= 5;

    p[0]=  0;
    p+= 1;

    p[0]= 0;			// TYPEH
    p[1]= 12;			// TYPEL
    p+= 2;

    p[0]= 00;			// CLASSH
    p[1]= 1;			// CLASSL
    p+= 2;

    int n= nServers;
    int size= p-packet;
    while(n--)
    {
	while(1)
	{
#ifdef WATT32
          int flags = 0;
#else
          int flags = MSG_DONTWAIT | MSG_NOSIGNAL;
#endif
          int r= sendto (chaussette, packet, size, flags,
                         (struct sockaddr*)(servers+n),
                         sizeof(servers[n]));
          if (r < 0 && (errno == EAGAIN || errno == EINTR))
             continue;
          if (r != size)
             warning ("sendto failed for server %d",n);
          ++packetsOut;
          break;
	}
    }
    timeStamp= time(0);
    ++sent;
}

static void output(
const char *ip,
const char *name,
const char *err
)
{
    printf(
	"%s %s status=%s pending=%d processed=%d total=%d packetsOut=%d packetsIn=%d\n",
	name,
	ip,
	err,
	pendingRequests,
	totalRequests-pendingRequests,
	totalRequests,
	packetsOut,
	packetsIn
    );
}

static void bookKeeping()
{
    time_t now = time(0);
    static time_t lastNow = time(0);
    if (now < lastNow)
       fatal("clock runs backward.giving up.",1);

    Request *r = allRequests;
    while (r)
    {
        Request *next = r->getANext();
        if (now-r->getTimeStamp() > 10)
	{
            if (r->getSent() < 5)
               r->send();
	    else
	    {
		char buf[32];
		r->getIP(buf);
		output(buf,buf,"timeout");
		delete r;
	    }
	}
        r = next;
    }
}

static uint8_t *walkName (uint8_t *packet, uint8_t *s, uint8_t *d, int *n)
{
    while(1)
    {
	uint8_t cnt= *(s++);
        if (cnt == 0)
           break;

        if ((cnt>>6) == 0x3)
	{
	    uint16_t offset= ((cnt&0x3F)<<8) | *(s++);
	    uint8_t *p= walkName(packet,packet+offset,d,n);
	    return p==0 ? 0 : s;
	}
        if (d==0 || n==0)
           s += cnt;
        else
        {
          if (cnt+1 > *n)
             return 0;
          *n -= cnt+1;
          while (cnt--)
             *(d++)= *(s++);
          *(d++)= '.';
        }
    }
    return s;
}

static void processDNSAnswer()
{
    uint8_t packet[512];
    memset(packet,0,512);

    int n= read(chaussette,packet,512);
    if(n<12) return;
    ++packetsIn;

    uint16_t id= (packet[0]<<8)|packet[1];
    Request *r= requests[id];
    while(r)
    {
	if(r->getId()==id) break;
	r= r->getNext();
    }
    if(r==0) return;

    char buf[32];
    r->getIP(buf);

// FIXME: Check we're dealing with the righ address

    int rcode = packet[3]&0xF;
    if (rcode)
       output(buf,buf,"serverError");
    else
    {
	uint8_t *p= packet+12;
        uint16_t qdCount = (packet[4]<<8) | packet[5];
	while(qdCount--)
	{
          p= walkName(packet,p,0,0);
          p+= 4;
	}

	p= walkName(packet,p,0,0);
	p+= 8;

	int n= 512;
	uint8_t rbuf[512];
	walkName(packet,p+2,rbuf,&n);
	rbuf[512-n-1]= 0;

	output(buf,(char*)rbuf,"ok");
    }
    delete r;

}

static void processDNSQuery()
{
    int a[4];
    if(
	4==sscanf(line,"%d.%d.%d.%d",a+0,a+1,a+2,a+3)	&&
	0<=a[0] && a[0]<256				&&
	0<=a[1] && a[1]<256				&&
	0<=a[2] && a[2]<256				&&
	0<=a[3] && a[3]<256
    )
    {
	Request *r= new Request(a);
	r->send();
    }
    else
    {
	output(
	    line,
	    line,
	    "malformed"
	);
    }
}


static void processSTDInput()
{
    static char *p= line;

    while(1)
    {
      while(1)
      {
          int r;

          errno = 0;        /* djgpp requires this !? */
          r = read(0,p,1);
          if (r < 0 && (errno == EINTR || errno == EAGAIN))
             continue;
          if (r == 0 && errno == 0)
          {
            inputDone= 1;
            p[0]= 0;
            break;
          }
#if defined(WATT32) && defined(__DJGPP__)  /* EOF situation? */
          if (errno == EPERM || errno == ENOSYS)
          {
            perror("stdin");
            inputDone = 1;
            p[0]= 0;  /* break both loops */
            break;
          }
#endif
          if (r != 1)
             fatal("read failed on stdin.");
          else
             break;
      }
      if (p[0] == '\n')
         p[0] = 0;
      if (p[0] == 0)
      {
        if (p>line)
           processDNSQuery();
        p = line;
        break;
      }
      ++p;
    }
}

int main (int argc, char **argv)
{
#ifdef WATT32
    if (argc > 1 && !strcmp(argv[1],"-d"))
    {
      argc--;
      argv++;
      debug = 1;
      dbug_init();
    }
    if ((chaussette = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP)) < 0)
       fatal("create socket failed.");
#else
    if ((chaussette = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP)) < 0)
       fatal("create socket failed.");

    if (fcntl(chaussette,F_SETFL,O_NDELAY))  /* No effect on udp ?! */
       fatal("set socket nonblocking failed.");
#endif

    sockaddr_in *srv= servers = (sockaddr_in*) malloc ((argc-1)*sizeof(sockaddr_in));
    if (srv == 0)
       fatal ("what was the chance of this happening. malloc failed.");

    for (int i=1; i<argc;++i)
    {
	struct hostent *he;

        memset (srv,0,sizeof(*srv));
        if (0 == (he = gethostbyname(argv[i])))
           warning ("couldn't resolve DNS server %s.",argv[i]);
	else
	{
          memcpy (&srv->sin_addr,he->h_addr,he->h_length);
          srv->sin_port   = htons((uint16_t)53);
          srv->sin_family = he->h_addrtype;
          ++nServers;
          ++srv;
	}
    }
    if (nServers==0)
       fatal("need at least 1 resolvable DNS server.",1);
    
    while (inputDone==0 || pendingRequests > 0)
    {
	fd_set rSet;
	fd_set eSet;
	FD_ZERO(&rSet);
	FD_ZERO(&eSet);
        FD_SET (chaussette,&rSet);

        if (inputDone==0)
           FD_SET(0,&rSet);

        int max = chaussette+1;
	struct timeval timeout;
        timeout.tv_sec  = 2;
        timeout.tv_usec = 0;

        int r = select(max,&rSet,0,&eSet,&timeout);
        if (r==-1)
           fatal("select failed.");

        if (FD_ISSET(0,&eSet))
           fatal("exception on stdin.");
        if (FD_ISSET(chaussette,&eSet))
           fatal("exception on socket.");

        if (FD_ISSET(0,&rSet))
           processSTDInput();
        if (FD_ISSET(chaussette,&rSet))
           processDNSAnswer();

	bookKeeping();
    }
}

