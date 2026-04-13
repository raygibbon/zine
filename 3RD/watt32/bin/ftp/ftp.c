#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <dos.h>
#include <time.h>
#include <fcntl.h>
#include <io.h>
#include <signal.h>
#include <malloc.h>
#include <assert.h>
#include <direct.h>
#include <math.h>

#ifdef __TURBOC__
#pragma hdrstop
#endif

//#define WATT_REINIT  /* complete sock_exit()/sock_init() for shells */

#define FTP_NAMES
#define TELCMDS
#define TELOPTS

#include "parse.h"
#include "ftp.h"

STATIC int          GetReply   (int expect_eof, int wait);
STATIC void         DoTelnet   (char c);
STATIC double       PrintXfer  (const char *dir, DWORD size, TIME *start);
STATIC TIME        *DoHashMark (DWORD got_bytes);
STATIC int          DoList     (char *cmd);
STATIC void         DataClose  (void);
STATIC int          BackGround (void);
STATIC int          PushMode   (void);
STATIC int          PopMode    (void);
STATIC DWORD        ExpectSize (int get, FILE *fil);
STATIC void         ClearPorts (void);
STATIC int          CheckPort  (WORD port);
STATIC const char  *RemoteGlob (const char *arg);
STATIC const char **LocalGlob  (const char *arg);
STATIC void         SetSockSize(void);

static DWORD  keep_alive;
static DWORD  host_ip;
static DWORD  expect;
static time_t start_time;

static char   host_name   [MAX_HOSTNAME];
static char   line_buffer [LIN_SIZE];
static int    sunique      = 0;
static int    runique      = 0;
static int    connected    = 0;
static int    sendport     = 1;
static int    doglob       = 1;
static int    xfer_mode    = TYPE_A;
static int    logged_in    = 0;
static int    closing      = 0;
static int    data_conn    = 0;
static int    trans_compl  = 0;
static int    data_error   = 0;
static int    reply_code   = 0;
static DWORD  restart_pos  = 0;
static FILE  *ftemp        = NULL;
static void  *data_sockbuf = NULL;
static BYTE   abort_cmd[]  = { IAC,IP,IAC,DM,'A','B','O','R',0 };

/*
 * Pool of tcp data-sockets. We use a new data-socket for
 * each data-operation. Hopefully the oldest socket (index 0)
 * is dead when we need it again.
 */
static tcp_Socket *sock_pool [NUM_DATA_SOCKS];

#ifdef USE_BSD_GLOB
  #include <sys/stat.h>
  #include "bsd_glob.h"

  static glob_t bglob;
#endif


/*
 * Initialise ftp client; (set debug-mode), init Watt-32, trap signals,
 * get working directory and set bakground "task".
 */
int ftp_init (void)
{
  if (debug_mode)
     dbug_init();

  sock_init();
  setup_sigint();
  setup_sigsegv();
  if (setjmp(cmdline))
     ftp_exit();

  getcwd (local_dir, sizeof(local_dir));
  strcpy (local_home, local_dir);
  home_drive = toupper (local_home[0]) - 'A' + 1;
  curr_drive = home_drive;
  background = BackGround;

  time (&start_time);
  is_init = 1;
  return (1);
}

/*
 * Open a control connection on 'port' (default 21).
 * Busy wait until established.
 */
STATIC int ctrl_sock_open (WORD port)
{
  static tcp_Socket sock;
  int    status = 0;

  if (!tcp_open(&sock,0,host_ip,port,NULL))
     return (0);

  ctrl_socket = &sock;
  sock_yield (ctrl_socket, (VoidProc)background);
  sock_wait_established (ctrl_socket, sock_delay, NULL, &status);

  connected = 1;
  closing   = 0;
  return (1);

sock_err:

  if (cfg.verbose)
  {
    switch (status)
    {
      case  1: WARN (_LANG("\r\nHost closed\r\n"));
               break;
      case -1: WARN1 (_LANG("\r\nError: %s\r\n"), _LANG(sockerr(&sock)));
               break;
    }
  }
  ftp_log ("ctrl-sock failed; %s", _LANG(sockerr(&sock)));
  sock_close (&sock);
  ctrl_socket = NULL;
  connected   = 0;
  closing     = 1;
  return (0);
}

/*
 * Connect to 'hostname' on control port 'hostport' (default 21).
 * Resolve 'hostname' to 'host_ip', connect, receive and show initial
 * greeting banner.
 */
STATIC int ftp_connect (char *hostname, WORD hostport)
{
  char ip[20];
  WORD port;
  int  rc;

  if (hostport)
       port = hostport;
  else port = cfg.ctrl_port;

  if (!_inet_addr(hostname) && cfg.verbose)
     xprintf (CtrlText, _LANG("Resolving `%s'.."), hostname);

  connected = 0;
  host_ip = resolve (hostname);
  if (!host_ip)
  {
    ftp_log ("Unknown host %s", hostname);
    xputs (CtrlText, _LANG("Unknown host\r\n"));
    return (0);
  }

  _inet_ntoa (ip, host_ip);
  if (!_inet_addr(hostname) && cfg.verbose)
     xprintf (CtrlText, " (%s)\r\n", ip);

  if (!_inet_addr(hostname))
       ftp_log ("Connecting to %s (%s), port %u", hostname, ip, port);
  else ftp_log ("Connecting to %s, port %u", hostname, port);

  if (!ctrl_sock_open(port))
     return (0);

  if (cfg.verbose)
     xputs (CtrlText, _LANG("Connected\r\n"));

  StatusLine (HostName, hostname);
  SetSockSize();

  sendport = 1;

  rc = GetReply (0, 1);
  if (rc > COMPLETE)
  {
    ftp_log ("Connection failed: %dxx", rc);
    StatusLine (HostName, " ");
    return (0);
  }
  return (1);
}

/*
 * Send login name, password (and account).
 * SRP: send user/password encrypted.
 */
STATIC int ftp_login (struct URL *url)
{
  char *user[2], *pass[2], *acct[2];

#ifdef USE_SRP
  if (!strcmp(cfg.auth_type,"SRP"))
  {
    user[0] = srp_user;
    pass[0] = srp_pass;
    acct[0] = srp_acct;
  }
  else
#endif
  {
    user[0] = url->user;
    pass[0] = url->pass;
    acct[0] = url->acct;
  }
  user[1] = NULL;
  pass[1] = NULL;
  acct[1] = NULL;

  if (!user[0][0])
     UserPrompt (url->user, sizeof(url->user), "Name (%s): ", url->host);

  if (ftp_user(1,user))
  {
    if (!pass[0][0])
    {
      password_mode = 1;
      UserPrompt (url->pass, sizeof(url->pass), "Password: ");
      password_mode = 0;
    }
    if (ftp_passw(1,pass) == CONTINUE)
    {
      if (!acct[0][0])
         UserPrompt (url->acct, sizeof(url->acct), "Account: ");
      Command ("ACCT %s",acct[0]);
    }
    return (1);
  }
  return (0);
}

/*
 * Command handler "open host [port]"
 * Connect, login, clear local ports in use, run init macro.
 */
int ftp_open (int argc, char **argv)
{
  struct URL url;

  if (ftp_isconn())
  {
    xprintf (CtrlText, _LANG("Already connected to `%s'\r\n"), host_name);
    return (0);
  }

  memset (&url, 0, sizeof(url));
  memset (&line_buffer, 0, sizeof(line_buffer));

  _strncpy (url.host, argv[0], sizeof(url.host)-1);
  url.port     = argc == 2 ? atoi (argv[1]) : FTP_CTRL_PORT;
  host_name[0] = 0;
  xfer_mode    = TYPE_A;
  logged_in    = 0;
  trans_compl  = 0;
  data_error   = 0;

  if (cfg.auto_login)
     GetUserPass (&url);

  if (!ftp_connect(url.host,url.port))
  {
    xputs (CtrlText, _LANG("Connection failed\r\n"));
    return (0);
  }

  _strncpy (host_name, url.host, sizeof(host_name)-1);

  if (!ftp_login(&url))
  {
    xprintf (CtrlText, _LANG("Login as `%s' failed on `%s'\r\n"),
             url.user, url.host);
    return (0);
  }
  logged_in = 1;

  if (cfg.initial_syst)
     ftp_syst();

  ftp_feat();

  ClearPorts();

  if (init_macro)
     ftp_macro (1, &init_macro);

  else if (FindMacro("init"))
  {
    char *argv[2] = { "init", NULL };
    ftp_macro (1, argv);
  }
  ARGSUSED (argc);
  return (1);
}

