/* netio.c
 *
 * Author:  Kai-Uwe Rommel <rommel@ars.de>
 * Created: Wed Sep 25 1996
 */

static char *rcsid =
"$Id: netio.c,v 1.14 2001/04/19 12:20:55 Rommel Exp Rommel $";
static char *rcsrev = "$Revision: 1.14 $";

/*
 * $Log: netio.c,v $
 * Revision 1.14  2001/04/19 12:20:55  Rommel
 * added fixes for Unix systems
 *
 * Revision 1.13  2001/03/26 11:37:41  Rommel
 * avoid integer overflows during throughput calculation
 *
 * Revision 1.12  2000/12/01 15:57:57  Rommel
 * *** empty log message ***
 *
 * Revision 1.11  2000/03/01 12:21:47  rommel
 * fixed _INTEGRAL_MAX_BITS problem for WIN32
 *
 * Revision 1.10  1999/10/28 17:36:57  rommel
 * fixed OS/2 timer code
 *
 * Revision 1.9  1999/10/28 17:04:12  rommel
 * fixed timer code
 *
 * Revision 1.8  1999/10/24 19:08:20  rommel
 * imported DOS support from G. Vanem <gvanem@broadpark.no>
 *
 *
 * Revision 1.8  1999/10/12 11:02:00  giva
 * added Watt-32 with djgpp support. Added debug mode.
 * G. Vanem <gvanem@broadpark.no>
 *
 * Revision 1.7  1999/06/13 18:42:25  rommel
 * added Linux port with patches from Detlef Plotzky <plo@bvu.de>
 *
 * Revision 1.6  1998/10/12 11:14:58  rommel
 * change to malloc'ed (and tiled) memory for transfer buffers
 * (hint from Guenter Kukkukk <kukuk@berlin.snafu.de>)
 * for increased performance
 *
 * Revision 1.5  1998/07/31 14:15:03  rommel
 * added random buffer data
 * fixed bugs
 *
 * Revision 1.4  1997/09/12 17:35:04  rommel
 * termination bug fixes
 *
 * Revision 1.3  1997/09/12 12:00:15  rommel
 * added Win32 port
 * (tested for Windows NT only)
 *
 * Revision 1.2  1997/09/12 10:44:22  rommel
 * added TCP/IP and a command line interface
 *
 * Revision 1.1  1996/09/25 08:42:29  rommel
 * Initial revision
 *
 */

#ifdef WIN32
#define _INTEGRAL_MAX_BITS 64
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>

#if defined(__WATCOMC__) || defined(__DMC__)
#include <unistd.h>
#include <errno.h>
#include <conio.h>

#elif defined(UNIX) || defined(DJGPP)
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>

#else
#include <process.h>
#include "getopt.h"
#include "netbios.h"
#endif


/* TCP/IP system specific details */

#if defined(OS2)

#define BSD_SELECT
#include <types.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netdb.h>

#elif defined(WATT32)

#include <tcp.h>       /* sock_init() etc. */
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#define soclose      close_s
#define select       select_s
#define psock_errno  perror
#define gettimeofday gettimeofday2

#elif defined(WIN32)

#include <windows.h>
#include <winsock.h>
#define soclose closesocket

int sock_init(void)
{
  WSADATA wsaData;
  return WSAStartup(MAKEWORD(1, 1), &wsaData);
}

void psock_errno(char *text)
{
  int rc = WSAGetLastError();
  printf("%s: error code %d\n", text, rc);
}

#elif defined(UNIX)

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <time.h>
#include <netinet/in.h>
#include <netdb.h>

#define psock_errno(x) perror(x)
#define soclose(x) close(x)

int sock_init(void)
{
  return 0;
}

#endif /* UNIX */

/* global data */

#ifndef EINTR
#define EINTR 0
#endif

#define THREADSTACK 65536

int nSizes[] = {1024, 2048, 4096, 8192, 16384, 32768};
#define NSIZES (sizeof(nSizes) / sizeof(int))
#define NMAXSIZE 32768

int tSizes[] = {1024, 2048, 4096, 8192, 16384, 32767};
#define TSIZES (sizeof(tSizes) / sizeof(int))
#define TMAXSIZE 32767

#define INTERVAL 10

/* timer code */

int bTimeOver;

#ifdef OS2

#define INCL_DOS
#define INCL_NOPM
#include <os2.h>

typedef QWORD TIMER;

