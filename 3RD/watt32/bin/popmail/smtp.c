/*
 * pcsmtp.c  --  WatTCP Pop-mail libraries
 *
 *  Copyright (C) 1992  Walter Andrew Nolan
 *
 *                      wan@eng.ufl.edu
 *
 *                      ECS-MIS
 *                      College of Engineering
 *                      University of Florida
 *
 *                      Version 1.00
 *
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <memory.h>
#include <tcp.h>

#include "popsmtp.h"

#ifdef __TURBOC__
#pragma warn -pro
#endif

#define SMTP_ASCII_MODE 1   /* use ASCII mode for smtp connection */

/*
 * Global Variables
 */

int   SMTP_debugOn = 0;
int   SMTP_errCond;
char *SMTP_errMsg;

/*
 * Local Variables
 */               
#define ERR_LEN 100
static char  errMsgBuf [ERR_LEN] = "";
static char  rx_buf [TEMP_BUF_LEN];
static int   SMTP_status;

/*
 * Handy macros
 */
#if SMTP_ASCII_MODE
  #define PUTS_DATA(sock)     sock_puts (sock, "DATA")
  #define PUTS_RSET(sock)     sock_puts (sock, "RSET")
  #define PUTS_QUIT(sock)     sock_puts (sock, "QUIT")
  #define PUTS_END(sock)      sock_puts (sock, ".")
  #define PUTS_BUF(sock,buf)  sock_puts (sock, buf)
  #define SOCK_PRINTF         sock_printf
#else
  #define PUTS_DATA(sock)     sock_puts (sock, "DATA\r\n")
  #define PUTS_RSET(sock)     sock_puts (sock, "RSET\r\n")
  #define PUTS_QUIT(sock)     sock_puts (sock, "QUIT\r\n")
  #define PUTS_END(sock)      sock_puts (sock, ".\r\n")
  #define SOCK_PRINTF         sock_printf_nl
  #define PUTS_BUF(sock,buf)  do {                     \
                                strcat (buf, "\r\n");  \
                                sock_puts (sock, buf); \
                              } while (0)
#endif


#define SMTP_DEBUG_MSG(s1,s2) if (SMTP_debugOn) {            \
                                 printf ("%s %s\n", s1, s2); \
                              }

#define SOCK_GETS(sock)                                         \
        sock_wait_input (sock, sock_delay, NULL, &SMTP_status); \
        sock_gets (sock, rx_buf, sizeof(rx_buf));               \
        SMTP_DEBUG_MSG (">>>", rx_buf);


#define SMTP_READ_ERR(sock,SHUTDOWN)                            \
        sock_err:                                               \
        if (SMTP_status == 1 || SMTP_status == -1) {            \
           SMTP_DEBUG_MSG ("Error:", sockerr(sock));            \
           SHUTDOWN;                                            \
           return (0);                                          \
        }