/*
 *  Open connection to host, use username/password,
 *  finally get file specified in URL.
 */
int ftp_open_url (char *str, WORD port)
{
  struct URL url;
  char  *path = NULL;
  char  *file = NULL;

  if (strlen(str) >= sizeof(url))
  {
    xprintf (WarnText, _LANG("URL (%s) too large\r\n"), str);
    return (0);
  }

  memset (&url, 0, sizeof(url));
  ParseURL (str, &url);
  url.port = port ? port : url.port;

#if 0
  ftp_log ("url.proto = `%s'\n\t\t\t   "
           "url.user  = `%s'\n\t\t\t   "
           "url.pass  = `%s'\n\t\t\t   "
           "url.host  = `%s'\n\t\t\t   "
           "url.port  =  %d \n\t\t\t   "
           "url.path  = `%s'\n\t\t\t   "
           "url.file  = `%s'",
           url.proto, url.user, url.pass, url.host,
           url.port, url.path, url.file);
#endif

  if (strnicmp(url.proto, "ftp", 3) && strcmp(url.proto,url.host))
  {
    xprintf (WarnText, _LANG("`%s://' protocol not supported\r\n"),
             url.proto);
    return (0);
  }

  if (!url.host[0])
  {
    xputs (WarnText, _LANG("Missing hostname\r\n"));
    return (0);
  }

  xfer_mode = TYPE_A;
  logged_in = 0;
  c_flag    = 1;       /* handle ^C */
  sigint_counter = 1;  /* cause ^C to exit program */

  if (cfg.auto_login && !url.user[0])
     GetUserPass (&url);

  if (!ftp_connect(url.host,url.port))
  {
    xputs (WarnText, _LANG("Connection failed\r\n"));
    return (0);
  }

  _strncpy (host_name, url.host, sizeof(host_name)-1);

  if (!ftp_login(&url))
  {
    xprintf (WarnText, _LANG("Login as `%s' failed on `%s'\r\n"),
             url.user, url.host);
    return (0);
  }

  if (url.path[0])
     path = url.path;

  logged_in = 1;
  ClearPorts();

  if (path && !ftp_cwd(1,&path))
  {
    xprintf (WarnText, _LANG("Path `%s' not found\n"), path);
    return (0);
  }

  if (file)
  {
    ftp_binary();
    if (strcspn(file," *?[]"))
         ftp_mget(1, &file);
    else ftp_get (1, &file);
  }
  sigint_counter = 0;
  return (1);
}

/*
 * Check if we're connected on the control socket (ftp_isconn)
 * and that we have a data connection             (ftp_isdata)
 */
int ftp_isconn (void)
{
  int con = (connected && ctrl_socket &&
             sock_sselect(ctrl_socket, SOCKESTABLISHED));

  if (con && (sock_sselect(ctrl_socket,0) == SOCKCLOSED))
      con = 0;
  if (!con)
     connected = 0;
  return (con);
}

int ftp_isdata (void)
{
  int dat = (connected && data_socket && data_conn &&
             sock_sselect(data_socket, SOCKESTABLISHED));
  if (!dat)
     data_conn = 0;
  return (dat);
}

/*
 * Polls the control-socket while transfering data.
 * Check for error conditions.
 */
STATIC void CtrlDrain (void)
{
  char buf[100], *p;
  int  len;

  if (!ctrl_socket || !sock_dataready(ctrl_socket))
     return;

  len = sock_fastread (ctrl_socket, buf, sizeof(buf));
  buf[len] = 0;
  rip (buf);
  p = strchr (buf, '2');

  /*
   * On a fast connection we may get control-message
   * "226 ASCII/Binary Transfer complete" before data arrives
   * or while data is in progress.
   */
  if (p && isdigit(*(p+1)))
  {
    int compl, abort;
    strcpy (line_buffer, buf);
    strupr (buf);
    compl = (strncmp(p,"226",3)  == 0);
    abort = (strstr (p+4,"ABOR") != NULL);

    if (compl || abort)
       trans_compl = 1;

    FTP_LOG (("drained ctrl-sock:\n\t\t\t   %s", line_buffer));
  }
  else
  {
    FTP_LOG (("drained ctrl-sock, %d bytes:\n\t\t\t   %s (%d,%d,%d)",
              len, buf, buf[0], buf[1], buf[2]));
  }
}

/*
 * Check if keep-alive timer should send a keepalive segment
 * in order not to timeout the ctrl-connection
 */
STATIC void CtrlKeepAlive (void)
{
  if (keep_alive && chk_timeout(keep_alive) && ctrl_socket)
  {
    sock_keepalive (ctrl_socket);
    DoKeepAlive();
    FTP_LOG (("ctrl-sock keepalive"));
  }
}

void DoKeepAlive (void)
{
  if (cfg.keepalive_to)
     keep_alive = set_timeout (1000*cfg.keepalive_to);
}

/*
 * DataYield() is called while data_socket (get/put) is active.
 */
STATIC void DataYield (void)
{
  kbhit();
  CtrlKeepAlive();
  if (background)
    (*background)();
  CtrlDrain();
}


/*
 * Parse response lines from ftp-server. e.g:
 *
 * 220-Welcome to the EUnet Norway archive\r\n
 *    ^response continues on next line
 * 220 relay FTP server (Version wu-2.4(3) ready\r\n
 *    ^last response line
 */

/*
 * for parsing replies to the ADAT command
 */
char *reply_parse, *reply_ptr;
char  reply_buf [FTP_BUFSIZ];

STATIC int GetReply (int expect_eof, int wait)
{
  char     digits[4];
  unsigned i    = 0;
  int      brk_save;
  int      idx  = 0;
  int      code = 0;

  reply_code = 0;
  if (wait && trans_compl)
  {
    if (cfg.verbose)
       xprintf (CtrlText, "%s\r\n", line_buffer);
    reply_code = 226;
    return (COMPLETE);
  }

  if (!wait && !sock_dataready(ctrl_socket))
     return (0);

  brk_save = _watt_handle_cbreak;
  _watt_handle_cbreak = 1;
  _watt_cbroke = 0;

  if (reply_parse)
      reply_ptr = reply_buf;

  while (i < sizeof(line_buffer) && idx < 5)
  {
    int c = sock_getc (ctrl_socket);

    if (c == EOF && idx > -1)
       break;

    switch (idx)
    {
      case 0:
      case 1:
      case 2: if (isdigit(c))
                   digits[idx++] = c;
              else idx = -1;
              break;

      case 3: if (c == '-')   /* continuation request */
                   idx = -1;
              else idx++;
              break;

      case 4: if (c == '\n')
                 idx = 5;
              break;

      default:if (c == '\n')
                 idx = 0;
              break;
    }

    if ((BYTE)c == IAC)
       DoTelnet (c);
    else
    {
      line_buffer[i++] = c;
      if (reply_parse)
         *reply_ptr++ = c;
    }
    kbhit();
  }

  line_buffer[i] = '\0';
  _watt_handle_cbreak = brk_save;

  if (!_watt_cbroke && i == 0 && !sock_sselect(ctrl_socket,SOCKESTABLISHED))
  {
    int close_status = 0;

    connected = 0;
    if (expect_eof)
       return (0);

    if (cfg.verbose)
       xputs (CtrlText, _LANG("Service not available, remote "
              "server has closed connection\r\n"));
    ftp_log ("remote server closed connection");

    sock_close (ctrl_socket);
    sock_wait_closed (ctrl_socket, sock_delay, NULL, &close_status);
  sock_err:
    ctrl_socket = NULL;
    reply_code  = 600;
    return (6);
  }

  if (i >= 3)
  {
    digits[3] = '\0';
    code = atoi (digits);
  }

  if (cfg.verbose)
     xputs (CtrlText, line_buffer);

  if (i > 4 && !strncmp(line_buffer,"425 ",4))
  {
    trans_compl = data_error = 1;
    code = 425;
  }

  if (_watt_cbroke)
  {
    _watt_cbroke = 0;
    if (cfg.verbose)
       xputs (CtrlText, _LANG("\r\nClosing.."));

    sock_puts (ctrl_socket, "QUIT\r\n");
    sock_close(ctrl_socket);          /* don't wait for closing */
    connected = 0;
    closing   = 1;
    code      = 0;
  }

  else if (reply_parse)
  {
    *reply_ptr = '\0';
    reply_ptr  = strstr (reply_buf, reply_parse);
    if (reply_ptr)
    {
      reply_parse = reply_ptr + strlen (reply_parse);
      reply_ptr   = strpbrk (reply_parse, " \r");
      if (reply_ptr)
         *reply_ptr = '\0';
    }
    else
      reply_parse = reply_ptr;
  }
  reply_code = code;
  return (code / 100);
}