void APIENTRY TimerThread(ULONG nArg)
{
  HEV hSem;
  HTIMER hTimer;

  DosCreateEventSem(0, &hSem, DC_SEM_SHARED, 0);

  DosAsyncTimer(nArg * 1000, (HSEM) hSem, &hTimer);
  DosWaitEventSem(hSem, SEM_INDEFINITE_WAIT);
  DosStopTimer(hTimer);

  DosCloseEventSem(hSem);

  bTimeOver = 1;

  DosExit(EXIT_THREAD, 0);
}

int StartAlarm(long nSeconds)
{
  TID ttid;

  bTimeOver = 0;

  if (DosCreateThread(&ttid, TimerThread, nSeconds, 0, THREADSTACK))
    return printf("Cannot create timer thread.\n"), -1;

  return 0;
}

int StartTimer(TIMER *nStart)
{
  if (DosTmrQueryTime(nStart))
    return printf("Timer error.\n"), -1;

  return 0;
}

long StopTimer(TIMER *nStart, int nAccuracy)
{
  TIMER nStop;
  ULONG nFreq;

  if (DosTmrQueryTime(&nStop))
    return printf("Timer error.\n"), -1;
  if (DosTmrQueryFreq(&nFreq))
    return printf("Timer error.\n"), -1;

  nFreq = (nFreq + nAccuracy / 2) / nAccuracy;

  return (* (__int64*) &nStop - * (__int64*) nStart) / nFreq;
}

#endif /* OS2 */

#ifdef WIN32

typedef LARGE_INTEGER TIMER;

DWORD CALLBACK TimerThread(void * pArg)
{
  Sleep((long) pArg * 1000);
  bTimeOver = 1;

  return 0;
}

int StartAlarm(long nSeconds)
{
  DWORD ttid;

  bTimeOver = 0;

  if (CreateThread(0, THREADSTACK, TimerThread, (void *) nSeconds, 0, &ttid) == NULL)
    return printf("Cannot create timer thread.\n"), -1;

  return 0;
}

int StartTimer(TIMER *nStart)
{
  if (!QueryPerformanceCounter(nStart))
    return printf("Timer error.\n"), -1;

  return 0;
}

long StopTimer(TIMER *nStart, int nAccuracy)
{
  TIMER nStop, nFreq;

  if (!QueryPerformanceCounter(&nStop))
    return printf("Timer error.\n"), -1;
  if (!QueryPerformanceFrequency(&nFreq))
    return printf("Timer error.\n"), -1;

  nFreq.QuadPart = (nFreq.QuadPart + nAccuracy / 2) / nAccuracy;

  return (nStop.QuadPart - nStart->QuadPart) / nFreq.QuadPart;
}

#endif /* WIN32 */

#if (defined(UNIX) || defined(WATT32)) && !defined(WIN32)

typedef struct timeval TIMER;
long StopTimer(TIMER *nStart, int nAccuracy);

#ifdef WATT32
int StartAlarm (long nSeconds)
{
  bTimeOver = 0;
  return (0);
}

void CheckTimeout (TIMER *start, int duration)
{
  if (StopTimer(start,10000) > 10000*duration)
     bTimeOver = 1;
}
#else

void on_alarm(int signum)
{
  alarm(0);
  bTimeOver = 1;
}

int StartAlarm(long nSeconds)
{
  bTimeOver = 0;
  signal(SIGALRM, on_alarm);
  alarm(nSeconds);
  return 0;
}
#endif  /* WATT32 */

int StartTimer(TIMER *nStart)
{
  struct timezone tz = {0, 0};

  gettimeofday(nStart, &tz);

  return 0;
}

long StopTimer(TIMER *nStart, int nAccuracy)
{
  TIMER nStop;

  gettimeofday(&nStop, NULL);

  return (nStop.tv_sec - nStart->tv_sec) * nAccuracy
       + (nStop.tv_usec - nStart->tv_usec) * nAccuracy / 1000000;
}

#endif /* (UNIX || WATT32) && !WIN32 */

/* initialize data to transfer */

char *InitBuffer(int nSize)
{
  char *cBuffer = malloc(nSize);

  if (cBuffer != NULL)
  {
    int i;

    cBuffer[0] = 0;
    srand(1);

    for (i = 1; i < nSize; i++)
      cBuffer[i] = (char) rand();
  }

  return cBuffer;
}

/* NetBIOS code */

#ifdef USE_NETBIOS

char *ServerName = "NETIOSRV";
char *ClientName = "NETIOCLT";
USHORT nAdapter = 0;

