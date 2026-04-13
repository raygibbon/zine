// =====================================================================
// message.c - Sending and receiving of messages
//
// (C) 1993, 1994 by Michael Ringe, Institut fuer Theoretische Physik,
// RWTH Aachen, Aachen, Germany (michael@thphys.physik.rwth-aachen.de)
//
// This program is free software; see the file COPYING for details.
// =====================================================================

#ifndef NONET  /* file not used if NONET */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "talk.h"


// ---------------------------------------------------------------------
// Data structures used to communicate with the talk daemon. This is
// (partially) stolen from Linux's talkd and talk source.
// ---------------------------------------------------------------------

typedef struct {
        BYTE     vers;
        BYTE     type;
        BYTE     answer;
        BYTE     pad;
        DWORD    id_num;
        struct   sockaddr_in addr;
        struct   sockaddr_in ctl_addr;
	long	 pid;
#define NAME_SIZE 12
	char	 l_name[NAME_SIZE];
	char	 r_name[NAME_SIZE];
#define TTY_SIZE  16
	char	 r_tty[TTY_SIZE];
      } CTL_MSG;

typedef struct {
        BYTE     vers;
        BYTE     type;
        BYTE     answer;
        BYTE     pad;
        DWORD    id_num;
        struct   sockaddr_in addr;
	short    pad1;
      } CTL_RESPONSE;

#define TALK_VERSION      1

#define LEAVE_INVITE      0
#define LOOK_UP           1
#define DELETE            2
#define ANNOUNCE          3

#define SUCCESS           0
#define NOT_HERE          1
#define FAILED            2
#define MACHINE_UNKNOWN   3
#define PERMISSION_DENIED 4
#define UNKNOWN_REQUEST   5
#define BADVERSION        6
#define BADADDR           7
#define BADCTLADDR        8
#define MAXANSWER         8


// Old talk protocol

typedef struct {
        char   type;
        char   l_name[9];
        char   r_name[9];
        char   pad1;
        long   unknown;
        long   id_num;
        char   r_tty[16];
        struct sockaddr_in addr;
        short  pad2;
        struct sockaddr_in ctl_addr;
        short  pad3;
      } O_CTL_MSG;


typedef struct {
        char   type;
        char   answer;
        long   id_num;        // ??
        short  unknown;
        struct sockaddr_in addr;
        short  pad1;
      } O_CTL_RESPONSE;


static char *answermsg[MAXANSWER+2] = {
	"",
	"Your party is not logged on",
	"Target machine is too confused to talk to us",
	"Target machine does not recognize us",
	"Your party is refusing messages",
	"Target machine can not handle remote talk",
	"Target machine indicates protocol mismatch",
	"Target machine indicates protocol botch (addr)",
	"Target machine indicates protocol botch (ctl_addr)",

	// Error codes > MAXANSWER are mapped to MAXANSWER + 1
	"Unknown error code from target machine"
      };


static char err_connection[] = "Error: cannot open connection\r\n";
static char err_noresponse[] = "No response from target machine, giving up\r\n";

// ---------------------------------------------------------------------
// DB_dumpctl() - Display control message (for debugging)
// DB_dumpans() - Display answer message (for debugging)
// ---------------------------------------------------------------------

#ifdef DEBUG

static void DB_dumpctl (CTL_MSG *m)
{
  printf ("CTL_MSG: vers=%d type=%d id=%8.8lx addr=%d.%d.%d.%d:%u\n"
          "         ctl_addr=%d.%d.%d.%d:%u\n"
          "         l_name='%s' r_name='%s' r_tty='%s'\n",
          m->vers, m->type, m->id_num,
          DB_ipsplit (intel(m->addr.sin_addr.s_addr)), intel16(m->addr.sin_port),
          DB_ipsplit (intel(m->ctl_addr.sin_addr.s_addr)),
          intel16 (m->ctl_addr.sin_port), m->l_name, m->r_name, m->r_tty);
}

static void DB_dumpans (CTL_RESPONSE *m)
{
  printf ("CTL_RESPONSE: vers=%d type=%d answer=%d id=%8.8lx"
          " addr=%d.%d.%d.%d:%u\n",
          m->vers, m->type, m->answer, m->id_num,
          DB_ipsplit (intel(m->addr.sin_addr.s_addr)), intel16 (m->addr.sin_port));
}