/*----------------------------------------------------------------*/

STATIC void DoTelnet (char c0)
{
  int c1 = sock_getc (ctrl_socket);

  if (c1 == WILL || c1 == WONT || c1 == DO || c1 == DONT)
  {
#if FTP_DEBUG
    int c2 = sock_getc (ctrl_socket);
    FTP_LOG (("Telnet: %s,%s,%s", TELCMD(c0), TELCMD(c1), TELOPT(c2)));
    FTP_LOG (("        RemCtrl: %d", sock_rbused(ctrl_socket)));
#else
    ARGSUSED (c0);
#endif
  }
}

/*----------------------------------------------------------------*/

static int SendCommand (void *sock, char *cmd, int len)
{
#ifdef USE_SRP
  if (!strcmp(cfg.auth_type,"SRP"))
  {
    /* File protection level also determines whether
     * commands are MIC or ENC. Should be independent ...
     */
    char in [FTP_BUFSIZ];
    char out[FTP_BUFSIZ];
    int  error;
    int  length = srp_encode (prot_level == PROT_P, cmd, out, len);

    if (length < 0)
    {
      xputs (WarnText, _LANG("SRP failed to encode message.\r\n"));
      return (0);
    }
    error = radix_encode (out, in, &length, 0);
    if (error)
    {
      xprintf (WarnText, _LANG("Couldn't base 64 encode command (%s)\r\n"),
               radix_error(error));
      return (0);
    }
    if (prot_level == PROT_P)
         sock_printf (sock, "ENC %s\r\n", in);
    else sock_printf (sock, "MIC %s\r\n", in);

    if (debug_mode)
       xprintf (CtrlText,
                _LANG("secure_command(%s)\nencoding %d bytes %s %s\n"),
                cmd, length, prot_level == PROT_P ? "ENC" : "MIC", in);
  }
  else
#endif
    sock_printf (sock, "%s\r\n", cmd);

#ifndef USE_SRP
  ARGSUSED (len);
#endif
  return (1);
}

/*----------------------------------------------------------------*/

int Command (const char *fmt, ...)
{
  char cmd[512];
  int  ret, len;
  int  is_abort = 0;

  assert (ctrl_socket);

  if (!memcmp(fmt,abort_cmd,sizeof(abort_cmd)))
  {
    strcpy (cmd, (const char*)abort_cmd);
    strcat (cmd, "\r\n");
    len = strlen (cmd);
    is_abort = 1;
    if (debug_mode)
    {
      if (proxy_flag)
         xprintf (CtrlText, "%s ", host_name);
      xputs (CtrlText, "---> ABOR\r\n");
    }
  }
  else
  {
    va_list args;

    va_start (args, fmt);
    len = VSPRINTF (cmd, fmt, args);
    assert (len < (int)sizeof(cmd)-1);

    if (debug_mode)
    {
      if (proxy_flag)
         xprintf (CtrlText, "%s ", host_name);
      if (!strncmp("PASS ", cmd, 5))
           xputs  (CtrlText, "---> PASS ******\r\n");
      else xprintf(CtrlText, "---> %s\r\n", cmd);
    }
    va_end (args);
  }

  while (1)
  {
    if (is_abort)
    {
      sock_write (ctrl_socket, cmd, len);
      return (0);
    }

    if (!SendCommand(ctrl_socket,cmd,len))
       return (0);

    c_flag      = 1;
    trans_compl = 0;
    data_error  = 0;

    ret = GetReply (!strcmp(fmt,"QUIT"), 1);
    if (ret != ERROR)
       break;

    if (reply_code == 533 && prot_level && PROT_P)
    {
      xputs (WarnText, _LANG("ENC command not supported at server; "
                             "retrying under MIC...\r\n"));
      prot_level = PROT_S;
    }
    else
      break;
  }
  return (ret);
}

/*
 * We need to start a listen on the data channel before we send the
 * command, otherwise the server's data-connection will fail.
 * We use data-sockets in a round-robin fashion. In case a previously
 * used socket is not completely closed, we cannot reuse it. And we
 * don't have time to wait...
 */
STATIC int InitConn (void)
{
  static int  idx  = 0;
  static WORD port = 0;
  BYTE   ip [4];
  int    rc;

  if (passive)
  {
    // !! to-do
  }

  if (sendport || !port)
     do
     {
       port = RANDPORT (FTP_LOW_PORT, FTP_HIGH_PORT);
     }
     while (!CheckPort(port));           /* recently used port ? */

  if (!sock_pool[0])          /* first time alloc of socket-pool */
  {
    tcp_Socket *pool = malloc (NUM_DATA_SOCKS * sizeof(tcp_Socket));
    int i;

    assert (pool);
    for (i = 0; i < NUM_DATA_SOCKS; i++)
        sock_pool[i] = pool + i;
  }

  if (data_socket)
  {
    sock_setbuf (data_socket, NULL, 0);  /* use normal rx-buffer */
    sock_close (data_socket);      /* close previous data socket */
  }

  data_socket  = sock_pool [idx++];
  if (idx == NUM_DATA_SOCKS)       /* ready for next data socket */
     idx = 0;

  tcp_listen (data_socket, port, host_ip, 0, NULL, cfg.dataconn_to);

  if (cfg.recv_socksize > 2048)
  {
    if (!data_sockbuf)
    {
      data_sockbuf = malloc (cfg.recv_socksize);  /* never freed */
      assert (data_sockbuf);
    }
    sock_setbuf (data_socket, data_sockbuf, cfg.recv_socksize);
  }

  FTP_LOG (("data-sock listening on port %u", port));

  if (!sendport)
  {
    /* !!fix me: PASV not supported */
    return (1);
  }

  *(DWORD*)&ip = _gethostid();

  /* We need a string of comma separated byte values. The first four
   * are the an IP address. The 5th is the MSB (network order) of
   * the port number, the 6th is the LSB
   */
  rc = Command ("PORT %u,%u,%u,%u,%u,%u",
                ip[3], ip[2], ip[1], ip[0],
                port >> 8, port & 255);

  return (rc == COMPLETE && !data_error);
}

/*----------------------------------------------------------------*/

STATIC int CheckComplete (void)
{
//return (trans_compl);
  return (0);
}

STATIC int CheckDataError (void)
{
//return (data_error);
  return (0);
}

STATIC int DataConn (void)
{
  char buf[100];

  if (passive)
     return (1);

  if (data_error)   /* premature error in data-connection */
     return (0);

  assert (data_socket);
  sock_yield (data_socket, DataYield);
  DoKeepAlive();
  data_conn = 1;
  sock_timeout (ctrl_socket, 0);
  sock_wait_established (data_socket, cfg.dataconn_to,
                         (UserHandler)CheckComplete, NULL);
  return (1);

sock_err:

  sprintf (buf, _LANG("data connection failed; %s"),
                _LANG(sockerr(data_socket)));
  xprintf (CtrlText, "%s\r\n", buf);
  ftp_log (buf);

  sock_yield (data_socket, NULL);
  data_socket = NULL;
  data_conn   = 0;
  return (0);
}