NCB WorkNcb;
USHORT nLSN, rc;

void NetBiosServer(void *arg)
{
  char *cBuffer;
  int bQuit = 0;

  if ((cBuffer = InitBuffer(NMAXSIZE)) == NULL)
  {
    perror("malloc()");
    return;
  }

  if ((rc = NCBAddName(&WorkNcb, nAdapter, ServerName)) != 0 && rc != 13)
  {
    printf("NetBIOS AddName Failed, rc=0x%02X\n", rc);
    free(cBuffer);
    return;
  }

  for (;;)
  {
    printf("NetBIOS server Listening.\n");

    if ((rc = NCBListen(&WorkNcb, nAdapter, ServerName, "*", 0, 0, TRUE)) != 0)
    {
      printf("NetBIOS Listen failed, rc=0x%02X\n", rc);
      break;
    }

    nLSN = WorkNcb.basic_ncb.bncb.ncb_lsn;

    printf("NetBIOS connection established ... ");
    fflush(stdout);

    while ((rc = NCBReceive(&WorkNcb, nAdapter, nLSN,
		            cBuffer, NMAXSIZE, TRUE)) == 0
	   || rc == NB_MESSAGE_INCOMPLETE);

    if (rc != 0 && rc != NB_SESSION_CLOSED)
      printf("NetBIOS Receive Failed, rc=0x%02X\n", rc);

    rc = NCBHangup(&WorkNcb, nAdapter, nLSN);

    printf("done.\n");
    fflush(stdout);

    if (bQuit)
      break;
  }

  NCBDeleteName(&WorkNcb, nAdapter, ServerName);

  free(cBuffer);
}

void NetBiosBench(void *arg)
{
  char *cBuffer;
  TIMER nTimer;
  long nTime;
  long nData;
  int i;

  if ((cBuffer = InitBuffer(NMAXSIZE)) == NULL)
  {
    perror("malloc()");
    return;
  }

  if ((rc = NCBAddName(&WorkNcb, nAdapter, ClientName)) != 0 && rc != 13)
  {
    printf("NetBIOS AddName Failed, rc=0x%02X\n", rc);
    free(cBuffer);
    return;
  }

  if ((rc = NCBCall(&WorkNcb, nAdapter, ClientName, ServerName, 0, 0, TRUE)) != 0)
    printf("NetBIOS Call failed, rc=0x%02X\n", rc);
  else
  {
    nLSN = WorkNcb.basic_ncb.bncb.ncb_lsn;

    printf("\nNetBIOS connection established.\n");

    for (i = 0; i < NSIZES; i++)
    {
      printf("Packet size %2d KByte: ", (nSizes[i] + 512) / 1024);
      fflush(stdout);

      nData = 0;

      if (StartAlarm(INTERVAL) == 0 && StartTimer(&nTimer) == 0)
      {
	while (!bTimeOver)
	{
	  if ((rc = NCBSend(&WorkNcb, nAdapter, nLSN,
			    cBuffer, nSizes[i], TRUE)) != 0)
	  {
	    printf("NetBIOS Send Failed, rc=0x%02X\n", rc);
	    break;
	  }

	  nData++;
	}

	if ((nTime = StopTimer(&nTimer, 1024)) == -1)
	  printf("Timing failed.\n");
	else
	  if (nData < 100 * 1024 * INTERVAL / nSizes[i])
	    printf("  %.0f Byte/s\n", (double) nData * nSizes[i] * 1024 / nTime);
	  else
	    printf("  %.0f KByte/s\n", (double) nData * nSizes[i] / nTime);
      }
    }

    rc = NCBHangup(&WorkNcb, nAdapter, nLSN);
  }

  NCBDeleteName(&WorkNcb, nAdapter, ClientName);

  free(cBuffer);
}

#endif /* USE_NETBIOS */

/* TCP/IP code */

int nPort = 0x494F; /* "IO" */
struct in_addr addr_server;