static void O_DB_dumpctl (O_CTL_MSG *m)
{
  printf ("O_CTL_MSG: type=%d,u=%lx,id=%lx,addr=[%x]%d.%d.%d.%d:%u\n"
          "                            ctl_addr=[%x]%d.%d.%d.%d:%u\n"
          "           l_name='%s' r_name='%s' r_tty='%s'\n",
          m->type, intel(m->unknown), intel(m->id_num),
         intel16 (m->addr.sin_family),
         DB_ipsplit (intel(m->addr.sin_addr.s_addr)), intel16 (m->addr.sin_port),
         intel16 (m->addr.sin_family), DB_ipsplit (intel(m->ctl_addr.sin_addr.s_addr)),
         intel16 (m->ctl_addr.sin_port), m->l_name, m->r_name, m->r_tty);
}

static void O_DB_dumpans (O_CTL_RESPONSE *m)
{
  printf ("O_CTL_RESPONSE: type=%d answer=%d id=%lx u1=%x"
          " addr=[%x]%d.%d.%d.%d:%u\n",
          m->type, m->answer, intel (m->id_num), intel16 (m->unknown),
          intel16 (m->addr.sin_family), DB_ipsplit (intel(m->addr.sin_addr.s_addr)),
          intel16 (m->addr.sin_port));
}

#else
  #define DB_dumpctl(m)
  #define DB_dumpans(m)
  #define O_DB_dumpctl(m)
  #define O_DB_dumpans(m)
#endif

// ---------------------------------------------------------------------
// wait_invite() - Wait for a LOOK_UP message from the remote talk
// daemon. Listen on ports 517 (old talk) and 518 (new talk).
// Returns 0 on success (i.e., ds socket opened)
// ---------------------------------------------------------------------

static int reply (udp_Socket *s, CTL_MSG *m)
{
  CTL_RESPONSE ans;

  // Reply with SUCCESS
  ans.vers   = TALK_VERSION;
  ans.type   = LOOK_UP;
  ans.answer = SUCCESS;
  ans.id_num = m->id_num;
  setsockaddr (&ans.addr,MY_DATA_PORT,my_ip_addr);
  sock_write (s,(char*)&ans,sizeof(ans));
  DB_dumpans (&ans);
  DB_printf (("Replied with SUCCESS\r\n"));
  strncpy (localuser,m->r_name,NAME_SIZE+1);
  strncpy (remoteuser,m->l_name,NAME_SIZE+1);
  return 0;
}


static int o_reply (udp_Socket *s, O_CTL_MSG *m)
{
  O_CTL_RESPONSE ans;

  // Reply with SUCCESS
  ans.type    = LOOK_UP;
  ans.answer  = SUCCESS;
  ans.id_num  = m->id_num;
  ans.unknown = 0;

  setsockaddr (&ans.addr,MY_DATA_PORT,my_ip_addr);
  if ((m->id_num & 0xFFFF) != 0)
     ans.addr.sin_family=intel16(ans.addr.sin_family);
  sock_write (s,(char*)&ans,sizeof(ans));
  O_DB_dumpans (&ans);
  DB_printf (("Replied with SUCCESS\r\n"));
  strncpy (localuser,m->r_name,sizeof(m->r_name));
  strncpy (remoteuser,m->l_name,sizeof(m->l_name));
  return 0;
}