/*
 * Read a chunk on the data-connection
 */
STATIC int get_data_line (int line_mode, char *buf, int len)
{
  int status = 0;

  sock_wait_input (data_socket, cfg.datawait_to,
                   (UserHandler)CheckDataError, &status);
  if (line_mode)
       len = sock_gets     (data_socket, buf, len-1);
  else len = sock_fastread (data_socket, buf, len-1);
  buf [len] = 0;
  return (len);

sock_err:
  if (status == -1)
     WARN1 (_LANG("Error: %s\r\n"), _LANG(sockerr(data_socket)));
  return (0);
}

/*
 * Long list remote directory using wildcards.
 */
int ftp_list (int argc, char **argv)
{
  char cmd[50];

  if (argc >= 1)
       sprintf(cmd, "LIST %s", argv[0]);
  else strcpy (cmd, "LIST");
  return DoList (cmd);
}

/*
 * Short list remote directory using wildcards.
 * If "ls -l", call ftp_list() for long list.
 */
int ftp_nlist (int argc, char **argv)
{
  char cmd[50];

  if (argc && !strncmp(argv[0],"-lR",3))
  {
    argv[0] = "*";
    return ftp_list (argc, argv);
  }
  if (argc && !strncmp(argv[0],"-l",2))
     return ftp_list (argc-1, argv+1);

  if (argc >= 1)
       sprintf(cmd, "NLST %s", argv[0]);
  else strcpy (cmd, "NLST");
  return DoList (cmd);
}

/*
 * List contents of multiple remote directories
 */
int ftp_mdir (int argc, char **argv)
{
  ARGSUSED (argc);
  ARGSUSED (argv);
  return (0);  /* !! to-do */
}

/*
 * Retrieve a remote file.
 */
STATIC int get_data_file (FILE *fil, double *speed)
{
  TIME  *start;
  DWORD  bytes, total = 0;
  double kbs;
  char   block [2048];

  if (!data_error)
  {
    start  = DoHashMark (0L);
    expect = ExpectSize (1, NULL);

    while ((bytes = sock_read(data_socket,block,sizeof(block))) > 0)
    {
      fwrite (block, bytes, 1, fil);
      DoHashMark (total += bytes);
      if (data_error)
         break;
    }
    kbs = PrintXfer ("received", total, start);
    StatusLine (XferStatistics, "%5.1fkbs (%.0f%%)", kbs, 100.0);

    if (!data_error)
       *speed = kbs;
  }
  return (total);
}

/*
 * Put a remote file.
 */
STATIC int put_data_file (FILE *fil, void *sock, double *speed)
{
  TIME  *start;
  DWORD  bytes, total = 0L;
  char  *block = malloc (cfg.send_socksize+1);
  double kbs;
  int    error = 0;

  assert (block);
  start  = DoHashMark (0L);
  expect = ExpectSize (0, fil);

  while (1)
  {
    if (!tcp_tick(sock))
    {
      error = 1;
      break;
    }
    if (sock_tbused(sock) == 0)  /* Tx buffer empty */
    {
      bytes = fread (block, 1, cfg.send_socksize, fil);
      if (bytes > 0)
      {
        sock_enqueue (sock, block, bytes);
        DoHashMark (total += bytes);
      }
      else
        break;
    }
    DataYield();
  }

  kbs = PrintXfer ("sent", total, start);
  StatusLine (XferStatistics, "%5.1fkBs (%.0f%%)", kbs, 100.0);

  if (!error)
     *speed = kbs;

  free (block);
  return (total);
}

/*----------------------------------------------------------------*/

int ftp_close (void)
{
  if (ftp_isconn())
  {
    if (!fast_exit)
       Command ("QUIT");
    sock_close (ctrl_socket);
  }
  StatusLine (HostName, NULL);
  StatusLine (XferStatistics, NULL);
  xfer_mode = TYPE_A;
  logged_in = 0;
  closing   = 1;
  return (1);
}

/*----------------------------------------------------------------*/

int ftp_exit (void)
{
  int rc = fast_exit ? -1 : 0;

  CleanUp();

  if (data_sockbuf)
     free (data_sockbuf);
  if (sock_pool[0])
     free (sock_pool[0]);
  data_sockbuf = sock_pool[0] = NULL;

#if defined(__DJGPP__) && 0
  _exit (rc);
#else
  exit (rc);
#endif
  return (0);
}

int ftp_fexit (void)
{
  fast_exit = 1;
  return ftp_exit();
}

/*----------------------------------------------------------------*/

int ftp_cwd (int argc, char **argv)
{
  static char last_wd [_MAX_PATH] = "/";
  char  *dir;
  int    rc;

  if (!strcmp(argv[0],"-"))
       dir = last_wd;
  else dir = argv[0];

  rc = Command ("CWD %s", dir);
  _strncpy (last_wd, dir, sizeof(last_wd));
  ftp_log ("CWD %s, %s", dir, OkComplete(rc));
  ARGSUSED (argc);
  return (rc == COMPLETE);
}

/*----------------------------------------------------------------*/

int ftp_cdup (void)
{
  int rc = Command ("CDUP");
  ftp_log ("CDUP, %s", OkComplete(rc));
  return (rc == COMPLETE);
}

/*----------------------------------------------------------------*/

int ftp_quote (int argc, char **argv)
{
  char cmd[80] = { 0 };
  int  verb    = cfg.verbose;
  int  i, rc;

  for (i = 0; i < argc; i++)
  {
    strcat (cmd, argv[i]);
    strcat (cmd, " ");
  }
  *strrchr (cmd,' ') = 0;
  cfg.verbose = 1;
  rc = Command (cmd);
  cfg.verbose = verb;
  ftp_log ("%s, %s", cmd, OkComplete(rc));
  return (rc == COMPLETE);
}

int ftp_site (int argc, char **argv)
{
  memmove (&argv[1], &argv[0], argc*sizeof(char*));
  argv[0] = "SITE";
  return ftp_quote (argc+1, argv);
}

int ftp_idle (int argc, char **argv)
{
  memmove (&argv[2], &argv[0], argc*sizeof(char*));
  argv[0] = "SITE";
  argv[1] = "IDLE";
  return ftp_quote (argc+2, argv);
}

int ftp_chmod (int argc, char **argv)
{
  memmove (&argv[2], &argv[0], argc*sizeof(char*));
  argv[0] = "SITE";
  argv[1] = "CHMOD";
  return ftp_quote (argc+2, argv);
}

/*
 * issue a proxy command on alternate connection
 */

int ftp_proxy (int argc, char **argv)
{
  if (argc >= 1 && argv[0][0] == '?')
  {
    ProxyHelp();
    return (1);
  }
  /* to-do !! */
  return (0);
}

/*----------------------------------------------------------------*/

static int    mtime_echo = 1;
static struct tm rem_time;

/*
 * get last modification time of file on remote machine
 */

int ftp_modtime (int argc, char **argv)
{
  char *file = argv[0];
  int  verb  = cfg.verbose;
  int  rc;

  cfg.verbose = 0;
  rc = Command ("MDTM %s", file);
  if (rc == COMPLETE)
  {
    sscanf (line_buffer, "%*s %04d%02d%02d%02d%02d%02d",
            &rem_time.tm_year, &rem_time.tm_mon, &rem_time.tm_mday,
            &rem_time.tm_hour, &rem_time.tm_min, &rem_time.tm_sec);

    if (mtime_echo)  /* format "06/23/1997 22:49:00 GMT" */
       xprintf (CtrlText,"%s:  %02d/%02d/%04d %02d:%02d:%02d GMT\r\n",
                file, rem_time.tm_mon, rem_time.tm_mday,rem_time.tm_year,
                      rem_time.tm_hour,rem_time.tm_min, rem_time.tm_sec);

    rem_time.tm_year -= 1900;
    rem_time.tm_mon--;
  }
  else
  {
    xprintf (CtrlText, "%s\r\n", line_buffer);
    memset (&rem_time, 0, sizeof(rem_time));
  }
  cfg.verbose = verb;
  ARGSUSED (argc);
  return (rc == COMPLETE);
}