void TcpIpServer(void *arg)
{
  char *cBuffer;
  int bQuit = 0;
  struct sockaddr_in sa_server, sa_client;
  int server, client, length;
  struct timeval tv;
  fd_set fds;
  int rc;

  if ((cBuffer = InitBuffer(TMAXSIZE)) == NULL)
  {
    perror("malloc()");
    return;
  }

  if ((server = socket(PF_INET, SOCK_STREAM, 0)) < 0)
  {
    psock_errno("socket()");
    free(cBuffer);
    return;
  }

  sa_server.sin_family = AF_INET;
  sa_server.sin_port = htons(nPort);
  sa_server.sin_addr.s_addr = INADDR_ANY;

  if (bind(server, (struct sockaddr *) &sa_server, sizeof(sa_server)) < 0)
  {
    psock_errno("bind()");
    soclose(server);
    free(cBuffer);
    return;
  }

  if (listen(server, 2) != 0)
  {
    psock_errno("listen()");
    soclose(server);
    free(cBuffer);
    return;
  }

  for (;;)
  {
    printf("TCP/IP server Listening.\n");

    FD_ZERO(&fds);
    FD_SET(server, &fds);
    tv.tv_sec  = 3600;
    tv.tv_usec = 0;

    if ((rc = select(FD_SETSIZE, &fds, 0, 0, &tv)) < 0)
    {
      psock_errno("select()");
      break;
    }

    if (rc == 0 || FD_ISSET(server, &fds) == 0)
      continue;

    length = sizeof(sa_client);
    if ((client = accept(server, (struct sockaddr *) &sa_client, &length)) == -1)
      continue;

    printf("TCP/IP connection established ... ");
    fflush(stdout);

    while ((rc = recv(client, cBuffer, TMAXSIZE, 0)) > 0);

    if (rc == -1)
    {
      psock_errno("recv()");
      bQuit = 1;
    }
    else
      printf("done.\n");

    soclose(client);

    if (bQuit)
      break;

#if defined(WATT32) && !defined(DJGPP) && !defined(WIN32)
    kbhit();
#endif
  }

  soclose(server);

  free(cBuffer);
}

void TcpIpBench(void *arg)
{
  char *cBuffer;
  TIMER nTimer;
  long nTime;
  long nData;
  int i;
  struct sockaddr_in sa_server;
  int server;
  int rc;

  if ((cBuffer = InitBuffer(TMAXSIZE)) == NULL)
  {
    perror("malloc()");
    return;
  }

  sa_server.sin_family = AF_INET;
  sa_server.sin_port = htons(nPort);
  sa_server.sin_addr = addr_server;

  if ((server = socket(PF_INET, SOCK_STREAM, 0)) < 0)
  {
    psock_errno("socket()");
    free(cBuffer);
    return;
  }

  if (connect(server, (struct sockaddr *) &sa_server, sizeof(sa_server)) < 0)
  {
    psock_errno("connect()");
    soclose(server);
    free(cBuffer);
    return;
  }

  printf("\nTCP/IP connection established.\n");

  for (i = 0; i < TSIZES; i++)
  {
    printf("Packet size %2d KByte: ", (tSizes[i] + 512) / 1024);
    fflush(stdout);

    nData = 0;

    if (StartAlarm(INTERVAL) == 0 && StartTimer(&nTimer) == 0)
    {
      while (!bTimeOver)
      {
	if ((rc = send(server, cBuffer, tSizes[i], 0)) != tSizes[i] && errno != 0 && errno != EINTR)
	{
	  psock_errno("send()");
	  break;
	}
#if defined(WATT32) && !defined(WIN32)
        CheckTimeout(&nTimer,INTERVAL);
#ifndef DJGPP
        kbhit();
#endif
#endif
        nData++;
      }

      if ((nTime = StopTimer(&nTimer, 1024)) == -1)
	printf("Timing failed.\n");
      else
	if (nData < 100 * 1024 * INTERVAL / tSizes[i])
	  printf("  %.0f Byte/s\n", (double) nData * tSizes[i] * 1024.0 / nTime);
	else
	  printf("  %.0f KByte/s\n", (double) nData * tSizes[i] / nTime);
    }
  }

  soclose(server);

  free(cBuffer);
}

/* main / user interface */

int bSRV, bNB, bTCP;

void handler(int sig)
{
#ifdef USE_NETBIOS
  if (bNB)
  {
    NCBDeleteName(&WorkNcb, nAdapter, ServerName);
    NCBDeleteName(&WorkNcb, nAdapter, ClientName);
    NCBClose(&WorkNcb, nAdapter);
  }
#endif

  exit(0);
}