int wait_invite (void)
{
  static    int firsttime = 1;
  static    udp_Socket daemon, o_daemon;
  CTL_MSG   ctl;
  O_CTL_MSG o_ctl;
  WORD      status;

  if (!udp_open(&daemon,TALK_PORT,0,0,NULL))
      goto sock_err;

  DB_printf (("Listening on port %d\r\n",TALK_PORT));
  if (!udp_open(&o_daemon,O_TALK_PORT,0,0,NULL))
      goto sock_err;

  DB_printf (("Listening on port %d\r\n",O_TALK_PORT));

  // Wait for LOOK_UP
  if (firsttime || !quiet)
      cprintf ("Waiting for %s... press ESC to quit\r\n",
      (userarg == NULL) ? "invitation" : "your party to respond");
  firsttime = 0;

  while (1)
  {
    if (kbhit() && getch() == 27)
    {
      sock_close (&daemon);
      sock_close (&o_daemon);
      return 27;
    }

    tcp_tick (&daemon);

    if (sock_fastread(&daemon,(char*)&ctl,sizeof(ctl)) == sizeof(ctl))
    {
      DB_dumpctl(&ctl);
      if (ctl.vers == TALK_VERSION && ctl.type == LOOK_UP)
      {
        reply(&daemon,&ctl);
        break;
      }
    }
    if (sock_fastread(&o_daemon,(char*)&o_ctl,sizeof(o_ctl)) == sizeof(o_ctl))
    {
      O_DB_dumpctl (&o_ctl);
      if (o_ctl.type == LOOK_UP)
      {
        o_reply (&o_daemon,&o_ctl);
        break;
      }
    }
  }

  // Wait for tcp connection
  tcp_listen (&ds,MY_DATA_PORT,0,0,NULL,0);
  sock_wait_established (&ds,30,NULL,&status);
  sock_close (&daemon);
  sock_close (&o_daemon);

  // Set remote host name
  if (userarg == NULL)
  {
    static char hostname[100];
    struct sockaddr_in sa;
    int    l = sizeof(sa);

    _getpeername (&ds,&sa,&l);
    if (!resolve_ip(sa.sin_addr.s_addr,hostname,sizeof(hostname)))
       _inet_ntoa (hostname,sa.sin_addr.s_addr);   /* Lookup failed */
    remotehost = hostname;
  }
  return 0;

sock_err:
  return 1;
}

// ---------------------------------------------------------------------
// send_ctl() - Send a CTL_MSG to the remote talkd. Returns 0 on
// success.
// ---------------------------------------------------------------------

static int send_ctl (udp_Socket *cs, CTL_MSG *m, CTL_RESPONSE *r)
{
  int  count;
  WORD status = 0;

  for (count = 10; status != 1 && count > 0; --count)
  {
    DB_printf (("send_ctl(): Sending message\r\n"));
    DB_dumpctl (m);
    sock_write (cs,(char*)m,sizeof(*m));
    if (_ip_delay1(cs,2,NULL,&status))
       continue;
    if (sock_read(cs,(char*)r,sizeof(*r)) != sizeof(*r))
       continue;
    DB_dumpans (r);
    if (r->vers != TALK_VERSION || r->type != m->type)
       continue;
    DB_printf (("Valid response received, answer=%d (%s)\r\n",
               r->answer,answermsg[r->answer > MAXANSWER ? MAXANSWER+1:
               r->answer]));
    return 0;
  }
  cputs (err_noresponse);
  return 1;
}


// ---------------------------------------------------------------------
// invite() - Announce an invitation to the remote talkd.
// Return values: 0=sucess, but wait; 1=error; 2=success, connected;
// 3=connection refused
// ---------------------------------------------------------------------

int invite (void)
{
  udp_Socket   cs;           // Control socket
  CTL_MSG      ctl;          // Control message
  CTL_RESPONSE ans;          // Response from remote talkd
  WORD         status;
  int a;

  // Step I: Connect to remote talkd
  // -------------------------------
  if (!udp_open(&cs,MY_CTL_PORT,remoteip,TALK_PORT,NULL))
     goto sock_err;

  sock_wait_established (&cs,sock_delay,NULL,&status);
  DB_printf (("Connection to remote talkd established\r\n"));

  // Step II: Check if there is an invitation for us
  // -----------------------------------------------
  DB_printf (("Lookup on remote machine\r\n"));
  // Prepare the LOOK_UP message
  ctl.vers   = TALK_VERSION;
  ctl.type   = LOOK_UP;
  ctl.id_num = 0x1234;
  setsockaddr (&ctl.addr,0,0);
  setsockaddr (&ctl.ctl_addr,MY_CTL_PORT,my_ip_addr);
  strncpy (ctl.l_name,localuser, NAME_SIZE);
  strncpy (ctl.r_name,remoteuser,NAME_SIZE);
  strncpy (ctl.r_tty, remotetty, TTY_SIZE);

  // Send the message
  if (send_ctl(&cs,&ctl,&ans))
     return 1;

  // If the remote talkd had an invitation for us, connect now
  if (ans.answer == SUCCESS)
  {
    tcp_open (&ds,MY_DATA_PORT,intel(ans.addr.sin_addr.s_addr),
              intel16(ans.addr.sin_port),NULL);
    sock_wait_established (&ds, 30, NULL, &status);
    return 2;
  }

  // Step III: Announce our invitation to the remote talkd
  // -----------------------------------------------------
  DB_printf (("Announcing invitation to remote talkd\r\n"));
  // Prepare the ANNOUNCE message
  ctl.type = ANNOUNCE;
  setsockaddr (&ctl.addr,MY_DATA_PORT,my_ip_addr);
  // Send the ANNOUNCE message
  if (send_ctl(&cs,&ctl,&ans))
     return 1;

  a = ans.answer;
  if (a == SUCCESS)
     return 0;

  // Invitation failed: Print error message from remote talkd and exit
  // -----------------------------------------------------------------
  if (a > MAXANSWER)
      a = MAXANSWER + 1;
  cprintf ("%s\r\n",answermsg[a]);
  return 3;

sock_err:
  cputs (err_connection);
  return 1;
}