/*
 * get remote file if newer than local file
 */
int ftp_newer (int argc, char **argv)
{
  char  *remote = argv[0];
  char  *local  = argc > 1 ? argv[1] : argv[0];
  int    rc     = 0;
  struct tm *tm = FileTime (local);

  mtime_echo = 0;

  /* if local file not found, get remote file
   */
  if (!tm)
     rc = ftp_get (argc, argv);

  else if (ftp_modtime(1,&remote))
  {
    if (mktime(&rem_time) > mktime(tm))
       rc = ftp_get (argc, argv);
  }
  mtime_echo = 1;
  return (rc);
}

/*
 * get remote file size
 */
static DWORD file_size;

int ftp_fsize (int argc, char **argv)
{
  int rc = Command ("SIZE %s", argv[0]);

  if (rc == COMPLETE)
       file_size = atol (line_buffer+4);
  else file_size = 0L;

  ftp_log ("File %s: size %lu, %s", argv[0], file_size, OkComplete(rc));
  ARGSUSED (argc);
  return (rc == COMPLETE);
}

/*----------------------------------------------------------------*/

int ftp_rhelp (int argc, char **argv)
{
  if (argc)
       return (Command("HELP %s",argv[0]) == COMPLETE);
  else return (Command("HELP") == COMPLETE);
}

/*----------------------------------------------------------------*/

int ftp_syst (void)
{
  if (!cfg.allow_syst)
  {
    xputs (CtrlText, _LANG("SYST command inhibited\r\n"));
    return (0);
  }
  return (Command("SYST") == COMPLETE);
}

/*----------------------------------------------------------------*/

int ftp_feat (void)
{
  int rc = Command ("FEAT");

  if (rc != COMPLETE)
     return (rc);

/* read result. e.g.:
     500 'FEAT': command not understood.
   or
     211-Features:
       MDTM
       REST STREAM
       SIZE
 */
  return (rc);
}

/*----------------------------------------------------------------*/

int ftp_binary (void)
{
  int rc = Command ("TYPE I");

  if (rc == COMPLETE)
     xfer_mode = TYPE_I;

  ftp_log ("BINARY mode, %s", OkComplete(rc));
  return (rc == COMPLETE);
}

/*----------------------------------------------------------------*/

int ftp_ascii (void)
{
  int rc = Command ("TYPE A");

  if (rc == COMPLETE)
     xfer_mode = TYPE_A;

  ftp_log ("ASCII mode, %s", OkComplete(rc));
  return (rc == COMPLETE);
}

/*----------------------------------------------------------------*/

int ftp_tenex (void)
{
  int rc = Command ("TYPE L 8");

  if (rc == COMPLETE)
     xfer_mode = TYPE_L;

  ftp_log ("TENEX mode, %s", OkComplete(rc));
  return (rc == COMPLETE);
}

/*----------------------------------------------------------------*/

int ftp_type (int argc, char **argv)
{
  char *mode = argv[0];

  if (argc == 0)
  {
    xprintf (CtrlText, _LANG("Using %s mode to transfer files.\r\n"),
             _LANG(typenames[xfer_mode]));
    return (1);
  }
  if (!stricmp(mode,_LANG(typenames[TYPE_A]))) return ftp_ascii();
  if (!stricmp(mode,_LANG(typenames[TYPE_I]))) return ftp_binary();
  if (!stricmp(mode,_LANG(typenames[TYPE_L]))) return ftp_tenex();
  if (!stricmp(mode,"tenex"))                  return ftp_tenex();

  xprintf (CtrlText, _LANG("%s: Unknown mode.\r\n"), mode);
  return (0);
}

/*----------------------------------------------------------------*/

int ftp_mkdir (int argc, char **argv)
{
  char *dir = argv[0];
  int   rc  = Command ("MKD %s", dir);

  ftp_log ("MKD %s, %s", dir, OkComplete(rc));
  ARGSUSED (argc);
  return (rc == COMPLETE);
}

/*----------------------------------------------------------------*/

int ftp_pwd (void)
{
  return (Command("PWD") == COMPLETE);
}

/*----------------------------------------------------------------*/

int ftp_user (int argc, char **argv)
{
  char *name = argv[0];
  int   rc   = Command ("USER %s", name);

  rc = (rc == CONTINUE || rc == COMPLETE);
  ftp_log ("USER %s, %s", name, OkFailed(rc));
  ARGSUSED (argc);
  return (rc);
}

/*----------------------------------------------------------------*/

int ftp_passw (int argc, char **argv)
{
  char *pass = argv[0];
  int   rc   = Command ("PASS %s", pass);
  int   ok   = (rc == CONTINUE || rc == COMPLETE);

  ftp_log ("PASS *******, %s", OkFailed(ok));
  ARGSUSED (argc);
  return (rc);
}

/*----------------------------------------------------------------*/

int ftp_acct (int argc, char **argv)
{
  char *acct = argv[0];
  int   rc   = Command ("ACCT %s", acct);

  ftp_log ("ACCT %s, %s", acct, OkComplete(rc));
  ARGSUSED (argc);
  return (rc == COMPLETE);
}

/*----------------------------------------------------------------*/

int ftp_delete (int argc, char **argv)
{
  char *file = argv[0];
  int   rc   = Command ("DELE %s", file);

  ftp_log ("DELE %s, %s", file, OkComplete(rc));
  ARGSUSED (argc);
  return (rc == COMPLETE);
}

/*----------------------------------------------------------------*/

int ftp_mdelete (int argc, char **argv)
{
  const char *fname;
  int         files = 0;

  while ((fname = RemoteGlob(argv[0])) != NULL)
  {
    if (*fname == 0)
       continue;

    if (interactive && !YesNoPrompt("mdelete %s? ",fname))
       continue;

    ftp_delete (1, (char**)&fname);
    files++;
  }
  CloseTempF();
  ARGSUSED (argc);
  return (files);
}

/*----------------------------------------------------------------*/

int ftp_rmdir (int argc, char **argv)
{
  char *dir = argv[0];
  char *cmd = "RMD";
  int   rc  = Command ("%s %s", cmd, dir);

  if (rc == ERROR && !strncmp(line_buffer,"550 ",4))
  {
    if (cfg.verbose)
        xprintf (CtrlText, _LANG("RMD command not recognized, trying XRMD\r\n"));
    cmd = "XRMD";
    rc  = Command ("%s %s", cmd, dir);
  }
  ftp_log ("RMD: %s %s, %s", cmd, dir, OkComplete(rc));
  ARGSUSED (argc);
  return (rc == COMPLETE);
}

/*----------------------------------------------------------------*/

int ftp_rename (int argc, char **argv)
{
  char *oldpath = argv[0];
  char *newpath = argv[1];
  int   rc;

  if (argc != 2 || !newpath)
  {
    xputs (CtrlText, _LANG("Usage: rename oldname newname\r\n"));
    return (0);
  }
  if (Command("RNFR %s",oldpath) == CONTINUE)
       rc = Command ("RNTO %s", newpath);
  else rc = ERROR;

  ftp_log ("RNFR %s, RNTO %s, %s", oldpath, newpath, OkComplete(rc));
  return (rc == COMPLETE);
}

/*
 * Change local (disk and) directory
 */
int ftp_lcd (int argc, char **argv)
{
  const char *path = argv[0];

#ifdef WIN32
  _chdir (path);
#else
  unsigned drives;
  chdir (path);
  if (path[1] == ':')
     _dos_setdrive (toupper(path[0])-'A'+1, &drives);
#endif

  getcwd (local_dir, sizeof(local_dir));
  curr_drive = toupper (local_dir[0]) - 'A' + 1;
  xprintf (CtrlText, _LANG("Local directory now %s\r\n"), local_dir);
  ftp_log ("LCD %s", local_dir);
  ARGSUSED (argc);
  return (strcmp(local_dir,local_home) != 0);
}

/*
 * put local-file [remote-file]
 */