void usage(void)
{
  printf(
#ifndef USE_NETBIOS
#ifdef WATT32
         "\nUsage: netio [-d] [-s] [-n] [-p <port>] [<server>]\n"
         "\n\t-d\t\tenable tcp/ip debugging"
#else
         "\nUsage: netio [-s] [-n] [-p <port>] [<server>]\n"
#endif
         "\n\t-s\t\trun server side of benchmark (otherwise run client)"
	 "\n\t-p <port>\tuse this port instead of the default (%d)"
         "\n\t<server>\tif the client side of the benchmark is running,"
	 "\n\t\t\ta server host name is required\n"
#else
	 "\nUsage: netio [-s] [-t | -n] [-p <port>] [-a <adapter>] [<server>]\n"
	 "\n\t-s\trun server side of benchmark (otherwise run client)"
	 "\n\t-t\tuse TCP/IP protocol for benchmark"
	 "\n\t-p <port>\tuse this port for TCP/IP (default is %d)"
	 "\n\t-n\tuse NetBIOS protocol for benchmark\n"
	 "\n\t-a <adapter>\tuse this NetBIOS adapter (default is 0)\n"
	 "\n\t<server>\tif TCP/IP is used for the client,"
	 "\n\t\t\ta server host name is required\n"
	 "\nThe server side can run either NetBIOS (-n) or TCP/IP (-t) protocol"
	 "\nor both (default if neither -t or -n are specified). The client runs"
	 "\none of both protocols only (must specify -t or -n).\n"
	 "\nThe -p and -a options apply to both client and server sides."
#endif
	 "\n", nPort);
  exit(1);
}

int main(int argc, char **argv)
{
  char szVersion[32];
  int option;
  struct hostent *host;

  strcpy(szVersion, rcsrev + sizeof("$Revision: ") - 1);
  *strchr(szVersion, ' ') = 0;

  printf("\nNETIO - Network Throughput Benchmark, Version %s"
	 "\n(C) 1997-2001 Kai Uwe Rommel\n", szVersion);

  if (argc == 1)
    usage();

  /* check arguments */

#ifndef USE_NETBIOS
  bTCP = 1;
#endif

  while ((option = getopt(argc, argv, "?stnp:a:d")) !=  -1)
    switch (option)
    {
    case 's':
      bSRV = 1;
      break;
    case 't':
      bTCP = 1;
      break;
    case 'p':
      nPort = atoi(optarg);
      break;
#ifdef USE_NETBIOS
    case 'n':
      bNB = 1;
      break;
    case 'a':
      nAdapter = atoi(optarg);
      break;
#endif
#ifdef WATT32
    case 'd':
      dbug_init();
      break;
#endif
    default:
      usage();
      break;
    }

  if (bSRV == 1 && bTCP == 0 && bNB == 0)
    bTCP = bNB = 1;

  /* initialize NetBIOS and/or TCP/IP */

#ifdef USE_NETBIOS
  if (bNB)
  {
    NetBIOS_API = NETBIOS;

    if (NetBIOS_Avail())
      return printf("NetBIOS not found\n"), 1;

    if ((rc = NCBReset(&WorkNcb, nAdapter, 2, 2, 2)) != 0)
      return printf("NetBIOS Reset failed, rc=0x%02X\n", rc), rc;
  }
#endif

  if (bTCP)
  {
    if (sock_init())
      return psock_errno("sock_init()"), 1;

    if (!bSRV)
    {
      if (optind == argc)
	usage();

      if (isdigit(*argv[optind]))
	addr_server.s_addr = inet_addr(argv[optind]);
      else
      {
	if ((host = gethostbyname(argv[optind])) == NULL)
	  return psock_errno("gethostbyname()"), 1;

	addr_server = * (struct in_addr *) (host->h_addr);
      }
    }
  }

  /* do work */

  signal(SIGINT, handler);

  if (bSRV)
  {
    printf("\n");

#ifndef USE_NETBIOS
    TcpIpServer(0);
#else
    if (bTCP && !bNB)
      TcpIpServer(0);
    else if (bNB && !bTCP)
      NetBiosServer(0);
    else
    {
      if (_beginthread(TcpIpServer, 0, THREADSTACK, 0) == -1)
	return printf("Cannot create additional thread.\n"), 2;

      NetBiosServer(0);
    }
#endif
  }
  else
  {
    if ((bTCP ^ bNB) == 0) /* exactly one of both only */
      usage();

    if (bTCP)
      TcpIpBench(0);
#ifdef USE_NETBIOS
    else if (bNB)
      NetBiosBench(0);
#endif
  }

  /* terminate */

#ifdef USE_NETBIOS
  if (bNB)
    NCBClose(&WorkNcb, nAdapter);
#else
  printf("\n");
#endif

  return 0;
}

/* end of netio.c */