#define SMTP_FAIL_ON(SOCK,NUM,SHUTDOWN)                         \
        if (SMTP_errCond == NUM) {                              \
           SMTP_DEBUG_MSG ("SMTP>", "Reply was `" #NUM "'");    \
           smtp_shutdown (SOCK);                                \
           free (SOCK);                                         \
           strncpy (SMTP_errMsg, "SMTP Error -- SMTP Reply"     \
                    " '" #NUM "'",ERR_LEN);                     \
           SMTP_errCond = SMTP_ERR;                             \
           SHUTDOWN;                                            \
           return (0);                                          \
        }


#define SMTP_RESET_ON(SOCK,NUM,SHUTDOWN)                          \
        if (SMTP_errCond == NUM) {                                \
           SMTP_DEBUG_MSG ("SMTP>","Reply was `" #NUM "'");       \
           PUTS_RSET (SOCK);                                      \
           sock_wait_input (SOCK, sock_delay, NULL, &SMTP_status);\
           sock_gets (SOCK, rx_buf, sizeof(rx_buf));              \
           SMTP_errCond = SMTP_PROB;                              \
           strncpy (SMTP_errMsg,"SMTP Problem -- SMTP Reply "     \
                    "'" #NUM "'",ERR_LEN);                        \
           SHUTDOWN;                                              \
           return (0);                                            \
        }


/*
 * General Stuff
 */

static char *trim (char *str)
{
  int i;

  if (!str)
     return (NULL);

  for (i = strlen(str)-1; (i >= 0) && isspace(str[i]); str[i--] = '\0')
      ;

  while (isspace(str[0]))
        strcpy (str, str+1);
  return (str);
}


static char *fix_quoted_commas (char *string)
{
  char *ptr    = string;
  int   quoted = 0;

  while (*ptr)
  {
    if (*ptr == '\"')
       quoted ^= 1;
    if (*ptr == ',' && quoted)
       *ptr = '³';
    ptr++;
  }
  return (string);
}


/*
 *  SMTP Stuff
 */
#if (SMTP_ASCII_MODE == 0)

static int sock_printf_nl (tcp_Socket *s, const char *fmt, ...)
{
  char buf[2000];
  int  len = vsprintf (buf, fmt, (&fmt)+1);

  strcat (buf, "\r\n");
  return sock_write (s, buf, len+2);
}
#endif


tcp_Socket * smtp_init (char *host, char *dom)
{
  DWORD      ip;
  tcp_Socket *sock = NULL;

  if (SMTP_errMsg == NULL)
      SMTP_errMsg = errMsgBuf;

  if ((ip = lookup_host(host,NULL)) == 0L)
  {
    (*_printf) ("Cannot resolve host %s\r\n", host);
    return (NULL);
  }

  sock = malloc (sizeof(tcp_Socket));
  if (!sock)
  {
    (*_printf) ("malloc failed\r\n");
    return (NULL);
  }

  if (!tcp_open(sock,0,ip,SMTP_PORT,NULL))
  {
    (*_printf) ("Unable to connect to host %s\r\n", host);
    return (NULL);
  }
#if SMTP_ASCII_MODE
  sock_mode (sock, TCP_MODE_ASCII);
#endif

  sock_wait_established (sock, sock_delay, NULL, &SMTP_status);

  while (sock_tbused(sock) > 0)
  {
    SOCK_GETS    (sock);
    SMTP_FAIL_ON (sock, SMTP_OOPS, NO_FUNC);
  }

  if (dom)
  {
    SOCK_PRINTF     (sock, "HELO %s", dom);
    sock_wait_input (sock, sock_delay, NULL, &SMTP_status);
  }

  while (sock_tbused(sock) > 0)
  {
    SOCK_GETS    (sock);
    SMTP_FAIL_ON (sock, SMTP_OOPS,    NO_FUNC);
    SMTP_FAIL_ON (sock, SMTP_SYNTAX,  NO_FUNC);
    SMTP_FAIL_ON (sock, SMTP_PARAM,   NO_FUNC);
    SMTP_FAIL_ON (sock, SMTP_BAD_PARM,NO_FUNC);
  }

  SMTP_READ_ERR (sock, NO_FUNC);
  return (sock);
}


char * smtp_parse_from_line (FILE *f)
{
  char *p    = NULL;
  char buf  [TEMP_BUF_LEN];
  char name [TEMP_BUF_LEN];
  char from [TEMP_BUF_LEN];

  rewind (f);

  while (!feof(f))
  {
    fgets (buf, sizeof(buf), f);
    if (buf[0] == '\n')
       break;

    if (!strnicmp(buf,"from:", 5))
    {
      p = buf + 5;
      break;
    }
    if (!strnicmp(buf,"sender:",7))
    {
      p = buf + 7;
      break;
    }
  }
  if (p)
  {
    if (sscanf(p, "\"%[^\"]\"<%[^>]>", name, from) == 2)
       p = from;
    return strdup (p);
  }
  return (NULL);
}

char ** smtp_parse_to_line (FILE *f)
{
  int    current = 0;
  char **list    = NULL;
  char  *addr;
  char   buf [TEMP_BUF_LEN];

  rewind (f);

  while (!feof(f))
  {
    if (!fgets (buf, sizeof(buf), f))
       return (NULL);

    if (buf[0] == '\n')
       break;

    if (!strnicmp(buf,"to:", 3) ||
        !strnicmp(buf,"cc:", 3) ||
        !strnicmp(buf,"bcc:",4))
    {
      fix_quoted_commas (buf);
      addr = strtok (buf, ":");
      while ((addr = strtok(NULL,",\n")) != NULL)
      {
        list = realloc (list, sizeof(char*)*((current)+2));
        list[current]   = strdup (addr);
        list[current+1] = NULL;
	current++;
      }
    }
  }
  return (list);
}


char * smtp_send_MAIL_FROM_line (tcp_Socket *sock, FILE * f)
{
  char *from = smtp_parse_from_line (f);

  if (!from)
     return (NULL);

  SOCK_PRINTF (sock, "MAIL FROM:%s", trim(from));
  SMTP_DEBUG_MSG ("MAIL FROM:", from);

  while (sock_tbused(sock) > 0)
  {
    SOCK_GETS    (sock);
    SMTP_FAIL_ON (sock, SMTP_OOPS, NO_FUNC);
  }
  SMTP_READ_ERR (sock, NO_FUNC);
  return (from);
}

#define FREE_ALL for (i = 0; to_list[i]; i++) \
                     free (to_list[i]);       \
                 free (to_list);


int smtp_send_RCPT_TO_line (tcp_Socket *sock, FILE *f)
{
  int  i;
  char **to_list = smtp_parse_to_line (f);

  for (i = 0; to_list && to_list[i] && to_list[i][0]; i++)
  {
    SOCK_PRINTF (sock, "RCPT TO:<%s>", trim(to_list[i]));

    while (sock_tbused(sock) > 0)
    {
      SOCK_GETS     (sock);
      SMTP_FAIL_ON  (sock, SMTP_OOPS,   FREE_ALL);
      SMTP_RESET_ON (sock, SMTP_SYNTAX, FREE_ALL);
      SMTP_RESET_ON (sock, SMTP_PARAM,  FREE_ALL);
      SMTP_RESET_ON (sock, SMTP_BAD_SEQ,FREE_ALL);
    }
  }

  SMTP_READ_ERR (sock,FREE_ALL);
  FREE_ALL;                     
  return (1);
}

#undef FREE_ALL


int smtp_sendf (tcp_Socket *sock, FILE *f)
{
  int   in_header = 1;
  int   in_bcc    = 0;
  char *from      = smtp_send_MAIL_FROM_line (sock, f);

  if (!from)
     return (0);

  smtp_send_RCPT_TO_line (sock, f);
  rewind (f);
  free (from);

  PUTS_DATA (sock);
  sock_wait_input (sock, sock_delay, NULL, &SMTP_status);

  while (sock_tbused(sock) > 0)
  {
    SOCK_GETS (sock);
    SMTP_FAIL_ON  (sock, SMTP_OOPS,  NO_FUNC);
    SMTP_RESET_ON (sock, SMTP_SYNTAX,NO_FUNC);
    SMTP_RESET_ON (sock, SMTP_PARAM, NO_FUNC);
    SMTP_RESET_ON (sock, SMTP_COM_NI,NO_FUNC);
    SMTP_RESET_ON (sock, SMTP_FAILED,NO_FUNC);
    SMTP_RESET_ON (sock, SMTP_ERROR, NO_FUNC);
  }

  while (!feof(f))
  {
    char *temp, buf [TEMP_BUF_LEN];

    memset (&buf, 0, sizeof(buf));

    if (!fgets(buf, sizeof(buf), f))
       break;

    rip (buf);
    trim (temp = strdup(buf));
    if (strlen(temp) == 0)
       in_header = 0;
    free (temp);

    if (in_header && !in_bcc && !strnicmp(buf,"bcc:",4))
    {
      in_bcc = 1;
      continue;
    }

    if (in_bcc)
        in_bcc = isspace (buf[0]);

    if (in_bcc)
       continue;

    if (buf[0] == '.' && buf[1] == 0)
    {
      memmove (buf+1, buf, sizeof(buf)-1);
      buf[0] = '.';
    }
    PUTS_BUF (sock, buf);
  }

  PUTS_END (sock);
  sock_wait_input (sock, sock_delay, NULL, &SMTP_status);

  while (sock_tbused(sock) > 0)
  {
    SOCK_GETS     (sock);
    SMTP_FAIL_ON  (sock,SMTP_OOPS,    NO_FUNC);
    SMTP_RESET_ON (sock,SMTP_ERROR,   NO_FUNC);
    SMTP_RESET_ON (sock,SMTP_SQUEEZED,NO_FUNC);
    SMTP_RESET_ON (sock,SMTP_FULL,    NO_FUNC);
    SMTP_RESET_ON (sock,SMTP_FAILED,  NO_FUNC);
  }
  SMTP_READ_ERR (sock, NO_FUNC);
  return (1);
}

int smtp_send_file (tcp_Socket *sock, char *fname)
{
  FILE *f = fopen (fname, "r");

  if (f)
     return smtp_sendf (sock, f);

  strcpy (SMTP_errMsg, "SMTP OK");
  SMTP_errCond = SMTP_BAD_FILE;
  strcpy (SMTP_errMsg, fname);
  return (0);
}

int smtp_shutdown (tcp_Socket *sock)
{
  PUTS_QUIT (sock);
  sock_close (sock);
  return (1);
}