int ftp_put (int argc, char **argv)
{
  const char *cmd;
  const char *local  = argv[0];
  const char *remote = argc > 1 ? argv[1] : argv[0];
  FILE       *fil    = _OpenFile (&local, "rb", NULL);
  int         rc     = 0;

  if (!fil)
     return (0);

  cmd = sunique ? "STOU" : "STOR";
  if (InitConn() && Command("%s %s",cmd,remote) == PRELIM)
  {
    if (DataConn())
    {
      double speed = 0.0;

      put_data_file (fil, data_socket, &speed);
      ftp_log ("File %s sent, remote name %s (%.1f kBs)",
               local, remote, speed);
      DataClose();
      rc = COMPLETE;
    }
  }
  fclose (fil);
  return (rc == COMPLETE);
}

/*
 * append local-file [remote-file]
 */
int ftp_append (int argc, char **argv)
{
  const char *local  = argv[0];
  const char *remote = argc > 1 ? argv[1] : argv[0];
  FILE       *fil    = _OpenFile (&local, "rb", NULL);
  int         rc     = 0;

  if (!fil)
     return (0);

  if (InitConn() && Command("APPE %s",remote) == PRELIM)
  {
    if (DataConn())
    {
      double speed = 0.0;

      put_data_file (fil, data_socket, &speed);
      ftp_log ("File %s sent, remote name %s appended (%.1f kBs)",
               local, remote, speed);
      DataClose();
      rc = COMPLETE;
    }
  }
  fclose (fil);
  return (rc == COMPLETE);
}

/*
 * get remote-file [local-file]
 */
int ftp_get (int argc, char **argv)
{
  const char *remote  = argv[0];
  const char *local   = argc > 1 ? argv[1] : argv[0];
  int         replace = interactive ? 0 : 1;
  int         rc      = 0;
  FILE       *fil;

  if (script_file)
     replace = 1;

  fil = _OpenFile (&local, "wb", &replace);
  if (!fil)
     return (CONTINUE);

  if (InitConn() && Command("RETR %s",remote) == PRELIM)
  {
    if (DataConn())
    {
      double speed = 0.0;

      get_data_file (fil, &speed);
      ftp_log ("File %s received, local name %s (%.1f kBs)",
               remote, local, speed);
      DataClose();
      rc = COMPLETE;
    }
  }
  fclose (fil);
  restart_pos = 0;
  return (rc == COMPLETE);
}

/*
 *  reget remote-file [local-file]
 *  continue transfer from restart position
 */
int ftp_reget (int argc, char **argv)
{
  const char *remote = argv[0];
  const char *local  = argc > 1 ? argv[1] : argv[0];
  FILE       *fil    = AppendFile (&local, &restart_pos);
  int         rc     = 0;

  if (!fil || !InitConn())
     return (0);

  if (Command("REST %lu",restart_pos) == CONTINUE &&
      Command("RETR %s", remote)      == PRELIM)
  {
    if (DataConn())
    {
      double speed = 0.0;

      get_data_file (fil, &speed);
      ftp_log ("File %s received, local name %s appended (%.1f kBs)",
               remote, local, speed);
      DataClose();
      rc = COMPLETE;
    }
  }
  fclose (fil);
  restart_pos = 0;
  return (rc == COMPLETE);
}

/*----------------------------------------------------------------*/

int ftp_restart (int argc, char **argv)
{
  long pos;

  if (argc != 1)
  {
    WARN (_LANG("restart: offset not specified\r\n"));
    return (0);
  }
  pos = atol (argv[0]);
  if (pos >= LONG_MAX || pos < 0L)
  {
    WARN1 (_LANG("Illegal offset %l"), pos);
    return (0);
  }
  restart_pos = (DWORD)pos;

  xprintf (CtrlText, _LANG("restarting at %lu. execute get, put or "\
           "append to initiate transfer\r\n"), restart_pos);
  return (1);
}

/*----------------------------------------------------------------*/

int ftp_mget (int argc, char **argv)
{
  const char *fname;
  int   files = 0;

  while ((fname = RemoteGlob(argv[0])) != NULL)
  {
    if (*fname == 0)
       continue;

    if (interactive && !YesNoPrompt("mget %s? ",fname))
       continue;

    if (!ftp_get(1,(char**)&fname))
       break;
    files++;
  }
  CloseTempF();
  ARGSUSED (argc);
  return (files);
}

/*----------------------------------------------------------------*/

int ftp_mput (int argc, char **argv)
{
  const char **fname;   /* fname[0] is local, fname[1] is remote name */
  int   files = 0;

  while ((fname = LocalGlob(argv[0])) != NULL)
  {
    if (!fname[0][0] || !fname[1][0])
       continue;

    if (interactive && !YesNoPrompt("mput %s? ",fname[0]))
       continue;

    if (!ftp_put(2,(char**)fname))
       break;
    files++;
  }

#ifdef USE_BSD_GLOB
  bsd_globfree (&bglob);
#else
  CloseTempF();
#endif

  ARGSUSED (argc);
  return (files);
}

/*----------------------------------------------------------------*/

int ftp_hash (void)
{
  cfg.hash_marks ^= 1;
  if (cfg.hash_marks)
       xprintf (CtrlText,
                _LANG("Hash mark printing on (%d bytes/hash mark)\r\n"),
                HASH_MARK_SIZE);

  else xputs (CtrlText, _LANG("Hash mark printing off\r\n"));
  return (cfg.hash_marks);
}

/*----------------------------------------------------------------*/

int ftp_debug (int argc, char **argv)
{
  if (argc >= 1)
  {
    debug_mode = atoi (argv[0]);
    tcp_set_debug_state (debug_mode);
  }
  xprintf (CtrlText, _LANG("Debugging %s (debug=%d).\r\n"),
           OnOff(debug_mode), debug_mode);
  return (debug_mode);
}

/*----------------------------------------------------------------*/

int ftp_pasv (void)
{
  passive ^= 1;
  xprintf (CtrlText, _LANG("Passive mode %s.\r\n"), OnOff(passive));
  return (1);
}

int ftp_sunique (void)
{
  sunique ^= 1;
  xprintf (CtrlText, _LANG("Store unique %s.\r\n"), OnOff(sunique));
  return (1);
}

int ftp_runique (void)
{
  runique ^= 1;
  xprintf (CtrlText, _LANG("Receive unique %s.\r\n"), OnOff(runique));
  return (1);
}

int ftp_port (void)
{
  sendport ^= 1;
  xprintf (CtrlText, _LANG("Use of PORT cmds %s.\r\n"), OnOff(sendport));
  return (1);
}

/*----------------------------------------------------------------*/

int ftp_beep (void)
{
  if (script_file && (!cfg.verbose || !script_echo))  /* silent script */
     return (1);

#ifdef __DMC__
  sound_beep (2000);
#else
  sound (2000);
  delay (20);
  nosound();
#endif
  return (1);
}

int ftp_bell (void)
{
  cfg.bell_mode ^= 1;
  xprintf (CtrlText, _LANG("Bell mode %s.\r\n"), OnOff(cfg.bell_mode));
  return (1);
}

int ftp_verb (void)
{
  cfg.verbose ^= 1;
  xprintf (CtrlText, _LANG("Verbose mode %s.\r\n"), OnOff(cfg.verbose));
  return (1);
}

int ftp_prompt (void)
{
  interactive ^= 1;
  xprintf (CtrlText, _LANG("Interactive mode %s.\r\n"), OnOff(interactive));
  return (1);
}

int ftp_glob (void)
{
  doglob ^= 1;
  xprintf (CtrlText, _LANG("Globbing %s.\r\n"), OnOff(doglob));
  return (1);
}

int ftp_trace (void)
{
  cfg.tracing ^= 1;
  xprintf (CtrlText, _LANG("Packet tracing %s.\r\n"), OnOff(cfg.tracing));
  return (1);
}

int ftp_struct (void)
{
  xprintf (CtrlText, _LANG("We don't support file structure, sorry.\r\n"));
  return (0);
}

int ftp_form (void)
{
  xprintf (CtrlText, _LANG("We only support non-print format, sorry.\r\n"));
  return (0);
}