// ---------------------------------------------------------------------
// o_send_ctl() - Send an O_CTL_MSG to the remote talkd. Returns 0 on
// success.
// ---------------------------------------------------------------------

static int o_send_ctl (udp_Socket *cs, O_CTL_MSG *m, O_CTL_RESPONSE *r)
{
  int  count;
  WORD status = 0;
  WORD len;

  for (count = 10; status != 1 && count > 0; --count)
  {
    DB_printf (("o_send_ctl(): Sending message\r\n"));
    O_DB_dumpctl (m);
    sock_write (cs,(char*)m,sizeof(*m));
    sock_flush (&cs);
    if (_ip_delay1(cs,2,NULL,&status))
       continue;

    len = sock_fastread (cs,(char*)r,sizeof(*r));
    DB_printf (("%u bytes received (%d expected)\r\n",len,(int)sizeof(*r)));
    if (len != sizeof(*r) && len != sizeof(*r) - 2)
       continue;

    DB_printf (("response received for type %d\r\n",r->type));
    O_DB_dumpans (r);
    if (r->type != m->type)
       continue;
    return 0;
  }
  cputs (err_noresponse);
  return 1;
}


// ---------------------------------------------------------------------
// o_do_announce() - Announce an invitation to the remote talkd using
// the old protocol.
// ---------------------------------------------------------------------

static int o_do_announce (udp_Socket *cs)
{
  O_CTL_MSG      msg;         // Our message
  O_CTL_RESPONSE resp;        // Response from remote talk daemon
  int a;

  // We don't look up pending invitations at the remote host. This
  // would be useless because we never reply to incoming ANNOUNCE
  // messages. In fact, our talk daemon responds to any LOOK_UP
  // pretending that we have already an invitation for the caller.
  // The only situation where we have to look up is a PC-to-PC
  // connection, but this is done with the new protocol.
  // --mr

  // Announce our invitation to remote machine
  // -----------------------------------------

  DB_printf (("Announcing invitation to remote machine\r\n"));
  memset (&msg,0,sizeof(msg));
  msg.type = ANNOUNCE;
  strncpy (msg.l_name,localuser,sizeof(msg.l_name));
  strncpy (msg.r_name,remoteuser,sizeof(msg.r_name));

  srand ((unsigned)time(NULL));
  msg.id_num = rand();

  strncpy (msg.r_tty,remotetty,sizeof(msg.r_tty));
  setsockaddr (&msg.addr,MY_DATA_PORT,my_ip_addr);
  setsockaddr (&msg.ctl_addr,MY_CTL_PORT,my_ip_addr);
  if (o_send_ctl(cs,&msg,&resp)) return 1;

  // Process the response
  // --------------------
  a = resp.answer;
  if (resp.answer == SUCCESS)
     return 0;
  if (a > MAXANSWER)
      a = MAXANSWER + 1;
  cprintf ("%s\r\n",answermsg[a]);
  return 3;
}


// ---------------------------------------------------------------------
// o_invite() - Announce an invitation to the remote talkd.
// ---------------------------------------------------------------------

int o_invite (void)
{
  udp_Socket cs;              // Control socket
  int        i;
  WORD       status;

  if (!udp_open(&cs,MY_CTL_PORT,remoteip,O_TALK_PORT,NULL))
     goto sock_err;

  sock_wait_established (&cs,sock_delay,NULL,&status);
  DB_printf (("Connection to remote talkd established\r\n"));
  i = o_do_announce (&cs);
  sock_close (&cs);
  return i;

sock_err:
  if (status != 1)
     sock_close (&cs);
  cputs (err_connection);
  return 1;
}
#endif