int ftp_ftmode (void)
{
  xprintf (CtrlText, _LANG("We only support stream mode, sorry.\r\n"));
  return (0);
}

int ftp_cls (void)
{
  clrscr();
  return (1);
}

int ftp_reset (void)
{
  CtrlDrain();
  return (1);
}

/*----------------------------------------------------------------*/

int ftp_shell (char *cmd)
{
  const char *shell = getenv ("SHELL");
  static      BOOL new_shell;

  if (!shell)
     shell = getenv ("COMSPEC");

  if (!shell || *shell == '\0')
  {
    WARN (_LANG("No shell environment variable defined\r\n"));
    return (0);
  }

  if (*cmd == '\0')    /* Start a shell */
  {
    cmd = (char*) shell;
    new_shell = TRUE;
    ftp_log ("shell started");
    ftp_log_pause();
    StatusLine (HostName, "Type EXIT to return");
  }
  else
    new_shell = FALSE;

#ifdef WATT_REINIT
  sock_exit();
#else
  CloseWattFiles();
  _eth_release();      /* release packet-driver */
#endif

  signal (SIGINT, SIG_IGN);
  in_a_shell = 1;

  {
#if defined (__DJGPP__)
    int flags = __system_flags;

    if (cfg.use_comspec)  /* always call COMMAND/$SHELL */
         __system_flags |=  __system_call_cmdproc;
    else __system_flags &= ~__system_call_cmdproc;
#endif

    system (cmd);

#if defined (__DJGPP__)
    __system_flags = flags;
#endif
  }

#ifndef WATT_REINIT
  ReopenWattFiles();
#endif

  if (new_shell)
       ftp_log_continue();
  else ftp_log ("shell command: `%s'", cmd);

  in_a_shell = 0;
  setup_sigint();      /* reinit SIGINT handler */

#ifdef WATT_REINIT
  sock_init();
#else
  _eth_init();         /* reinit packet-driver */
#endif

  return (1);
}

/*----------------------------------------------------------------*/

int ftp_status (void)   /* local status */
{
  if (ftp_isconn())
       xprintf (CtrlText, _LANG("Connected to `%s'.%s\r\n"),
                host_name, logged_in ? "" : _LANG(" (not logged in)"));
  else xputs (CtrlText, _LANG("Not connected.\r\n"));

  xprintf (CtrlText, _LANG("Mode: %s; Type: %s; Form: %s; Structure: %s\r\n"),
           _LANG("stream"),   _LANG(typenames[xfer_mode]),
           _LANG("non-print"),_LANG("file"));

  xprintf (CtrlText, _LANG("Verbose: %s; Bell: %s; Prompting: %s; Globbing: %s\r\n"),
           OnOff(cfg.verbose), OnOff(cfg.bell_mode),
           OnOff(interactive), OnOff(doglob));

  xprintf (CtrlText, _LANG("Store unique: %s; Receive unique: %s\r\n"),
           OnOff(sunique), OnOff(runique));

#ifdef USE_SRP
  {
    char *lvl = ftp_getlevel();
    xprintf (CtrlText, _LANG("SRP protect level: %s\r\n"),
             lvl == NULL ? "none" : lvl);
  }
#endif

  xprintf (CtrlText,
           _LANG("Hash mark printing: %s; Use of PORT cmds: %s; Debugging: %d\r\n"),
           OnOff(cfg.hash_marks), OnOff(sendport), debug_mode);

  xprintf (CtrlText, _LANG("Memory free: %s. Disk %c: free: %s\r\n"),
           RamFree(), curr_drive+'A'-1, DiskFree());
  return (1);
}

/*
 * Command handler "rstat [filespec]"
 * Shows remote status (of filespec)
 */
int ftp_rstat (int argc, char **argv)
{
  if (argc)
       return Command ("STAT %s", argv[0]);
  else return Command ("STAT");
}

/*----------------------------------------------------------------*/

STATIC double PrintXfer (const char *direction, DWORD bytes, TIME *start)
{
  TIME   now;
  double delta, kbs;

  if (!bytes)
     return (0.0);

  gettimeofday (&now, NULL);
  delta = (now.tv_sec - start->tv_sec) + 1E-6*(now.tv_usec - start->tv_usec);
  delta = fabs (delta);
  if (delta)
       kbs = bytes / (1024.0 * delta);
  else kbs = bytes / 1024.0;

  if (cfg.hash_marks)
     xputs (CtrlText, "\r\n");
  xprintf (CtrlText, _LANG("%lu bytes %s in %.2g secs (%.2g kBs)\r\n"),
          bytes, direction, delta, kbs);
  return (kbs);
}

/*----------------------------------------------------------------*/

STATIC DWORD ExpectSize (int get, FILE *fil)
{
  DWORD size = 0;

  if (get)
  {
    char *s = strstr (line_buffer, " bytes)");

    if (s)
    {
      *s-- = '\0';
      while (isdigit(*s))
            s--;
      size = atol (s+1);
    }
  }
  else
  {
    assert (fil);
    size = filelength (fileno(fil));
  }
  if (size > restart_pos)
     return (size - restart_pos);
  return (size);
}

/*
 * Print hashmarks ('#') showing progress of transfer
 */
STATIC TIME *DoHashMark (DWORD got_bytes)
{
  static DWORD count = 0;
  static TIME  start;

  if (got_bytes == 0L)   /* starting new transfer */
  {
    gettimeofday (&start, NULL);
    count  = HASH_MARK_SIZE;
    expect = 0L;
    StatusLine (XferStatistics, " --- kBs (%.0f%%)", 0.0);
  }
  else if (cfg.hash_marks)
  {
    while (got_bytes >= count)
    {
      count += HASH_MARK_SIZE;
      xputch (DataText, '#');
    }
  }

  if (cfg.status_line && expect && got_bytes > 0L)
  {
    double bytes = (double) got_bytes;
    double msec;
    TIME   now;
    gettimeofday (&now, NULL);

    msec = 1E3 *(now.tv_sec  - start.tv_sec) +
           1E-3*(now.tv_usec - start.tv_usec);
    if (msec)
    {
      double kbs = bytes / msec; /* accumulated rate. to-do!! make filter */
      double pct = 100.0F * bytes / (double) expect;
      StatusLine (XferStatistics, "%5.1fkBs (%.0f%%)", kbs, pct);
    }
  }
  return (&start);
}

/*
 * Normal close of data connection
 */
STATIC void DataClose (void)
{
  keep_alive = 0;
  if (data_socket)
  {
    sock_yield (data_socket, NULL);
    sock_close (data_socket);
  }
  data_conn = 0;
  GetReply (0, 1);  /* get "226 Transfer complete" */
}

/*
 * Abort data connection sending ABOR command
 */
void DataAbort (void)
{
  if (ftp_isconn() && ftp_isdata())  /* have ctrl and data-socket */
  {
    keep_alive  = 0;
    data_conn   = 0;
    sock_yield (data_socket, NULL);
    sock_abort (data_socket);
    ftp_log ("ABORT command");
    Command ((char*)abort_cmd);
  }
}

STATIC void CheckCtrlClose (void)
{
  if (!closing || !ctrl_socket)
     return;

  if (sock_sselect(ctrl_socket,0) == SOCKCLOSED)
  {
    ctrl_socket = NULL;
    connected   = 0;
    closing     = 0;
    FTP_LOG (("Ctrl-sock closed (timeout)"));
  }
}

STATIC int BackGround (void)
{
  static DWORD sec_timer = 0UL;

  if (sec_timer == 0)
      sec_timer = set_timeout (1000);

  else if (chk_timeout(sec_timer))
  {
    UpdateTime (start_time);
    sec_timer = set_timeout (1000);
  }

  kbhit();       /* help ^C generation */

  if (editing)   /* we're handling user/script input */
  {
    tcp_tick (NULL);
    CtrlKeepAlive();
  }
  CheckCtrlClose();
  return (1);
}

/*----------------------------------------------------------------*/

static int first_line;

static void print_parsed_line (struct ftpparse *fp)
{
  if (first_line)
     xputs (DataText, "\r\n");
  first_line = 0;

  xprintf (DataText, " %6ld%c  %.24s  %.*s",
           fp->size, "?BA"[fp->sizetype & 3], ctime (&fp->mtime),
           fp->namelen, fp->name);

  if (fp->flagtrycwd)
     xputs (DataText, "<dir> ");

  if (fp->flagislink)
     xputs (DataText, "<link>");

  xputch (DataText, '\n');
}

/*----------------------------------------------------------------*/

STATIC int DoList (char *cmd)
{
  int rc = 0;

  if (!PushMode())
     return (0);

  if (InitConn() && (Command(cmd) == PRELIM) && DataConn())
  {
    first_line = 1;
    while (1)
    {
      struct ftpparse fp;
      char   buf[200];
      int    len = get_data_line (use_parse, buf, sizeof(buf));

      if (!len)
         break;

      if (use_parse && parse_line(&fp,buf,len))
      {
        print_parsed_line (&fp);
        continue;
      }
      xputs (DataText, buf);
      rc = COMPLETE;
    }
    DataClose();
  }
  PopMode();
  ftp_log ("%s, %s", cmd, OkComplete(rc));
  return (rc);
}

/*----------------------------------------------------------------*/

static int old_mode;

STATIC int PushMode (void)
{
  int rc = 1;

  old_mode = xfer_mode;
  if (old_mode == TYPE_I)
  {
    int verb = cfg.verbose;
    cfg.verbose = 0;
    rc = ftp_ascii();
    cfg.verbose = verb;
  }
  return (rc);
}

STATIC int PopMode (void)
{
  int rc = 0;

  if (ftp_isconn() && old_mode == TYPE_I)
  {
    int verb = cfg.verbose;
    cfg.verbose = 0;
    rc = ftp_binary();
    cfg.verbose = verb;
  }
  return (rc);
}

/*----------------------------------------------------------------*/

void CloseTempF (void)
{
  if (ftemp)
  {
    fclose (ftemp);
    ftemp = NULL;
  }
}

/*
 * Globulize a remote directory by making a list (NLST)
 * of files matching arg. Write list to temp-file (ftemp).
 */
STATIC const char *RemoteGlob (const char *arg)
{
  static char buf[80];

  if (!ftemp)          /* create temp list-file */
  {
    int verb;

    SET_BIN_MODE();

    ftemp = tmpfile();
    if (!ftemp)
    {
      WARN (_LANG("Can't find list of remote files, oops\r\n"));
      return (NULL);
    }
    if (!PushMode())   /* set temporary ASCII mode */
       return (NULL);

    verb = cfg.verbose;
    cfg.verbose = 0;

    if (InitConn() && Command("NLST %s",arg) == PRELIM && DataConn())
    {
      while (1)
      {
        if (!get_data_line(1, buf, sizeof(buf)) ||
            !strncmp(buf, "550 ", 4))  /* "550 ... No such file.." */
           break;
        fprintf (ftemp, "%s\n", buf);
      }
      DataClose();
    }
    else
    {
      cfg.verbose = verb;
      WARN (_LANG("Can't build list of remote files, oops\r\n"));
      PopMode();
      return (NULL);
    }
    cfg.verbose = verb;
    rewind (ftemp);
    PopMode();
  }

  buf[0] = '\0';
  if (!fgets(buf,sizeof(buf),ftemp) || !buf[0])
     return (NULL);

  return rip (buf);    /* return first globbed file */
}

/*
 * Globulize a local directory by making a list
 * of files matching arg. Write list to temp-file.
 */
STATIC const char **LocalGlob (const char *pattern)
{
  static const char *argv[2];
  static char  buf [_MAX_PATH*2];
  static char  rem [_MAX_PATH];

#if defined(WIN32)
  WARN (_LANG("No globulize on Windows yet\r\n"));
  ARGSUSED (pattern);
  return (NULL);

#elif defined(USE_BSD_GLOB)
  if (bglob.gl_pathv[0] == NULL)
     bsd_glob (pattern, 0, NULL, &bglob);

  if (bglob.gl_matchc == 0)
  {
    WARN1 (_LANG("No match `%s'\r\n"), pattern);
    return (NULL);
  }
  strncpy (buf, bglob.gl_pathv[bglob.gl_pathc], sizeof(buf)-1);
  buf [sizeof(buf)-1] = '\0';

#else
  if (!ftemp)  /* create temp list-file */
  {
    struct   find_t fi;
    unsigned attr  = (_A_NORMAL | _A_ARCH | _A_RDONLY);
    char    *slash = strrchr (pattern, '\\');
    char     path [_MAX_PATH] = { 0 };

    if (!strcmp(pattern,"*") || !strcmp(pattern,"."))
       pattern = "*.*";

    if (slash)
         _strncpy (path, pattern, slash-pattern);
    else _strncpy (path, local_dir, sizeof(path));

    if (_dos_findfirst(pattern,attr,&fi))
    {
      WARN1 (_LANG("No match `%s'\r\n"), pattern);
      return (NULL);
    }

    SET_BIN_MODE();

    ftemp = tmpfile();
    if (!ftemp)
    {
      WARN (_LANG("Can't get list of local files, oops\r\n"));
      return (NULL);
    }
    do
    {
      fprintf (ftemp, "%s\\%s\n", path, fi.name);
    }
    while (!_dos_findnext(&fi));
    rewind (ftemp);
  }

  buf[0] = '\0';
  if (!fgets(buf,sizeof(buf),ftemp) || !buf[0])
     return (NULL);
#endif

  rip (buf);
  strcpy (rem, strrchr(buf,'\\')+1);
  strlwr (buf);

  if (!sunique)
     strlwr (rem);

  argv[0] = buf;
  argv[1] = rem;
  return (argv);
}

/*
 * SetSockSize():
 *   Sets the cfg.recv_socksize based on the speed of the connection.
 *   Size is between 2k bytes for slow connections and 8k bytes for
 *   the fastest. Connection speed (approx. RTT) is measured using ICMP
 *   EchoRequest (ping).
 */
STATIC void SetSockSize (void)
{
  TIME  start, now;
  DWORD dummy, diff = 0L;

  if (!cfg.do_ping || !_ping(host_ip,1,NULL,0))
     return;

  gettimeofday (&start, NULL);

  while (diff < 2000)   /* wait max 2 seconds */
  {
    tcp_tick (NULL);
    gettimeofday (&now, NULL);
    diff = 1E3 *(now.tv_sec  - start.tv_sec) +
           1E-3*(now.tv_usec - start.tv_usec);

    if (_chk_ping(host_ip,&dummy) != 0xFFFFFFFFL)
    {
      int buf_size = 8192; /* heuristic buf-size based on speed */

      if (diff >= 2000) buf_size = 1024;
      if (diff <= 1000) buf_size = 2048;
      if (diff <=  300) buf_size = 4096;
      if (diff <=  100) buf_size = 8192;

      /*
       * 1. On a high-latency connection with low bandwidth, we
       *    should use a small data-window.
       * 2. On a high-latency connection with high bandwidth (e.g. a
       *    satelitte link), we should use a high data-window.
       */
      if (cfg.recv_socksize > buf_size)
          cfg.recv_socksize = buf_size;

      ftp_log ("ping: RTT %lumS, Rx sock-size set to %d bytes",
               diff, cfg.recv_socksize);
      return;
    }
  }
}

/*
 * Determine what local port to use for data tranfers.
 * We keep an array of MAX_PORTS words that we check against a
 * randomly generated port in InitConn().
 */

static int  num_ports = 0;
static WORD ports [MAX_PORTS];

STATIC void ClearPorts (void)
{
  num_ports = 0;
  memset (&ports, 0, sizeof(ports));
}

STATIC int CheckPort (WORD port)
{
  int i, found = 0;

  if (num_ports >= MAX_PORTS)  /* array full, allow the port */
     return (1);

  for (i = 0; i < num_ports; i++)
  {
    found = (ports[i] == port);
    if (found)
       break;
  }
  if (!found)
  {
    ports [num_ports++] = port;
    return (1);
  }
  return (0);  /* found an already used port */
}
